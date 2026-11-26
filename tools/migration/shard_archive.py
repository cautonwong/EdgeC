#!/usr/bin/env python3
"""P4-03e: offline archive for the v3 MQTT shard queues (read-only on sources).

A v3 cloud-cache shard is ``mqttshardN.db`` with
``data_queue(id INTEGER PRIMARY KEY AUTOINCREMENT, data BLOB)``. Each blob is
one frame, little endian::

    time (signed int, 4 or 8 bytes) | cmd (u32) | datalen (i32) | payload

The archive keeps the raw bytes (payload semantics stay for P4-02/P4-03b),
row counts, source-file hashes and a canonical record hash
``sha256(f"{time_s}|{cmd}|" + payload)``.

Design rules:
- sources are opened with SQLite URI ``mode=ro`` and never moved/removed;
- ``--time-size auto`` only accepts a width when exactly one of 4/8 satisfies
  ``header + datalen == len(blob)`` and ``datalen >= 0``; any ambiguity or
  mixed widths is an error that asks for an explicit ``--time-size``;
- the archive is written to a temp file and renamed into place, so a failed
  scan leaves no half archive (a previous archive stays untouched);
- exit codes: 0 ok, 2 parse/count/verify mismatch, 3 usage/environment.
"""

from __future__ import annotations

import argparse
import contextlib
import hashlib
import io
import os
import re
import sqlite3
import struct
import sys
import tempfile
import time
from dataclasses import dataclass
from pathlib import Path
from urllib.parse import quote

FORMAT = "v3-shard-archive/1"
ARCHIVE_NAME = "shard_archive.db"
SHARD_COUNT = 20
SHARD_RE = re.compile(r"^mqttshard(\d+)\.db$")
TIME_SIZES = (4, 8)
EXIT_OK = 0
EXIT_MISMATCH = 2
EXIT_USAGE = 3


class UsageError(Exception):
    """Bad invocation or environment (exit 3)."""


@dataclass
class Frame:
    time_s: int
    cmd: int
    payload: bytes


@dataclass
class ShardScan:
    index: int
    path: Path
    file_bytes: int
    sha256: str
    rows: int


@dataclass
class ScanResult:
    shards: list[ShardScan]
    rows_total: int
    payload_bytes: int
    time_size: int | None
    errors: list[str]
    missing: list[int]


def find_shards(source_dir: Path) -> dict[int, Path]:
    if not source_dir.is_dir():
        raise UsageError(f"source dir is not a directory: {source_dir}")
    found: dict[int, Path] = {}
    for path in sorted(source_dir.iterdir()):
        match = SHARD_RE.match(path.name)
        if not match or not path.is_file():
            continue
        index = int(match.group(1))
        if 0 <= index < SHARD_COUNT:
            found[index] = path
    return found


def open_ro(path: Path) -> sqlite3.Connection:
    uri = f"file:{quote(path.as_posix(), safe='/')}?mode=ro"
    try:
        return sqlite3.connect(uri, uri=True)
    except sqlite3.Error as error:
        raise UsageError(f"cannot open {path} read-only: {error}") from error


def file_sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with open(path, "rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def try_parse(blob: bytes, time_size: int):
    """Return a Frame when the blob holds exactly one frame of this width."""
    header = time_size + 8
    if len(blob) < header:
        return None
    datalen = int.from_bytes(blob[time_size + 4 : time_size + 8], "little", signed=True)
    if datalen < 0 or header + datalen != len(blob):
        return None
    time_s = int.from_bytes(blob[:time_size], "little", signed=True)
    cmd = int.from_bytes(blob[time_size : time_size + 4], "little", signed=False)
    return Frame(time_s, cmd, blob[header:])


def record_hash(time_s: int, cmd: int, payload: bytes) -> str:
    canonical = f"{time_s}|{cmd}|".encode("ascii") + payload
    return hashlib.sha256(canonical).hexdigest()


def _blob_of(value) -> bytes:
    if value is None:
        return b""
    return bytes(value)


def _shard_label(path: Path, index: int) -> str:
    return f"shard={index} file={path.name}"


def scan_shards(source_dir: Path, time_size_opt) -> ScanResult:
    """First pass: validate every row, resolve the width, count and hash files."""
    shards = find_shards(source_dir)
    scans: list[ShardScan] = []
    errors: list[str] = []
    rows_total = 0
    payload_bytes = 0
    resolved: int | None = time_size_opt if time_size_opt in TIME_SIZES else None

    for index in sorted(shards):
        path = shards[index]
        conn = open_ro(path)
        try:
            cursor = conn.execute("SELECT id, data FROM data_queue ORDER BY id")
            rows = 0
            for src_id, value in cursor:
                blob = _blob_of(value)
                if time_size_opt in TIME_SIZES:
                    frame = try_parse(blob, time_size_opt)
                    if frame is None:
                        errors.append(
                            f"{_shard_label(path, index)} id={src_id}: frame does not match "
                            f"time_size={time_size_opt} (bytes={len(blob)})"
                        )
                        continue
                else:
                    widths = [w for w in TIME_SIZES if try_parse(blob, w) is not None]
                    if not widths:
                        errors.append(
                            f"{_shard_label(path, index)} id={src_id}: no 4/8 byte frame matches "
                            f"(bytes={len(blob)})"
                        )
                        continue
                    if len(widths) > 1:
                        errors.append(
                            f"{_shard_label(path, index)} id={src_id}: ambiguous time width "
                            f"4 and 8 both match; pass --time-size 4|8"
                        )
                        continue
                    width = widths[0]
                    if resolved is None:
                        resolved = width
                    elif resolved != width:
                        errors.append(
                            f"{_shard_label(path, index)} id={src_id}: time width {width} "
                            f"disagrees with earlier rows ({resolved}); pass --time-size 4|8"
                        )
                        continue
                    frame = try_parse(blob, width)
                    if frame is None:  # pragma: no cover - widths came from try_parse
                        errors.append(f"{_shard_label(path, index)} id={src_id}: frame vanished")
                        continue
                rows += 1
                rows_total += 1
                payload_bytes += len(frame.payload)
        finally:
            conn.close()
        try:
            file_bytes = path.stat().st_size
            digest = file_sha256(path)
        except OSError as error:
            raise UsageError(f"cannot read {path}: {error}") from error
        scans.append(ShardScan(index, path, file_bytes, digest, rows))

    missing = [i for i in range(SHARD_COUNT) if i not in shards]
    return ScanResult(scans, rows_total, payload_bytes, resolved, errors, missing)


def _report(plan: ScanResult, source_dir: Path, out_db: Path | None) -> None:
    print(f"source_dir={source_dir}")
    for shard in plan.shards:
        print(
            f"  shard={shard.index} file={shard.path.name} rows={shard.rows} "
            f"bytes={shard.file_bytes} sha256={shard.sha256}"
        )
    if plan.missing:
        print("  missing shards: " + ", ".join(str(i) for i in plan.missing))
    print(
        f"rows_total={plan.rows_total} payload_bytes={plan.payload_bytes} "
        f"time_size={plan.time_size if plan.time_size is not None else 'unresolved'}"
    )
    if out_db is not None:
        print(f"archive={out_db}")


def write_archive(plan: ScanResult, out_dir: Path) -> Path:
    """Second pass: write records, then atomically rename the archive into place."""
    if plan.errors:
        raise UsageError("refusing to write an archive from a failed scan")
    out_dir.mkdir(parents=True, exist_ok=True)
    out_db = out_dir / ARCHIVE_NAME
    fd, tmp_name = tempfile.mkstemp(prefix=".shard_archive.", suffix=".db", dir=out_dir)
    os.close(fd)
    tmp_path = Path(tmp_name)
    conn = sqlite3.connect(tmp_path)
    try:
        conn.executescript(
            """
            CREATE TABLE meta (key TEXT PRIMARY KEY, value TEXT);
            CREATE TABLE shards (
                shard INTEGER PRIMARY KEY, file TEXT, bytes INTEGER,
                sha256 TEXT, rows INTEGER);
            CREATE TABLE records (
                shard INTEGER, src_id INTEGER, time_s INTEGER, cmd INTEGER,
                payload BLOB, sha256 TEXT, PRIMARY KEY (shard, src_id));
            """
        )
        time_size = plan.time_size if plan.time_size is not None else 4
        meta = {
            "format": FORMAT,
            "time_size": str(time_size),
            "source_dir": str(plan.shards[0].path.parent) if plan.shards else "",
            "shards_present": str(len(plan.shards)),
            "rows_total": str(plan.rows_total),
            "bytes_total": str(plan.payload_bytes),
            "created_at": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        }
        conn.executemany("INSERT INTO meta (key, value) VALUES (?, ?)", meta.items())
        conn.executemany(
            "INSERT INTO shards (shard, file, bytes, sha256, rows) VALUES (?, ?, ?, ?, ?)",
            [
                (s.index, s.path.name, s.file_bytes, s.sha256, s.rows)
                for s in plan.shards
            ],
        )
        for shard in plan.shards:
            conn_ro = open_ro(shard.path)
            try:
                for src_id, value in conn_ro.execute(
                    "SELECT id, data FROM data_queue ORDER BY id"
                ):
                    frame = try_parse(_blob_of(value), time_size)
                    if frame is None:
                        raise UsageError(
                            f"{shard.path.name} id={src_id} changed between scan and write"
                        )
                    conn.execute(
                        "INSERT INTO records (shard, src_id, time_s, cmd, payload, sha256) "
                        "VALUES (?, ?, ?, ?, ?, ?)",
                        (
                            shard.index,
                            src_id,
                            frame.time_s,
                            frame.cmd,
                            frame.payload,
                            record_hash(frame.time_s, frame.cmd, frame.payload),
                        ),
                    )
            finally:
                conn_ro.close()
        conn.commit()
    except BaseException:
        conn.close()
        with contextlib.suppress(OSError):
            tmp_path.unlink()
        raise
    conn.close()
    os.replace(tmp_path, out_db)
    return out_db


def run_archive(source_dir: Path, out_dir: Path | None, time_size_opt, dry_run: bool) -> int:
    try:
        plan = scan_shards(source_dir, time_size_opt)
    except UsageError as error:
        print(f"error: {error}", file=sys.stderr)
        return EXIT_USAGE
    out_db = None if dry_run or out_dir is None else out_dir / ARCHIVE_NAME
    _report(plan, source_dir, out_db)
    if plan.errors:
        for error in plan.errors:
            print(f"error: {error}", file=sys.stderr)
        return EXIT_MISMATCH
    if dry_run:
        return EXIT_OK
    assert out_dir is not None
    try:
        archive = write_archive(plan, out_dir)
    except UsageError as error:
        print(f"error: {error}", file=sys.stderr)
        return EXIT_MISMATCH
    except (OSError, sqlite3.Error) as error:
        print(f"error: cannot write archive: {error}", file=sys.stderr)
        return EXIT_USAGE
    print(f"archive written: {archive}")
    return EXIT_OK


def run_verify(archive: Path, source_dir: Path | None) -> int:
    if archive.is_dir():
        archive = archive / ARCHIVE_NAME
    if not archive.is_file():
        print(f"error: archive not found: {archive}", file=sys.stderr)
        return EXIT_USAGE
    try:
        conn = open_ro(archive)
    except UsageError as error:
        print(f"error: {error}", file=sys.stderr)
        return EXIT_USAGE

    errors: list[str] = []
    counted_rows: dict[int, int] = {}
    counted_bytes = 0
    try:
        meta = dict(conn.execute("SELECT key, value FROM meta"))
        if meta.get("format") != FORMAT:
            errors.append(f"meta: format is {meta.get('format')!r}, expected {FORMAT!r}")
        shards = {
            row[0]: row for row in conn.execute("SELECT shard, file, bytes, sha256, rows FROM shards")
        }
        for shard, src_id, time_s, cmd, payload, stored in conn.execute(
            "SELECT shard, src_id, time_s, cmd, payload, sha256 FROM records"
        ):
            payload = _blob_of(payload)
            if record_hash(time_s, cmd, payload) != stored:
                errors.append(f"record shard={shard} id={src_id}: sha256 mismatch")
            counted_rows[shard] = counted_rows.get(shard, 0) + 1
            counted_bytes += len(payload)
    except sqlite3.Error as error:
        conn.close()
        print(f"error: cannot read archive: {error}", file=sys.stderr)
        return EXIT_MISMATCH

    def _meta_int(key: str) -> int | None:
        try:
            return int(meta.get(key, ""))
        except ValueError:
            errors.append(f"meta: {key} is not an integer: {meta.get(key)!r}")
            return None

    rows_total = sum(counted_rows.values())
    if _meta_int("rows_total") != rows_total:
        errors.append(f"meta: rows_total {meta.get('rows_total')!r} != records {rows_total}")
    if _meta_int("bytes_total") != counted_bytes:
        errors.append(f"meta: bytes_total {meta.get('bytes_total')!r} != records {counted_bytes}")
    if _meta_int("shards_present") != len(shards):
        errors.append(f"meta: shards_present {meta.get('shards_present')!r} != shards {len(shards)}")
    for shard, row in shards.items():
        if counted_rows.get(shard, 0) != int(row[4]):
            errors.append(
                f"shards: shard={shard} rows {row[4]} != records {counted_rows.get(shard, 0)}"
            )

    if source_dir is not None:
        if not source_dir.is_dir():
            errors.append(f"source: not a directory: {source_dir}")
        else:
            for shard, row in sorted(shards.items()):
                path = source_dir / row[1]
                if not path.is_file():
                    errors.append(f"source: shard={shard} file missing: {path}")
                    continue
                size = path.stat().st_size
                if size != int(row[2]):
                    errors.append(f"source: shard={shard} bytes {size} != archive {row[2]}")
                digest = file_sha256(path)
                if digest != row[3]:
                    errors.append(f"source: shard={shard} sha256 {digest} != archive {row[3]}")
                try:
                    conn_src = open_ro(path)
                    try:
                        count = conn_src.execute("SELECT COUNT(*) FROM data_queue").fetchone()[0]
                    finally:
                        conn_src.close()
                except (UsageError, sqlite3.Error) as error:
                    errors.append(f"source: shard={shard} unreadable: {error}")
                    continue
                if count != int(row[4]):
                    errors.append(f"source: shard={shard} rows {count} != archive {row[4]}")

    conn.close()
    if errors:
        for error in errors:
            print(f"error: {error}", file=sys.stderr)
        return EXIT_MISMATCH
    print(
        f"verify ok: rows_total={rows_total} bytes_total={counted_bytes} "
        f"shards={len(shards)}"
    )
    return EXIT_OK


# ---------------------------------------------------------------------------
# selftest: synthetic shards, no v3 data required
# ---------------------------------------------------------------------------


def _frame(time_s: int, cmd: int, payload: bytes, time_size: int) -> bytes:
    return (
        int(time_s).to_bytes(time_size, "little", signed=True)
        + struct.pack("<Ii", cmd, len(payload))
        + payload
    )


def _make_shard(path: Path, blobs: list[bytes]) -> None:
    if path.exists():
        path.unlink()
    conn = sqlite3.connect(path)
    try:
        conn.execute(
            "CREATE TABLE data_queue (id INTEGER PRIMARY KEY AUTOINCREMENT, data BLOB)"
        )
        conn.executemany("INSERT INTO data_queue (data) VALUES (?)", [(b,) for b in blobs])
        conn.commit()
    finally:
        conn.close()


def _snapshot(directory: Path) -> dict[str, tuple[int, int, str]]:
    return {
        path.name: (path.stat().st_size, path.stat().st_mtime_ns, file_sha256(path))
        for path in sorted(directory.glob("mqttshard*.db"))
    }


def run_selftest() -> int:
    checks: list[tuple[str, bool, str]] = []

    def check(name: str, ok: bool, detail: str = "") -> None:
        checks.append((name, ok, detail))

    with tempfile.TemporaryDirectory(prefix="shard_archive_selftest_") as tmp:
        base = Path(tmp)
        t = 1700000000

        src4 = base / "src4"
        src4.mkdir()
        _make_shard(src4 / "mqttshard0.db", [_frame(t, 1, b"ab", 4), _frame(t + 1, 2, b"cde", 4)])
        _make_shard(src4 / "mqttshard1.db", [_frame(t + 2, 3, b"\xde\xad\xbe\xef\x01\x02", 4)])
        _make_shard(src4 / "mqttshard2.db", [_frame(t + 3, 4, b"", 4)])
        before4 = _snapshot(src4)

        # 1. 4-byte time: dry-run counts, archive totals, verify.
        plan4 = scan_shards(src4, 4)
        check(
            "1a 4-byte scan",
            not plan4.errors and plan4.rows_total == 4 and plan4.payload_bytes == 11,
            f"errors={plan4.errors} rows={plan4.rows_total} bytes={plan4.payload_bytes}",
        )
        rc_dry = run_archive(src4, None, 4, dry_run=True)
        check("1b 4-byte dry-run", rc_dry == 0, f"rc={rc_dry}")
        rc = run_archive(src4, base / "out4", 4, dry_run=False)
        out4 = base / "out4" / ARCHIVE_NAME
        check("1c 4-byte archive", rc == 0 and out4.is_file(), f"rc={rc}")
        check("1d 4-byte verify", run_verify(out4, src4) == 0)
        with contextlib.suppress(sqlite3.Error):
            conn = sqlite3.connect(out4)
            meta = dict(conn.execute("SELECT key, value FROM meta"))
            conn.close()
            check(
                "1e 4-byte meta",
                meta.get("rows_total") == "4" and meta.get("bytes_total") == "11",
                str(meta),
            )

        # 2. 8-byte time: same shape.
        src8 = base / "src8"
        src8.mkdir()
        _make_shard(src8 / "mqttshard0.db", [_frame(t, 5, b"xy", 8), _frame(t + 1, 6, b"z", 8)])
        _make_shard(src8 / "mqttshard1.db", [_frame(t + 2, 7, b"12345", 8)])
        _make_shard(src8 / "mqttshard2.db", [_frame(t + 3, 8, b"", 8)])
        plan8 = scan_shards(src8, 8)
        check(
            "2a 8-byte scan",
            not plan8.errors and plan8.rows_total == 4 and plan8.payload_bytes == 8,
            f"errors={plan8.errors} rows={plan8.rows_total} bytes={plan8.payload_bytes}",
        )
        rc8 = run_archive(src8, base / "out8", 8, dry_run=False)
        out8 = base / "out8" / ARCHIVE_NAME
        check("2b 8-byte archive", rc8 == 0 and out8.is_file(), f"rc={rc8}")
        check("2c 8-byte verify", run_verify(out8, src8) == 0)

        # 3. datalen mismatch: exit 2 and no archive file.
        srcbad = base / "srcbad"
        srcbad.mkdir()
        bad = _frame(t, 1, b"abc", 4)
        bad = bad[:-1]  # datalen says 3, payload now 2 bytes
        _make_shard(srcbad / "mqttshard0.db", [bad])
        rc_bad = run_archive(srcbad, base / "outbad", "auto", dry_run=False)
        check(
            "3 bad frame rejected",
            rc_bad == EXIT_MISMATCH
            and not (base / "outbad" / ARCHIVE_NAME).exists(),
            f"rc={rc_bad}",
        )

        # 4. tamper one record payload: verify names shard/id and fails.
        conn = sqlite3.connect(out4)
        conn.execute(
            "UPDATE records SET payload = ? WHERE shard = 0 AND src_id = 1",
            (b"XX",),
        )
        conn.commit()
        conn.close()
        stderr = io.StringIO()
        with contextlib.redirect_stderr(stderr):
            rc_tamper = run_verify(out4, src4)
        check(
            "4 tamper detected",
            rc_tamper == EXIT_MISMATCH and "shard=0 id=1" in stderr.getvalue(),
            f"rc={rc_tamper} stderr={stderr.getvalue().strip()}",
        )

        # 5. idempotent re-archive; sources keep size/mtime/hash.
        out5 = base / "out5"
        rc_a = run_archive(src4, out5, 4, dry_run=False)
        first = _archive_fingerprint(out5 / ARCHIVE_NAME)
        rc_b = run_archive(src4, out5, 4, dry_run=False)
        second = _archive_fingerprint(out5 / ARCHIVE_NAME)
        after4 = _snapshot(src4)
        check(
            "5 idempotent + read-only",
            rc_a == 0 and rc_b == 0 and first == second and before4 == after4,
            f"rc={rc_a}/{rc_b} same={first == second} sources_unchanged={before4 == after4}",
        )

        # 6. missing shard: reported, totals count present shards only.
        src6 = base / "src6"
        src6.mkdir()
        _make_shard(src6 / "mqttshard0.db", [_frame(t, 1, b"a", 4)])
        _make_shard(src6 / "mqttshard2.db", [_frame(t + 1, 2, b"bcd", 4)])
        plan6 = scan_shards(src6, "auto")
        rc6 = run_archive(src6, base / "out6", "auto", dry_run=False)
        out6 = base / "out6" / ARCHIVE_NAME
        ok6 = (
            not plan6.errors
            and plan6.missing == [i for i in range(1, 20) if i != 2]
            and plan6.rows_total == 2
            and plan6.payload_bytes == 4
            and rc6 == 0
            and run_verify(out6, None) == 0
        )
        check("6 missing shard", ok6, f"missing={plan6.missing} rc={rc6}")

    # 子断言按用例编号（名字首字符）归并成 6 个用例。
    cases: dict[str, list[tuple[str, bool, str]]] = {}
    for name, ok, detail in checks:
        cases.setdefault(name[0], []).append((name, ok, detail))
    case_order = ["1", "2", "3", "4", "5", "6"]
    failed_cases = [c for c in case_order if not all(ok for _, ok, _ in cases.get(c, []))]
    passed = len(case_order) - len(failed_cases)
    if passed == len(case_order):
        print(f"selftest {passed}/{len(case_order)} ok")
        return EXIT_OK
    print(f"selftest {passed}/{len(case_order)} failed:")
    for name, ok, detail in checks:
        if not ok:
            print(f"  - {name}: {detail}")
    return EXIT_MISMATCH


def _archive_fingerprint(archive: Path) -> tuple:
    conn = sqlite3.connect(archive)
    try:
        meta = dict(conn.execute("SELECT key, value FROM meta"))
        records = conn.execute(
            "SELECT shard, src_id, time_s, cmd, payload, sha256 FROM records "
            "ORDER BY shard, src_id"
        ).fetchall()
        shards = conn.execute(
            "SELECT shard, file, bytes, sha256, rows FROM shards ORDER BY shard"
        ).fetchall()
    finally:
        conn.close()
    return (
        meta.get("format"),
        meta.get("time_size"),
        meta.get("rows_total"),
        meta.get("bytes_total"),
        meta.get("shards_present"),
        tuple((sid, time_s, cmd, bytes(payload), sha) for _, sid, time_s, cmd, payload, sha in records),
        tuple(shards),
    )


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------


class _Parser(argparse.ArgumentParser):
    def error(self, message):  # exit 3 for usage errors, per the tool contract
        self.print_usage(sys.stderr)
        self.exit(EXIT_USAGE, f"{self.prog}: error: {message}\n")


def build_parser() -> argparse.ArgumentParser:
    parser = _Parser(description="Archive v3 MQTT shard queues (read-only on sources).")
    parser.add_argument("--source-dir", type=Path, help="directory with mqttshard0..19.db")
    parser.add_argument("--out", type=Path, help="archive output directory")
    parser.add_argument(
        "--time-size",
        choices=("auto", "4", "8"),
        default="auto",
        help="time_t width; auto rejects ambiguity instead of guessing",
    )
    parser.add_argument("--dry-run", action="store_true", help="scan/report only, write nothing")
    parser.add_argument("--verify", type=Path, metavar="ARCHIVE", help="verify an archive")
    parser.add_argument("--selftest", action="store_true", help="run the 6 synthetic checks")
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)

    if args.selftest:
        return run_selftest()
    if args.verify is not None:
        return run_verify(args.verify, args.source_dir)
    if args.source_dir is None:
        parser.error("--source-dir is required unless --verify/--selftest is used")
    if not args.dry_run and args.out is None:
        parser.error("--out is required unless --dry-run is used")
    time_size = args.time_size if args.time_size == "auto" else int(args.time_size)
    return run_archive(args.source_dir, args.out, time_size, args.dry_run)


if __name__ == "__main__":
    sys.exit(main())

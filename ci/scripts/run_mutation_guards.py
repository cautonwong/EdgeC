#!/usr/bin/env python3
"""Prove each invariant test turns red when its real defect is reintroduced.

`run_guard_selftest.py` proves the guard scripts reject a bad fixture. This
runner proves the opposite side: the product tests behind a historical defect
fail when the fix is neutralized. A test that stays green under the defect it
guards proves nothing.

Each manifest row is applied alone: the find string must occur exactly once,
the mutated target is rebuilt, the named test must fail, the source is restored
byte for byte, rebuilt, and the test must pass again. Any deviation exits
non-zero. Source restore runs in a finally block, and SIGINT/SIGTERM/SIGHUP
raise, so a killed run still restores before exiting. SIGKILL cannot be caught;
the next run refuses to start on a dirty source file.

Manifest fields are tab separated:
  name  file  find  replace  build_target  test_cmd  timeout
`\\n` and `\\t` in find/replace decode to a newline and a tab. `{build}` in
test_cmd is replaced with the build directory. Blank lines and `#` comments
are ignored.

Usage:
  python3 ci/scripts/run_mutation_guards.py --build build
  python3 ci/scripts/run_mutation_guards.py --build build --only P6-04b-rollback
  python3 ci/scripts/run_mutation_guards.py --selftest
"""

import argparse
import hashlib
import os
import shlex
import shutil
import signal
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
MANIFEST = os.path.join(REPO_ROOT, "ci", "mutation_guards.tsv")


@dataclass(frozen=True)
class Entry:
    name: str
    file: str
    find: str
    replace: str
    build_target: str
    test_cmd: str
    timeout: int


def _decode(field):
    return field.replace("\\n", "\n").replace("\\t", "\t")


def parse_manifest(path):
    entries = []
    with open(path, encoding="utf-8") as handle:
        for lineno, raw in enumerate(handle, 1):
            line = raw.rstrip("\r\n")
            if not line.strip() or line.lstrip().startswith("#"):
                continue
            fields = line.split("\t")
            if len(fields) != 7:
                raise SystemExit(
                    f"{path}:{lineno}: expected 7 tab-separated columns, got {len(fields)}"
                )
            name, file, find, replace, target, cmd, timeout = fields
            entries.append(
                Entry(
                    name.strip(),
                    file.strip(),
                    _decode(find),
                    _decode(replace),
                    target.strip(),
                    cmd.strip(),
                    int(timeout),
                )
            )
    return entries


def _sha256(data):
    return hashlib.sha256(data).hexdigest()


def _tail(text, limit=4000):
    return (text or "").strip()[-limit:]


def _run(argv, timeout, cwd, shell=False):
    try:
        proc = subprocess.run(
            argv,
            cwd=cwd,
            shell=shell,
            stdin=subprocess.DEVNULL,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            timeout=timeout,
        )
    except subprocess.TimeoutExpired as exc:
        out = exc.stdout or ""
        if isinstance(out, bytes):
            out = out.decode("utf-8", "replace")
        return 124, f"timeout after {timeout}s\n{out}"
    except OSError as exc:
        return 127, str(exc)
    return proc.returncode, proc.stdout


def _test_cmd(entry, build_dir):
    return entry.test_cmd.replace("{build}", shlex.quote(build_dir))


def _git_dirty(root, relpath):
    proc = subprocess.run(
        ["git", "-C", root, "status", "--porcelain", "--untracked-files=no", "--", relpath],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    if proc.returncode != 0:
        raise RuntimeError(f"git status failed for {relpath}: {proc.stdout.strip()}")
    return proc.stdout.strip()


def _restore(path, original, mutated_sha):
    current = path.read_bytes()
    if _sha256(current) != mutated_sha:
        raise RuntimeError(f"{path} changed while mutated; refusing to overwrite")
    path.write_bytes(original)
    if _sha256(path.read_bytes()) != _sha256(original):
        raise RuntimeError(f"restore hash mismatch for {path}")


def check_entry(entry, root, build_dir, build_argv_for, check_clean=True):
    """Return (ok, detail). Restores the source on every path, including errors."""
    path = Path(root) / entry.file
    if not path.is_file():
        return False, f"missing file: {entry.file}"
    if check_clean:
        dirty = _git_dirty(root, entry.file)
        if dirty:
            return False, f"{entry.file} has uncommitted changes; refusing to mutate"

    original = path.read_bytes()
    needle = entry.find.encode()
    count = original.count(needle)
    if count != 1:
        return False, f"find string occurs {count} times in {entry.file}; expected exactly 1"
    mutated = original.replace(needle, entry.replace.encode(), 1)
    mutated_sha = _sha256(mutated)

    print(f"  [mutate] {entry.file}: 1 occurrence replaced")
    active = True
    try:
        path.write_bytes(mutated)
        build_rc, build_out = _run(build_argv_for(entry), entry.timeout, root)
        test_rc = None
        test_out = ""
        if build_rc == 0:
            test_rc, test_out = _run(
                _test_cmd(entry, build_dir), entry.timeout, root, shell=True
            )
        print(
            "  [mutated] build rc=%s, test rc=%s"
            % (build_rc, "skipped" if test_rc is None else test_rc)
        )

        _restore(path, original, mutated_sha)
        active = False
        print(f"  [restore] {entry.file}: original bytes and hash verified")

        green_build_rc, green_build_out = _run(
            build_argv_for(entry), entry.timeout, root
        )
        green_test_rc = None
        green_test_out = ""
        if green_build_rc == 0:
            green_test_rc, green_test_out = _run(
                _test_cmd(entry, build_dir), entry.timeout, root, shell=True
            )
        print(
            "  [restored] build rc=%s, test rc=%s"
            % (green_build_rc, "skipped" if green_test_rc is None else green_test_rc)
        )

        if build_rc != 0:
            return False, f"build failed under mutation (rc={build_rc})\n{_tail(build_out)}"
        if green_build_rc != 0:
            return False, (
                f"rebuild after restore failed (rc={green_build_rc})\n"
                f"{_tail(green_build_out)}"
            )
        if test_rc == 0:
            return False, (
                f"INSENSITIVE_GUARD {entry.name}: test still passes under mutation"
            )
        if green_test_rc != 0:
            return False, (
                f"test not green after restore (rc={green_test_rc})\n"
                f"{_tail(green_test_out)}"
            )
        return True, f"red under mutation (rc={test_rc}), green after restore"
    finally:
        if active:
            try:
                _restore(path, original, mutated_sha)
                print(f"  [restore] {entry.file}: recovered after failure")
            except Exception as exc:  # noqa: BLE001 - report, never mask the crash
                print(
                    f"  [FATAL] {exc}; source file may still be mutated",
                    file=sys.stderr,
                )


def _raise_interrupt(signum, _frame):
    raise KeyboardInterrupt(f"signal {signum}")


def run_manifest(args):
    build_dir = os.path.abspath(args.build)
    if not os.path.isfile(os.path.join(build_dir, "CMakeCache.txt")):
        print(f"❌ build directory is not configured: {build_dir}")
        return 2
    if not os.path.isfile(os.path.join(build_dir, "build.ninja")):
        print(f"❌ build directory is not a Ninja build: {build_dir}")
        return 2
    if shutil.which("ninja") is None:
        print("❌ ninja not found on PATH")
        return 2

    entries = parse_manifest(MANIFEST)
    if args.only:
        wanted = [name.strip() for name in args.only.split(",") if name.strip()]
        known = {entry.name for entry in entries}
        unknown = [name for name in wanted if name not in known]
        if unknown:
            print(f"❌ unknown guard name(s): {', '.join(unknown)}")
            return 2
        entries = [entry for entry in entries if entry.name in wanted]

    for sig in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP):
        signal.signal(sig, _raise_interrupt)

    def build_argv_for(entry):
        return ["ninja", "-C", build_dir, entry.build_target]

    print(f"=== Mutation guards: {len(entries)} entries, build {build_dir} ===")
    failures = []
    for entry in entries:
        print(f"\n[MUTATION] {entry.name} ({entry.file})")
        try:
            ok, detail = check_entry(entry, REPO_ROOT, build_dir, build_argv_for)
        except KeyboardInterrupt:
            raise
        except Exception as exc:  # noqa: BLE001 - one broken entry must not hide the rest
            ok, detail = False, f"restore/verify failed: {exc}"
        if ok:
            print(f"✅ [MUTATION] {entry.name}: {detail}")
        elif detail.startswith("INSENSITIVE_GUARD"):
            print(f"❌ {detail}")
            failures.append(entry.name)
        else:
            print(f"❌ [MUTATION] {entry.name}: {detail}")
            failures.append(entry.name)

    print()
    if failures:
        print(f"❌ [MUTATION_GUARDS] failed: {', '.join(failures)}")
        return 1
    # The target build relinks only its own dependents; product binaries link the
    # same static libraries, so refresh everything to keep the gate's freshness
    # check green.
    catch_up_rc, catch_up_out = _run(
        ["ninja", "-C", build_dir],
        max(entry.timeout for entry in entries),
        REPO_ROOT,
    )
    if catch_up_rc != 0:
        print(
            "❌ [MUTATION_GUARDS] catch-up build after restore failed "
            f"(rc={catch_up_rc})\n{_tail(catch_up_out)}"
        )
        return 1
    print(
        f"✅ [MUTATION_GUARDS] {len(entries)}/{len(entries)} guards sensitive, "
        "build refreshed"
    )
    return 0


def run_selftest():
    failures = []

    def red_on_mutation(path):
        return f"! grep -q MUTATED {shlex.quote(path)}"

    def run_case(label, source, find, replace, build_argv, test_for, expect_ok, expect_detail=None):
        with tempfile.TemporaryDirectory(prefix="mutation-selftest-") as tmp:
            path = os.path.join(tmp, "guarded.c")
            with open(path, "w", encoding="utf-8") as handle:
                handle.write(source)
            entry = Entry("selftest", path, find, replace, "fake", test_for(path), 30)
            ok, detail = check_entry(
                entry, tmp, tmp, lambda _entry: build_argv, check_clean=False
            )
            with open(path, encoding="utf-8") as handle:
                restored = handle.read() == source
            good = (
                ok == expect_ok
                and restored
                and (expect_detail is None or expect_detail in detail)
            )
            if good:
                print(f"  ✅ [SELFTEST] {label}")
            else:
                failures.append(label)
                print(
                    f"  ❌ [SELFTEST] {label}: ok={ok} restored={restored} detail={detail}"
                )

    run_case(
        "sensitive mutation is red, restore is green",
        "int guard = 1;\n",
        "int guard = 1;",
        "int guard = 0; /* MUTATED */",
        ["true"],
        red_on_mutation,
        True,
    )
    run_case(
        "insensitive mutation is reported",
        "int guard = 1;\n",
        "int guard = 1;",
        "int guard = 0; /* MUTATED */",
        ["true"],
        lambda _path: "true",
        False,
        "INSENSITIVE_GUARD",
    )
    run_case(
        "find string with 0 occurrences fails",
        "int guard = 1;\n",
        "absent line",
        "x",
        ["true"],
        lambda _path: "true",
        False,
        "occurs 0 times",
    )
    run_case(
        "find string with 2 occurrences fails",
        "ALPHA\nALPHA\n",
        "ALPHA",
        "BETA",
        ["true"],
        lambda _path: "true",
        False,
        "occurs 2 times",
    )
    run_case(
        "build failure is reported and source restored",
        "int guard = 1;\n",
        "int guard = 1;",
        "int guard = 0; /* MUTATED */",
        ["false"],
        red_on_mutation,
        False,
        "build failed under mutation",
    )

    if _decode(r"a\nb\tc") == "a\nb\tc":
        print("  ✅ [SELFTEST] \\n and \\t escapes decode")
    else:
        failures.append("escape decoding")
        print("  ❌ [SELFTEST] escape decoding")

    print()
    if failures:
        print(f"❌ [SELFTEST] failed: {', '.join(failures)}")
        return 1
    print("✅ [SELFTEST] mutation runner decision logic verified")
    return 0


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--build", help="configured Ninja build directory")
    parser.add_argument("--only", help="comma-separated guard names to run")
    parser.add_argument("--selftest", action="store_true", help="verify the runner logic")
    args = parser.parse_args(argv)

    if args.selftest:
        return run_selftest()
    if not args.build:
        parser.error("--build is required unless --selftest is given")
    return run_manifest(args)


if __name__ == "__main__":
    sys.exit(main())

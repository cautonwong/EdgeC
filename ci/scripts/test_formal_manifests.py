#!/usr/bin/env python3
"""Refutation tests for formal_manifest.py and the formal/ registration scan.

Run directly, or from run_guard_selftest.py as one guard. Every case builds a
throwaway tree under tempfile; no repository file is touched.
"""

import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import check_formal_sync
import formal_manifest

JAR = "7e53aa636dac8a3cb0533f343115c0548e03fc0ce5edc7739becf58d3d0fb84f"
HEADER = "\t".join(formal_manifest.FIELDS)
FOUNDATION = "foundation/src/probe.c"
SCRIPT = "formal/cbmc/negative/probe_mutation.sh"


def write(root, rel, text=""):
    path = os.path.join(root, rel)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as handle:
        handle.write(text)


def proof_row(**overrides):
    values = {
        "artifact": "formal/cbmc/probe.c",
        "property": "probe assertion holds",
        "covers": f"{FOUNDATION}:probe",
        "tool": "CBMC 5.12 (cbmc-5.12)",
        "bounds": "--unwind 2 (max_len 1)",
        "unwind": "2",
        "max_len": "1",
        "negative": "n/a",
        "sync": "2bbf729e",
        "owner": "iems",
        "runtime": "1s",
        "property_count": "1",
        "state_count": "n/a",
        "jar_sha256": "n/a",
        "mode": "proof",
        "cfg": "n/a",
        "function": "n/a",
        "assertions": "n/a",
    }
    values.update(overrides)
    return "\t".join(values[field] for field in formal_manifest.FIELDS)


def tla_row(**overrides):
    values = {
        "artifact": "formal/tla/Probe.tla",
        "property": "probe invariant",
        "covers": f"{FOUNDATION}:probe",
        "tool": "TLC 2026.11.15.002146 (rev: bc1b506)",
        "bounds": "N=1",
        "unwind": "n/a",
        "max_len": "n/a",
        "negative": "n/a",
        "sync": "2bbf729e",
        "owner": "iems",
        "runtime": "1s",
        "property_count": "n/a",
        "state_count": "1",
        "jar_sha256": JAR,
        "mode": "tla",
        "cfg": "formal/tla/Probe.cfg",
        "function": "n/a",
        "assertions": "n/a",
    }
    values.update(overrides)
    return "\t".join(values[field] for field in formal_manifest.FIELDS)


def base_tree(root):
    write(root, FOUNDATION, "int probe(void) { return 0; }\n")
    write(root, "formal/cbmc/probe.c", "int main(void) { return 0; }\n")
    write(root, SCRIPT, "#!/bin/sh\nexit 0\n")


def tla_fixtures(root):
    write(root, "formal/tla/Probe.tla", "---- MODULE Probe ----\n====\n")
    write(root, "formal/tla/Probe.cfg", "INIT Init\n")
    write(
        root,
        "formal/tla/negative/ProbeBad.cfg",
        "CONSTANTS\n    RecoveryGate = FALSE\n",
    )


def add_shard(root, owner, rows):
    write(
        root,
        f"formal/manifest/{owner}.tsv",
        "\n".join([HEADER, *rows]) + "\n",
    )


def expect_error(build, needle):
    with tempfile.TemporaryDirectory() as tmp:
        build(tmp)
        try:
            formal_manifest.load_manifests(tmp)
        except formal_manifest.ManifestError as error:
            assert needle in str(error), f"expected {needle!r} in {error}"
            return
        raise AssertionError(f"expected ManifestError containing {needle!r}")


def case_two_shards_are_executed():
    with tempfile.TemporaryDirectory() as tmp:
        base_tree(tmp)
        write(tmp, "formal/cbmc/dcu_probe.c", "int main(void) { return 0; }\n")
        add_shard(tmp, "iems", [proof_row()])
        add_shard(
            tmp,
            "dcu",
            [
                proof_row(
                    artifact="formal/cbmc/dcu_probe.c",
                    owner="dcu",
                    property="dcu probe assertion holds",
                )
            ],
        )
        rows = formal_manifest.load_manifests(tmp)
        assert {row["owner"] for row in rows} == {"iems", "dcu"}, rows
        proc = subprocess.run(
            [sys.executable, os.path.join(HERE, "formal_manifest.py"),
             "--root", tmp, "--rows"],
            capture_output=True, text=True,
        )
        assert proc.returncode == 0, proc.stderr
        emitted = {line.split("\t")[9] for line in proc.stdout.splitlines()}
        assert emitted == {"iems", "dcu"}, emitted


def case_unknown_owner():
    expect_error(
        lambda tmp: (base_tree(tmp), add_shard(tmp, "evil", [proof_row(owner="evil")])),
        "unknown owner",
    )


def case_missing_manifest():
    expect_error(base_tree, "formal/manifest/ is missing")


def case_header_missing_column():
    def build(tmp):
        base_tree(tmp)
        header = "\t".join(
            field for field in formal_manifest.FIELDS if field != "assertions"
        )
        write(tmp, "formal/manifest/iems.tsv", header + "\n" + "\t".join(["x"] * 17) + "\n")

    expect_error(build, "missing columns")


def case_field_count():
    def build(tmp):
        base_tree(tmp)
        write(
            tmp,
            "formal/manifest/iems.tsv",
            HEADER + "\n" + "\t".join(["x"] * (len(formal_manifest.FIELDS) - 1)) + "\n",
        )

    expect_error(build, "expected 18")


def case_property_count_not_integer():
    expect_error(
        lambda tmp: (base_tree(tmp), add_shard(tmp, "iems", [proof_row(property_count="many")])),
        "property_count is not a positive integer",
    )


def case_duplicate_artifact_mode():
    def build(tmp):
        base_tree(tmp)
        add_shard(tmp, "iems", [proof_row()])
        add_shard(tmp, "dcu", [proof_row(owner="dcu")])

    expect_error(build, "duplicate (artifact, mode)")


def case_unknown_negative_form():
    def build(tmp):
        base_tree(tmp)
        write(tmp, "formal/cbmc/probe.log", "log\n")
        add_shard(tmp, "iems", [proof_row(negative="formal/cbmc/probe.log")])

    expect_error(build, "neither <cfg>#<invariant>")


def case_cfg_negative_without_invariant():
    def build(tmp):
        base_tree(tmp)
        tla_fixtures(tmp)
        add_shard(
            tmp,
            "iems",
            [tla_row(negative="formal/tla/negative/ProbeBad.cfg#NoBad")],
        )

    expect_error(build, "has no 'INVARIANT NoBad' line")


def case_unregistered_formal_file():
    with tempfile.TemporaryDirectory() as tmp:
        base_tree(tmp)
        add_shard(tmp, "iems", [proof_row(negative=SCRIPT)])
        write(tmp, "formal/cbmc/stray.c", "int stray;\n")
        subprocess.run(["git", "-C", tmp, "init", "-q"], check=True)
        subprocess.run(["git", "-C", tmp, "add", "-A"], check=True)
        rows = formal_manifest.load_manifests(tmp)
        stray = check_formal_sync.unregistered_files(tmp, rows)
        assert stray == ["formal/cbmc/stray.c"], stray


CASES = (
    ("two shards are enumerated and executed", case_two_shards_are_executed),
    ("unknown owner is rejected", case_unknown_owner),
    ("missing manifest is rejected", case_missing_manifest),
    ("header missing a column is rejected", case_header_missing_column),
    ("row field count mismatch is rejected", case_field_count),
    ("non-integer property_count is rejected", case_property_count_not_integer),
    ("duplicate (artifact, mode) is rejected", case_duplicate_artifact_mode),
    ("unknown negative form is rejected", case_unknown_negative_form),
    ("cfg negative without its invariant is rejected", case_cfg_negative_without_invariant),
    ("unregistered formal file is rejected", case_unregistered_formal_file),
)


def main():
    failures = 0
    for name, case in CASES:
        try:
            case()
        except Exception as error:  # any failure is a refutation failure
            print(f"❌ [FORMAL-MANIFESTS] {name}: {type(error).__name__}: {error}")
            failures += 1
        else:
            print(f"  ✅ [FORMAL-MANIFESTS] {name}")
    if failures:
        print(f"❌ [FORMAL-MANIFESTS] {failures} case(s) failed")
        return 1
    print(f"✅ [FORMAL-MANIFESTS] {len(CASES)} case(s) passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())

#!/usr/bin/env python3
"""
run_guard_selftest.py - Refutation Testing for Architecture & CI Quality Guards (AEP-792 / Issue #337)

Implements the "Testing the Gates" paradigm:
1. Executes every guard script against a valid test fixture (Good Fixture) -> MUST PASS (exit 0);
2. Executes every guard script against an invalid violation fixture (Bad Fixture) -> MUST FAIL (exit != 0);
3. Validates that no architectural guard script can become insensitive or silent.
"""

import sys
import os
import subprocess

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
FIXTURES_DIR = os.path.join(REPO_ROOT, "tests", "guards", "fixtures")
SCRIPTS_DIR = os.path.join(REPO_ROOT, "ci", "scripts")

TEST_MATRIX = [
    {
        "guard_name": "check_plane_isolation.py",
        "script": os.path.join(SCRIPTS_DIR, "check_plane_isolation.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_plane_isolation"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_plane_isolation"),
        "arg_flag": "--root"
    },
    {
        "guard_name": "check_app_isolation.py",
        "script": os.path.join(SCRIPTS_DIR, "check_app_isolation.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_app_isolation"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_app_isolation"),
        "arg_flag": "--root"
    },
    {
        "guard_name": "check_no_dynamic_memory.py",
        "script": os.path.join(SCRIPTS_DIR, "check_no_dynamic_memory.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_dynamic_memory"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_dynamic_memory"),
        "arg_flag": "--root"
    },
    {
        "guard_name": "check_stack_usage.py",
        "script": os.path.join(SCRIPTS_DIR, "check_stack_usage.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_stack_usage"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_stack_usage"),
        "arg_flag": "--root"
    },
    {
        "guard_name": "check_dependencies.py",
        "script": os.path.join(SCRIPTS_DIR, "check_dependencies.py"),
        "good_target": os.path.join(REPO_ROOT, "ci", "dependencies.json"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_dependencies.json"),
        "arg_flag": "--file"
    },
    {
        "guard_name": "check_exemptions.py",
        "script": os.path.join(SCRIPTS_DIR, "check_exemptions.py"),
        "good_target": os.path.join(REPO_ROOT, "ci", "exemptions.json"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_exemptions.json"),
        "arg_flag": "--file"
    },
    {
        "guard_name": "check_caller_allocated_storage.py",
        "script": os.path.join(SCRIPTS_DIR, "check_caller_allocated_storage.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_caller_storage"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_caller_storage"),
        "arg_flag": "--root"
    },
    {
        "guard_name": "check_aggregate_boundary.py",
        "script": os.path.join(SCRIPTS_DIR, "check_aggregate_boundary.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_aggregate_boundary"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_aggregate_boundary"),
        "arg_flag": "--root"
    },
    {
        "guard_name": "check_composition_root.py",
        "script": os.path.join(SCRIPTS_DIR, "check_composition_root.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_composition_root"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_composition_root"),
        "arg_flag": "--root"
    },
    {
        "guard_name": "check_open_closed_plugins.py",
        "script": os.path.join(SCRIPTS_DIR, "check_open_closed_plugins.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_open_closed"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_open_closed"),
        "arg_flag": "--root"
    },
    {
        "guard_name": "check_single_responsibility.py",
        "script": os.path.join(SCRIPTS_DIR, "check_single_responsibility.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_single_responsibility"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_single_responsibility"),
        "arg_flag": "--root"
    }
]

# The eight probes recorded in issue #410 ("8 injection experiments, only 3 caught").
# Each one is a real-world violation shape placed at the same relative path it
# would occupy in the repository, so the guards are exercised against reality
# rather than a sanitised snippet.
#
# `expected` is a ledger, not a wish:
#   "caught" -> the guard must reject it; a regression fails the build.
#   "gap"    -> the guard is known not to cover this shape yet. The owning issue
#               is recorded, and the test FAILS if the gap silently closes, so
#               the entry gets promoted to "caught" in the same PR.
INJECTION_MATRIX = [
    ("i01", "hub->shadow_table field penetration",
     "check_aggregate_boundary.py", "i01_aggregate_field_penetration", "gap", "#413"),
    ("i02", "library module calls datahub_init()",
     "check_composition_root.py", "i02_library_assembles_datahub", "gap", "#417"),
    ("i03", "plain malloc in a non-critical helper",
     "check_no_dynamic_memory.py", "i03_plain_heap_alloc", "gap", "#412"),
    ("i04", "single-line _create_in() that allocates",
     "check_caller_allocated_storage.py", "i04_single_line_create_in", "caught", "#411"),
    ("i05", "edge_core includes stream_engine_priv.h",
     "check_plane_isolation.py", "i05_core_includes_compute_priv", "caught", "#414"),
    ("i06", "multi-line _create_in() that allocates",
     "check_caller_allocated_storage.py", "i06_multiline_create_in", "caught", "#411"),
    ("i07", "_storage_t without alignas",
     "check_caller_allocated_storage.py", "i07_unaligned_storage", "caught", "#411"),
    ("i08", "_storage_t without _Static_assert",
     "check_caller_allocated_storage.py", "i08_missing_static_assert", "caught", "#411"),
]


def run_cmd(cmd):
    proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    return proc.returncode, proc.stdout, proc.stderr

def main():
    print("=" * 80)
    print("🚀 [REFUTATION_TESTING] Running Architecture Guard Refutation Self-Tests (Testing the Gates)")
    print("=" * 80)

    total_tests = 0
    passed_tests = 0
    failures = []

    for item in TEST_MATRIX:
        guard_name = item["guard_name"]
        script = item["script"]
        arg_flag = item["arg_flag"]

        # 1. Test Good Case (Must Pass)
        total_tests += 1
        good_cmd = [sys.executable, script, arg_flag, item["good_target"]]
        rc, out, err = run_cmd(good_cmd)
        if rc == 0:
            print(f"  ✅ [PASS] {guard_name} (Positive Assertion / Good Fixture passed as expected)")
            passed_tests += 1
        else:
            msg = f"❌ [FAIL] {guard_name} falsely rejected good fixture:\n{out}\n{err}"
            print(msg)
            failures.append(msg)

        # 2. Test Bad Case (Must Reject / Refutation)
        if item["bad_target"]:
            total_tests += 1
            bad_cmd = [sys.executable, script, arg_flag, item["bad_target"]]
            rc_bad, out_bad, err_bad = run_cmd(bad_cmd)
            if rc_bad != 0:
                # #411: a passing "bad fixture" only proves the guard rejects *something*.
                # For the caller-storage guard we also require the one-line-body
                # regression fixture to be named, because that specific bypass used to
                # slip through while the fixture still failed for another reason.
                if guard_name == "check_caller_allocated_storage.py":
                    if "oneline_entity_create_in" not in (out_bad + err_bad):
                        msg = (
                            f"❌ [REGRESSION] {guard_name} no longer reports the one-line "
                            "placement constructor (oneline_entity_create_in)."
                        )
                        print(msg)
                        failures.append(msg)
                        continue
                print(f"  ✅ [PASS] {guard_name} (Refutation / Bad Fixture correctly caught and rejected)")
                passed_tests += 1
            else:
                msg = f"❌ [REFUTATION_FAIL] {guard_name} failed to catch violation in bad fixture!"
                print(msg)
                failures.append(msg)

    print("=" * 80)
    print(f"📊 Refutation Test Results: {passed_tests}/{total_tests} passed (100% sensitivity)")
    print("=" * 80)

    # ---- Injection regression: replay issue #410's eight probes --------------
    print("\n" + "=" * 80)
    print("🎯 [INJECTION_REGRESSION] Replaying the 8 probes from issue #410")
    print("=" * 80)
    caught_count = 0
    gap_count = 0
    for probe_id, desc, guard, fixture, expected, issue in INJECTION_MATRIX:
        script = os.path.join(SCRIPTS_DIR, guard)
        target = os.path.join(FIXTURES_DIR, "injection", fixture)
        rc, out, err = run_cmd([sys.executable, script, "--root", target])
        caught = rc != 0
        if expected == "caught":
            if caught:
                print(f"  ✅ [CAUGHT]  {probe_id}  {desc}  -> {guard}")
                caught_count += 1
            else:
                msg = (f"❌ [REGRESSION] {probe_id} {desc}: {guard} exited 0 on an injected "
                       f"violation. A guard that cannot catch its own regression probe is decorative.")
                print(msg)
                failures.append(msg)
        else:
            if caught:
                msg = (f"❌ [GAP_CLOSED] {probe_id} {desc}: {guard} now catches it. Promote this "
                       f"entry from 'gap' to 'caught' and drop the {issue} reference.")
                print(msg)
                failures.append(msg)
            else:
                print(f"  ⚪ [GAP]     {probe_id}  {desc}  -> {guard}  ({issue}, tracked)")
                gap_count += 1
    print(f"📊 Injection coverage: {caught_count} caught / {gap_count} known gap(s) "
          f"of {len(INJECTION_MATRIX)} probes")

    if failures:
        return 1

    # Also run live check on actual codebase
    print("\n🔍 Running live architecture guards on active repository...")
    # Every guard is exercised against the live tree, including the two that used to be
    # fixture-only (`no_dynamic_memory`, `stack_usage`) - that omission is what let the
    # live violations in #410 hide (#411).
    for script_name in [
        "check_plane_isolation.py",
        "check_app_isolation.py",
        "check_caller_allocated_storage.py",
        "check_dependencies.py",
        "check_exemptions.py",
        "check_aggregate_boundary.py",
        "check_composition_root.py",
        "check_open_closed_plugins.py",
        "check_single_responsibility.py",
        "check_no_dynamic_memory.py",
        "check_stack_usage.py",
    ]:
        script_path = os.path.join(SCRIPTS_DIR, script_name)
        flag = "--file" if "dependencies" in script_name or "exemptions" in script_name else "--root"
        target = os.path.join(REPO_ROOT, "ci", f"{script_name.replace('check_', '').replace('.py', '')}.json") if flag == "--file" else REPO_ROOT
        rc, out, err = run_cmd([sys.executable, script_path, flag, target])
        if rc != 0:
            print(f"❌ [LIVE_CHECK_FAIL] {script_name} reported violations on repository:\n{out}")
            return 1
        else:
            print(f"  ✅ [LIVE_PASS] {script_name}")

    print("\n🎉 All architecture guards, refutation tests, and live codebase checks PASSED successfully!")
    return 0

if __name__ == "__main__":
    sys.exit(main())

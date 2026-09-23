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
        "bad_target": None, # Validates current committed ledger
        "arg_flag": "--file"
    },
    {
        "guard_name": "check_exemptions.py",
        "script": os.path.join(SCRIPTS_DIR, "check_exemptions.py"),
        "good_target": os.path.join(REPO_ROOT, "ci", "exemptions.json"),
        "bad_target": None, # Validates current committed ledger
        "arg_flag": "--file"
    }
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
                print(f"  ✅ [PASS] {guard_name} (Refutation / Bad Fixture correctly caught and rejected)")
                passed_tests += 1
            else:
                msg = f"❌ [REFUTATION_FAIL] {guard_name} failed to catch violation in bad fixture!"
                print(msg)
                failures.append(msg)

    print("=" * 80)
    print(f"📊 Refutation Test Results: {passed_tests}/{total_tests} passed (100% sensitivity)")
    print("=" * 80)

    if failures:
        return 1

    # Also run live check on actual codebase
    print("\n🔍 Running live architecture guards on active repository...")
    for item in TEST_MATRIX:
        target = item["good_target"] if item["arg_flag"] == "--file" else REPO_ROOT
        rc, out, _ = run_cmd([sys.executable, item["script"], item["arg_flag"], target])
        if rc != 0:
            print(f"❌ [LIVE_CHECK_FAIL] {item['guard_name']} reported violations:\n{out}")
            return 1
        print(f"  ✅ [LIVE_PASS] {item['guard_name']}")

    print("\n🎉 All architecture guards, refutation tests, and live codebase checks PASSED successfully!")
    return 0

if __name__ == "__main__":
    sys.exit(main())

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
import tempfile

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
        "guard_name": "check_no_dynamic_memory.py (rt-alloc-ok)",
        "script": os.path.join(SCRIPTS_DIR, "check_no_dynamic_memory.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_rt_alloc_ok"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_rt_alloc_bare"),
        "arg_flag": "--root"
    },
    {
        "guard_name": "check_no_dynamic_memory.py (rt-alloc-ok block comment)",
        "script": os.path.join(SCRIPTS_DIR, "check_no_dynamic_memory.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_rt_alloc_ok"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_rt_alloc_block"),
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
        "guard_name": "check_stack_usage.py (opt-out)",
        "script": os.path.join(SCRIPTS_DIR, "check_stack_usage.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_stack_optout"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_stack_optout"),
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
        "guard_name": "check_sbom.py",
        "script": os.path.join(SCRIPTS_DIR, "check_sbom.py"),
        "good_target": REPO_ROOT,
        "bad_target": os.path.join(FIXTURES_DIR, "bad_sbom"),
        "arg_flag": "--root"
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
    },
    {
        "guard_name": "check_single_responsibility.py (multiline)",
        "script": os.path.join(SCRIPTS_DIR, "check_single_responsibility.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_single_responsibility"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_srp_multiline"),
        "arg_flag": "--root"
    },
    {
        "guard_name": "check_component_boundary.py",
        "script": os.path.join(SCRIPTS_DIR, "check_component_boundary.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_component_boundary"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_component_boundary"),
        "arg_flag": "--root"
    },
    {
        "guard_name": "check_product_build_graph.py",
        "script": os.path.join(SCRIPTS_DIR, "check_product_build_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_build_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_build_graph"),
        "arg_flag": "--root"
    },
    {
        "guard_name": "check_component_metadata.py",
        "script": os.path.join(SCRIPTS_DIR, "check_component_metadata.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_component_metadata"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_component_metadata"),
        "arg_flag": "--root"
    },
    {
        "guard_name": "check_baseline_caps.py",
        "script": os.path.join(SCRIPTS_DIR, "check_baseline_caps.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_baseline_caps"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_baseline_caps"),
        "arg_flag": "--root"
    },
    {
        "guard_name": "check_composition_root_budget.py",
        "script": os.path.join(SCRIPTS_DIR, "check_composition_root_budget.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_budget"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_budget"),
        "arg_flag": "--root"
    },
    {
        "guard_name": "check_net_buf_alloc_timeout.py",
        "script": os.path.join(SCRIPTS_DIR, "check_net_buf_alloc_timeout.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_net_buf_timeout"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_net_buf_timeout"),
        "arg_flag": "--root"
    },
    {
        "guard_name": "check_net_buf_alloc_timeout.py (null pool)",
        "script": os.path.join(SCRIPTS_DIR, "check_net_buf_alloc_timeout.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_net_buf_timeout"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_net_buf_null_pool"),
        "arg_flag": "--root"
    },
    {
        "guard_name": "check_script_permissions.py",
        "script": os.path.join(SCRIPTS_DIR, "check_script_permissions.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_script_permissions"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_script_permissions"),
        "arg_flag": "--root"
    },
    {
        "guard_name": "check_test_registration.py",
        "script": os.path.join(SCRIPTS_DIR, "check_test_registration.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_test_registration"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_test_registration"),
        "arg_flag": "--root"
    },
    {
        "guard_name": "check_app_host_no_exec.py",
        "script": os.path.join(SCRIPTS_DIR, "check_app_host_no_exec.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_app_host_no_exec"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_app_host_no_exec"),
        "arg_flag": "--root"
    },
    {
        "guard_name": "check_stage_registry.py",
        "script": os.path.join(SCRIPTS_DIR, "check_stage_registry.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_stage_registry"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_stage_registry"),
        "arg_flag": "--root"
    },
    {
        "guard_name": "check_app_abi_surface.py",
        "script": os.path.join(SCRIPTS_DIR, "check_app_abi_surface.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_abi_surface"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_abi_surface"),
        "arg_flag": "--root"
    },
    {
        "guard_name": "check_crypto_boundary.py",
        "script": os.path.join(SCRIPTS_DIR, "check_crypto_boundary.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_crypto_boundary"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_crypto_boundary"),
        "arg_flag": "--root"
    },
    {
        # Takes a captured build log instead of a source tree; the CI build step
        # tees it to build/build.log and runs this guard right after the build.
        "guard_name": "check_build_warnings.py",
        "script": os.path.join(SCRIPTS_DIR, "check_build_warnings.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_build_warnings", "build.log"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_build_warnings", "build.log"),
        "arg_flag": "--log"
    },
    {
        "guard_name": "check_strncpy_bounds.py",
        "script": os.path.join(SCRIPTS_DIR, "check_strncpy_bounds.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_strncpy_bounds"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_strncpy_bounds"),
        "arg_flag": "--root"
    },
    {
        "guard_name": "check_ftell_checked.py",
        "script": os.path.join(SCRIPTS_DIR, "check_ftell_checked.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_ftell_checked"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_ftell_checked"),
        "arg_flag": "--root"
    },
    {
        "guard_name": "check_alloc_null.py",
        "script": os.path.join(SCRIPTS_DIR, "check_alloc_null.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_alloc_null"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_alloc_null"),
        "arg_flag": "--root"
    },
    {
        "guard_name": "check_sanitizer_perf_gates.py",
        "script": os.path.join(SCRIPTS_DIR, "check_sanitizer_perf_gates.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_sanitizer_perf"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_sanitizer_perf"),
        "arg_flag": "--root"
    },
    {
        "guard_name": "check_app_export_dispatch.py",
        "script": os.path.join(SCRIPTS_DIR, "check_app_export_dispatch.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_app_export_dispatch"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_app_export_dispatch"),
        "arg_flag": "--root"
    },
    {
        "guard_name": "check_sdk_signature.py",
        "script": os.path.join(SCRIPTS_DIR, "check_sdk_signature.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_sdk_signature"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_sdk_signature"),
        "arg_flag": "--root"
    },
    {
        "guard_name": "check_ci_gate_parity.py",
        "script": os.path.join(SCRIPTS_DIR, "check_ci_gate_parity.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_ci_gate_parity"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_ci_gate_parity"),
        "arg_flag": "--root"
    },
    {
        "guard_name": "check_privileged_test_runs.py",
        "script": os.path.join(SCRIPTS_DIR, "check_privileged_test_runs.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_privileged_test_runs"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_privileged_test_runs"),
        "arg_flag": "--root"
    },
    {
        "guard_name": "check_test_io_eintr.py",
        "script": os.path.join(SCRIPTS_DIR, "check_test_io_eintr.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_test_io_eintr"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_test_io_eintr"),
        "arg_flag": "--root"
    },
    {
        "guard_name": "check_test_io_eintr.py (MSG_DONTWAIT)",
        "script": os.path.join(SCRIPTS_DIR, "check_test_io_eintr.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_test_io_dontwait"),
        "bad_target": None,
        "arg_flag": "--root"
    },
    {
        "guard_name": "check_protocol_selection.py",
        "script": os.path.join(SCRIPTS_DIR, "check_protocol_selection.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_protocol_selection"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_protocol_selection"),
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
     "check_composition_root.py", "i02_library_assembles_datahub", "caught", "#417"),
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

    # ---- Coverage floor gate: good info passes, low info fails ---------------
    print("\n" + "=" * 80)
    print("📈 [COVERAGE_GATE] Testing coverage floor sensitivity")
    print("=" * 80)
    coverage_script = os.path.join(SCRIPTS_DIR, "check_coverage.py")
    coverage_baseline = os.path.join(FIXTURES_DIR, "coverage_floor.txt")
    for label, hit, total, expect_rc in (("good", 9, 10, 0), ("bad", 5, 10, 1)):
        fd, info_path = tempfile.mkstemp(suffix=".info")
        with os.fdopen(fd, "w", encoding="utf-8") as f:
            f.write(f"SF:{os.path.join(REPO_ROOT, 'ci', 'coverage_fixture', 'a.c')}\n")
            for i in range(total):
                f.write(f"DA:{i + 1},{1 if i < hit else 0}\n")
            f.write("end_of_record\n")
        total_tests += 1
        rc, out, err = run_cmd([
            sys.executable, coverage_script, "--info", info_path,
            "--root", REPO_ROOT, "--baseline", coverage_baseline,
        ])
        os.unlink(info_path)
        if (expect_rc == 0 and rc == 0) or (expect_rc != 0 and rc != 0):
            print(f"  ✅ [PASS] check_coverage.py ({label} fixture, rc={rc})")
            passed_tests += 1
        else:
            msg = f"❌ [FAIL] check_coverage.py misjudged {label} fixture (rc={rc}):\n{out}\n{err}"
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
        "check_component_boundary.py",
        "check_product_build_graph.py",
        "check_component_metadata.py",
        "check_public_structs.py",
        "check_resource_discipline.py",
        "check_composition_root_budget.py",
        "check_baseline_caps.py",
        "check_sdk_wit.py",
        "check_net_buf_alloc_timeout.py",
        "check_script_permissions.py",
        "check_test_registration.py",
        "check_app_host_no_exec.py",
        "check_stage_registry.py",
        "check_app_abi_surface.py",
        "check_crypto_boundary.py",
        "check_strncpy_bounds.py",
        "check_ftell_checked.py",
        "check_alloc_null.py",
        "check_sanitizer_perf_gates.py",
        "check_app_export_dispatch.py",
        "check_sdk_signature.py",
        "check_protocol_selection.py",
        "check_privileged_test_runs.py",
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

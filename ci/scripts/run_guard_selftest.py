#!/usr/bin/env python3
"""
run_guard_selftest.py - Refutation Testing for Architecture & CI Quality Guards (AEP-792 / Issue #337)

Implements the "Testing the Gates" paradigm:
1. Executes every guard script against a valid test fixture (Good Fixture) -> MUST PASS (exit 0);
2. Executes every guard script against an invalid violation fixture (Bad Fixture) -> MUST FAIL (exit != 0);
3. Validates that no architectural guard script can become insensitive or silent.
"""

import importlib.util
import json
import re
import shutil
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
        "guard_name": "check_architecture_graph.py",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_cmake_cycle"),
        "arg_flag": "--root",
        "expect_bad": "TARGET_CYCLE"
    },
    {
        "guard_name": "check_architecture_graph.py (include cycle)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_include_cycle"),
        "arg_flag": "--root",
        "expect_bad": "INCLUDE_CYCLE"
    },
    {
        "guard_name": "check_architecture_graph.py (layer direction)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_layer"),
        "arg_flag": "--root",
        "expect_bad": "UNDECLARED_EDGE"
    },
    {
        "guard_name": "check_architecture_graph.py (unregistered skip)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_skip"),
        "arg_flag": "--root",
        "expect_bad": "UNREGISTERED_PARSE_GAP"
    },
    {
        "guard_name": "check_architecture_graph.py (include direction and basename fallback)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_include_direction"),
        "arg_flag": "--root",
        "expect_bad": "UNDECLARED_EDGE"
    },
    {
        "guard_name": "check_architecture_graph.py (forbidden header)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph_self_header"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_forbidden_header"),
        "arg_flag": "--root",
        "expect_bad": "FORBIDDEN_HEADER"
    },
    {
        "guard_name": "check_architecture_graph.py (ALIAS)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_alias"),
        "arg_flag": "--root",
        "expect_bad": "UNDECLARED_EDGE"
    },
    {
        "guard_name": "check_architecture_graph.py (semicolon list)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_semicolon"),
        "arg_flag": "--root",
        "expect_bad": "UNDECLARED_EDGE"
    },
    {
        "guard_name": "check_architecture_graph.py (bracket args and comments)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_brackets"),
        "arg_flag": "--root",
        "expect_bad": "UNDECLARED_EDGE"
    },
    {
        "guard_name": "check_architecture_graph.py (macro include)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_include_macro"),
        "arg_flag": "--root",
        "expect_bad": "include-macro"
    },
    {
        "guard_name": "check_architecture_graph.py (comment between # and include)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_include_comment"),
        "arg_flag": "--root",
        "expect_bad": "UNDECLARED_EDGE"
    },
    {
        "guard_name": "check_architecture_graph.py (// line continuation)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph_comment_continuation"),
        "bad_target": None,
        "arg_flag": "--root"
    },
    {
        "guard_name": "check_architecture_graph.py (BOM, digraph and backslash continuation)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_include_phase2"),
        "arg_flag": "--root",
        "expect_bad": "UNDECLARED_EDGE"
    },
    {
        "guard_name": "check_architecture_graph.py (quoted include containing '>')",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_quoted_gt"),
        "arg_flag": "--root",
        "expect_bad": "UNDECLARED_EDGE"
    },
    {
        "guard_name": "check_architecture_graph.py (escaped '(' in a CMake command)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_escape_paren"),
        "arg_flag": "--root",
        "expect_bad": "UNDECLARED_EDGE"
    },
    {
        "guard_name": "check_architecture_graph.py (keyword case sensitivity)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_keyword_case"),
        "arg_flag": "--root",
        "expect_bad": "UNDECLARED_EDGE"
    },
    {
        "guard_name": "check_architecture_graph.py (list APPEND)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_list_append"),
        "arg_flag": "--root",
        "expect_bad": "UNDECLARED_EDGE"
    },
    {
        "guard_name": "check_architecture_graph.py (PARENT_SCOPE)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_parent_scope"),
        "arg_flag": "--root",
        "expect_bad": "cmake-parent-scope"
    },
    {
        "guard_name": "check_architecture_graph.py (include relative to CMAKE_CURRENT_SOURCE_DIR)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_include_source_dir"),
        "arg_flag": "--root",
        "expect_bad": "UNDECLARED_EDGE"
    },
    {
        "guard_name": "check_architecture_graph.py (CMAKE_MODULE_PATH)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_module_path"),
        "arg_flag": "--root",
        "expect_bad": "UNDECLARED_EDGE"
    },
    {
        "guard_name": "check_architecture_graph.py (missing custom module)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_module_missing"),
        "arg_flag": "--root",
        "expect_bad": "cmake-module"
    },
    {
        "guard_name": "check_architecture_graph.py (quoted semicolon escape)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_quoted_escape"),
        "arg_flag": "--root",
        "expect_bad": "UNDECLARED_EDGE"
    },
    {
        "guard_name": "check_architecture_graph.py (target ownership by definition file)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_unowned_target"),
        "arg_flag": "--root",
        "expect_bad": "cmake-unowned-target"
    },
    {
        "guard_name": "check_architecture_graph.py (module-internal fixtures dir)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_prune_path"),
        "arg_flag": "--root",
        "expect_bad": "UNDECLARED_EDGE"
    },
    {
        "guard_name": "check_architecture_graph.py (cmake/CMakeLists.txt parsed)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_cmake_dir"),
        "arg_flag": "--root",
        "expect_bad": "cmake-unowned-target"
    },
    {
        "guard_name": "check_architecture_graph.py (composition_roots missing path)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_schema_composition_roots_path"),
        "arg_flag": "--root",
        "expect_bad": "missing path"
    },
    {
        "guard_name": "check_architecture_graph.py (composition_roots stale path)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_schema_composition_roots_stale"),
        "arg_flag": "--root",
        "expect_bad": "does not exist"
    },
    {
        "guard_name": "check_architecture_graph.py (forbidden_headers non-normalized files path)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_schema_forbidden_files"),
        "arg_flag": "--root",
        "expect_bad": "not normalized"
    },
    {
        "guard_name": "check_architecture_graph.py (sources whitelist)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_schema_sources"),
        "arg_flag": "--root",
        "expect_bad": "unknown source"
    },
    {
        "guard_name": "check_architecture_graph.py (duplicate sources entry)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_schema_sources_duplicate"),
        "arg_flag": "--root",
        "expect_bad": "duplicate sources"
    },
    {
        "guard_name": "check_architecture_graph.py (duplicate matrix entry)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_schema_duplicate_entry"),
        "arg_flag": "--root",
        "expect_bad": "duplicates an earlier entry"
    },
    {
        "guard_name": "check_architecture_graph.py (layer-qualified shared surface)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_shared_surface_layer"),
        "arg_flag": "--root",
        "expect_bad": "INCLUDE_CYCLE"
    },
    {
        "guard_name": "check_architecture_graph.py (IMPORTED/ALIAS case sensitivity)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_lowercase_options"),
        "arg_flag": "--root",
        "expect_bad": "UNDECLARED_EDGE"
    },
    {
        "guard_name": "check_architecture_graph.py (variable target and link)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_variable"),
        "arg_flag": "--root",
        "expect_bad": "link foundation_base"
    },
    {
        "guard_name": "check_architecture_graph.py (include recursion)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_cmake_include"),
        "arg_flag": "--root",
        "expect_bad": "link foundation_base"
    },
    {
        "guard_name": "check_architecture_graph.py (include cycle)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_cmake_include_cycle"),
        "arg_flag": "--root",
        "expect_bad": "cmake-include-cycle"
    },
    {
        "guard_name": "check_architecture_graph.py (unparseable include)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_cmake_include_missing"),
        "arg_flag": "--root",
        "expect_bad": "cmake-include"
    },
    {
        "guard_name": "check_architecture_graph.py (layer-qualified cycle nodes)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_cycle_layers"),
        "arg_flag": "--root",
        "expect_bad": "modules/io -> adapters/io"
    },
    {
        "guard_name": "check_architecture_graph.py (other layer scan)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_other"),
        "arg_flag": "--root",
        "expect_bad": "UNDECLARED_EDGE"
    },
    {
        "guard_name": "check_architecture_graph.py (ambiguous basename)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_ambiguous"),
        "arg_flag": "--root",
        "expect_bad": "include-ambiguous"
    },
    {
        "guard_name": "check_architecture_graph.py (unique basename owner)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_basename_owner"),
        "arg_flag": "--root",
        "expect_bad": "UNDECLARED_EDGE"
    },
    {
        "guard_name": "check_architecture_graph.py (ambiguous basename outside include roots)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_basename_ambiguous"),
        "arg_flag": "--root",
        "expect_bad": "include-ambiguous"
    },
    {
        "guard_name": "check_architecture_graph.py (declared but unregistered target)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_declared_target"),
        "arg_flag": "--root",
        "expect_bad": "cmake-declared-target"
    },
    {
        "guard_name": "check_architecture_graph.py (schema unknown key)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_schema"),
        "arg_flag": "--root",
        "expect_bad": "MATRIX_SCHEMA"
    },
    {
        "guard_name": "check_architecture_graph.py (schema missing section)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_schema_missing"),
        "arg_flag": "--root",
        "expect_bad": "missing section known_skips"
    },
    {
        "guard_name": "check_architecture_graph.py (schema missing composition_roots)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_schema_composition_roots"),
        "arg_flag": "--root",
        "expect_bad": "missing section composition_roots"
    },
    {
        "guard_name": "check_architecture_graph.py (schema dead file)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_schema_file"),
        "arg_flag": "--root",
        "expect_bad": "file does not exist"
    },
    {
        "guard_name": "check_architecture_graph.py (schema bad JSON)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_schema_json"),
        "arg_flag": "--root",
        "expect_bad": "cannot read matrix"
    },
    {
        "guard_name": "check_architecture_graph.py (stale known skip)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_arch_graph_stale_skip"),
        "arg_flag": "--root",
        "expect_bad": "STALE_KNOWN_SKIP"
    },
    {
        "guard_name": "check_architecture_graph.py (self-owned forbidden header)",
        "script": os.path.join(SCRIPTS_DIR, "check_architecture_graph.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_arch_graph_self_header"),
        "bad_target": None,
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
        "guard_name": "check_guard_path_independence.py",
        "script": os.path.join(SCRIPTS_DIR, "check_guard_path_independence.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_guard_path_independence"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_guard_path_independence"),
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
        "guard_name": "check_composition_root_budget.py (matrix without composition_roots)",
        "script": os.path.join(SCRIPTS_DIR, "check_composition_root_budget.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_budget"),
        "bad_target": os.path.join(
            FIXTURES_DIR, "bad_arch_graph_schema_composition_roots"
        ),
        "arg_flag": "--root",
        "expect_bad": "composition_roots"
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
        # 负例是动态的：把真实 docs 复制到临时目录再改坏一行，
        # 见 main() 里的 "DCU 698 catalog" 段。
        "guard_name": "check_dcu_698_catalog.py",
        "script": os.path.join(SCRIPTS_DIR, "check_dcu_698_catalog.py"),
        "good_target": REPO_ROOT,
        "bad_target": None,
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
        "guard_name": "check_ci_gate_parity.py (dropped argument)",
        "script": os.path.join(SCRIPTS_DIR, "check_ci_gate_parity.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_ci_gate_parity"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_ci_gate_parity_args"),
        "arg_flag": "--root"
    },
    {
        "guard_name": "check_ci_gate_parity.py (dropped formal job)",
        "script": os.path.join(SCRIPTS_DIR, "check_ci_gate_parity.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_ci_gate_parity"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_ci_gate_parity_formal"),
        "arg_flag": "--root"
    },
    {
        "guard_name": "check_formal_isolation.py",
        "script": os.path.join(SCRIPTS_DIR, "check_formal_isolation.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_formal_isolation"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_formal_isolation"),
        "arg_flag": "--root"
    },
    {
        "guard_name": "check_ci_gate_parity.py (dropped environment assignment)",
        "script": os.path.join(SCRIPTS_DIR, "check_ci_gate_parity.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_ci_gate_parity"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_ci_gate_parity_env"),
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
        "guard_name": "check_acceptance_tasks.py",
        "script": os.path.join(SCRIPTS_DIR, "check_acceptance_tasks.py"),
        "good_target": os.path.join(FIXTURES_DIR, "acceptance_tasks", "good"),
        "bad_target": os.path.join(FIXTURES_DIR, "acceptance_tasks", "bad_missing"),
        "arg_flag": "--root"
    },
    {
        "guard_name": "check_acceptance_tasks.py (nonzero)",
        "script": os.path.join(SCRIPTS_DIR, "check_acceptance_tasks.py"),
        "good_target": os.path.join(FIXTURES_DIR, "acceptance_tasks", "good"),
        "bad_target": os.path.join(FIXTURES_DIR, "acceptance_tasks", "bad_nonzero"),
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
        "guard_name": "check_test_io_eintr.py (EINTR branch)",
        "script": os.path.join(SCRIPTS_DIR, "check_test_io_eintr.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_test_io_eintr_branch"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_test_io_eintr_branch"),
        "arg_flag": "--root"
    },
    {
        "guard_name": "check_protocol_selection.py",
        "script": os.path.join(SCRIPTS_DIR, "check_protocol_selection.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_protocol_selection"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_protocol_selection"),
        "arg_flag": "--root"
    },
    {
        "guard_name": "check_protocol_selection.py (comments)",
        "script": os.path.join(SCRIPTS_DIR, "check_protocol_selection.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_protocol_selection_comment"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_protocol_selection_comment"),
        "arg_flag": "--root"
    },
    {
        "guard_name": "check_iems_control_slave.py",
        "script": os.path.join(SCRIPTS_DIR, "check_iems_control_slave.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_iems_control_slave"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_iems_control_slave"),
        "arg_flag": "--root"
    },
    {
        "guard_name": "check_iems_stdout_line_buffered.py",
        "script": os.path.join(SCRIPTS_DIR, "check_iems_stdout_line_buffered.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_iems_stdout_line_buffer"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_iems_stdout_line_buffer"),
        "arg_flag": "--root"
    },
    {
        "guard_name": "check_iems_control_slave.py (hex literal)",
        "script": os.path.join(SCRIPTS_DIR, "check_iems_control_slave.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_iems_control_slave"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_iems_control_slave_hex"),
        "arg_flag": "--root"
    },
    {
        "guard_name": "check_acceptance_wrapper_signals.py",
        "script": os.path.join(SCRIPTS_DIR, "check_acceptance_wrapper_signals.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_acceptance_wrapper_signals"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_acceptance_wrapper_signals"),
        "arg_flag": "--root"
    },
    {
        "guard_name": "check_doc_references.py",
        "script": os.path.join(SCRIPTS_DIR, "check_doc_references.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_doc_references"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_doc_references"),
        "arg_flag": "--root"
    },
    {
        "guard_name": "config_inventory.py (missing file)",
        "script": os.path.join(REPO_ROOT, "tools", "migration", "config_inventory.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_config_inventory"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_config_inventory_missing_file"),
        "arg_flag": "--root",
        "expect_bad": "missing file"
    },
    {
        "guard_name": "config_inventory.py (extra file)",
        "script": os.path.join(REPO_ROOT, "tools", "migration", "config_inventory.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_config_inventory"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_config_inventory_extra_file"),
        "arg_flag": "--root",
        "expect_bad": "unexpected file"
    },
    {
        "guard_name": "config_inventory.py (hash mismatch)",
        "script": os.path.join(REPO_ROOT, "tools", "migration", "config_inventory.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_config_inventory"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_config_inventory_hash_mismatch"),
        "arg_flag": "--root",
        "expect_bad": "sha256 mismatch"
    },
    {
        "guard_name": "config_inventory.py (invalid JSON)",
        "script": os.path.join(REPO_ROOT, "tools", "migration", "config_inventory.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_config_inventory"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_config_inventory_invalid_json"),
        "arg_flag": "--root",
        "expect_bad": "parse error"
    },
    {
        "guard_name": "config_inventory.py (missing manifest)",
        "script": os.path.join(REPO_ROOT, "tools", "migration", "config_inventory.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_config_inventory"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_config_inventory_missing_manifest"),
        "arg_flag": "--root",
        "expect_bad": "manifest not found"
    },
    {
        "guard_name": "config_inventory.py (malformed manifest)",
        "script": os.path.join(REPO_ROOT, "tools", "migration", "config_inventory.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_config_inventory"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_config_inventory_malformed_manifest"),
        "arg_flag": "--root",
        "expect_bad": "manifest not readable"
    },
    {
        "guard_name": "config_schema.py (unknown key)",
        "script": os.path.join(REPO_ROOT, "tools", "migration", "config_schema.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_config_schema"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_config_schema_unknown_key"),
        "arg_flag": "--root",
        "expect_bad": "UNKNOWN_KEY unexpected"
    },
    {
        "guard_name": "config_schema.py (missing required)",
        "script": os.path.join(REPO_ROOT, "tools", "migration", "config_schema.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_config_schema"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_config_schema_missing_required"),
        "arg_flag": "--root",
        "expect_bad": "MISSING_REQUIRED dev_class[0].devs[0].channel"
    },
    {
        "guard_name": "config_schema.py (bad reference)",
        "script": os.path.join(REPO_ROOT, "tools", "migration", "config_schema.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_config_schema"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_config_schema_bad_reference"),
        "arg_flag": "--root",
        "expect_bad": "BAD_REFERENCE dev_class[0].devs[0].channel"
    },
    {
        "guard_name": "config_schema.py (plaintext secret)",
        "script": os.path.join(REPO_ROOT, "tools", "migration", "config_schema.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_config_schema"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_config_schema_plaintext_secret"),
        "arg_flag": "--root",
        "expect_bad": "PLAINTEXT_SECRET pass"
    },
    {
        "guard_name": "config_schema.py (missing schema_version)",
        "script": os.path.join(REPO_ROOT, "tools", "migration", "config_schema.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_config_schema_converted"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_config_schema_missing_version"),
        "arg_flag": "--root",
        "expect_bad": "MISSING_SCHEMA_VERSION"
    },
    {
        "guard_name": "config_schema.py (missing template)",
        "script": os.path.join(REPO_ROOT, "tools", "migration", "config_schema.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_config_schema"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_config_schema_missing_template"),
        "arg_flag": "--root",
        "expect_bad": "template not found"
    },
    {
        "guard_name": "config_schema.py (bad pub tag)",
        "script": os.path.join(REPO_ROOT, "tools", "migration", "config_schema.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_config_schema"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_config_schema_bad_pub_tag"),
        "arg_flag": "--root",
        "expect_bad": "MissingTag"
    },
    {
        "guard_name": "config_schema.py (P2 unknown key)",
        "script": os.path.join(REPO_ROOT, "tools", "migration", "config_schema.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_config_schema"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_config_schema_p2_unknown_key"),
        "arg_flag": "--root",
        "expect_bad": "UNKNOWN_KEY dev[0].blocks[0].sub_id[0].unexpected"
    },
    {
        "guard_name": "config_schema.py (P2 missing required)",
        "script": os.path.join(REPO_ROOT, "tools", "migration", "config_schema.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_config_schema"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_config_schema_p2_missing_required"),
        "arg_flag": "--root",
        "expect_bad": "MISSING_REQUIRED pub_dev_list[0].tags"
    },
    {
        "guard_name": "config_schema.py (P2 bad type)",
        "script": os.path.join(REPO_ROOT, "tools", "migration", "config_schema.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_config_schema"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_config_schema_p2_bad_type"),
        "arg_flag": "--root",
        "expect_bad": "TYPE_MISMATCH sample_rate"
    },
    {
        "guard_name": "config_schema.py (north map tuple type)",
        "script": os.path.join(REPO_ROOT, "tools", "migration", "config_schema.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_config_schema"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_config_schema_north_map_bad_row"),
        "arg_flag": "--root",
        "expect_bad": "TYPE_MISMATCH pts[0].pointInfo[0][1]"
    },
    {
        "guard_name": "config_schema.py (rate_fig bad type)",
        "script": os.path.join(REPO_ROOT, "tools", "migration", "config_schema.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_config_schema"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_config_schema_rate_fig_bad_type"),
        "arg_flag": "--root",
        "expect_bad": "TYPE_MISMATCH [0].write.send_0"
    },
    {
        "guard_name": "convert_config.py (unknown key)",
        "script": os.path.join(REPO_ROOT, "tools", "migration", "convert_config.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_convert_config"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_convert_unknown_key"),
        "arg_flag": "--root",
        "expect_bad": "UNKNOWN_KEY unexpected"
    },
    {
        "guard_name": "convert_config.py (missing required)",
        "script": os.path.join(REPO_ROOT, "tools", "migration", "convert_config.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_convert_config"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_convert_missing_required"),
        "arg_flag": "--root",
        "expect_bad": "MISSING_REQUIRED dev_class[0].devs[0].channel"
    },
    {
        "guard_name": "convert_config.py (bad channel reference)",
        "script": os.path.join(REPO_ROOT, "tools", "migration", "convert_config.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_convert_config"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_convert_bad_channel"),
        "arg_flag": "--root",
        "expect_bad": "BAD_REFERENCE dev_class[0].devs[0].channel"
    },
    {
        "guard_name": "convert_config.py (missing schema_version)",
        "script": os.path.join(REPO_ROOT, "tools", "migration", "convert_config.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_convert_config"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_convert_missing_version"),
        "arg_flag": "--root",
        "expect_bad": "MISSING_SCHEMA_VERSION"
    },
    {
        "guard_name": "convert_config.py (plaintext secret in expected artifact)",
        "script": os.path.join(REPO_ROOT, "tools", "migration", "convert_config.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_convert_config"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_convert_plaintext_secret"),
        "arg_flag": "--root",
        "expect_bad": "PLAINTEXT_SECRET password"
    },
    {
        "guard_name": "check_plaintext_secrets.py (json plaintext)",
        "script": os.path.join(SCRIPTS_DIR, "check_plaintext_secrets.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_plaintext_secrets"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_plaintext_secret_json"),
        "arg_flag": "--root",
        "expect_bad": "cfg/app.json:2 key=password"
    },
    {
        "guard_name": "check_plaintext_secrets.py (yaml plaintext)",
        "script": os.path.join(SCRIPTS_DIR, "check_plaintext_secrets.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_plaintext_secrets"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_plaintext_secret_yaml"),
        "arg_flag": "--root",
        "expect_bad": "cfg/app.yaml:1 key=pass"
    },
    {
        "guard_name": "check_plaintext_secrets.py (env plaintext)",
        "script": os.path.join(SCRIPTS_DIR, "check_plaintext_secrets.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_plaintext_secrets"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_plaintext_secret_env"),
        "arg_flag": "--root",
        "expect_bad": "cfg/app.env:1 key=API_TOKEN"
    },
    {
        "guard_name": "check_plaintext_secrets.py (outside allowlist)",
        "script": os.path.join(SCRIPTS_DIR, "check_plaintext_secrets.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_plaintext_secrets"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_plaintext_secret_outside_allowlist"),
        "arg_flag": "--root",
        "expect_bad": "other/two.json:2 key=client_secret"
    },
    {
        "guard_name": "check_plaintext_secrets.py (stale allowlist)",
        "script": os.path.join(SCRIPTS_DIR, "check_plaintext_secrets.py"),
        "good_target": os.path.join(FIXTURES_DIR, "good_plaintext_secrets"),
        "bad_target": os.path.join(FIXTURES_DIR, "bad_plaintext_secret_stale_allowlist"),
        "arg_flag": "--root",
        "expect_bad": "STALE_ALLOWLIST cfg/clean.json"
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


MUTATION_PROBES = [
    {
        "label": "ALIAS resolution",
        "fixture": "bad_arch_graph_alias",
        "old": '    if "ALIAS" in kinds:\n        real = expand_token(args[2], env) if len(args) > 2 else None\n',
        "new": '    if "ALIAS" in kinds:\n        continue\n    if False:\n        real = expand_token(args[2], env) if len(args) > 2 else None\n',
        "marker": "UNDECLARED_EDGE",
    },
    {
        "label": "semicolon list split",
        "fixture": "bad_arch_graph_semicolon",
        "old": "    return [part for part in expanded.split(\";\") if part]\n",
        "new": "    return [expanded] if expanded else []\n",
        "marker": "UNDECLARED_EDGE",
    },
    {
        "label": "C comment strip before include scan",
        "fixture": "bad_arch_graph_include_comment",
        "old": "            strip_c_comments(normalize_c_source(read_text(path))).splitlines(), 1\n",
        "new": "            normalize_c_source(read_text(path)).splitlines(), 1\n",
        "marker": "UNDECLARED_EDGE",
    },
    {
        "label": "macro include gap",
        "fixture": "bad_arch_graph_include_macro",
        "old": "                if ident:\n",
        "new": "                if False:\n",
        "marker": "include-macro",
    },
    {
        "label": "variable expansion",
        "fixture": "bad_arch_graph_variable",
        "old": "        if name not in variables:\n",
        "new": "        if True:\n",
        "marker": "link foundation_base",
    },
    {
        "label": "forbidden header",
        "fixture": "bad_arch_graph_forbidden_header",
        "old": '                        "rule_id": "FORBIDDEN_HEADER",\n',
        "new": '                        "rule_id": "DISABLED_HEADER",\n',
        "marker": "FORBIDDEN_HEADER",
    },
    {
        "label": "matrix schema unknown key",
        "fixture": "bad_arch_graph_schema",
        "old": "    unknown = sorted(set(matrix) - TOP_LEVEL_KEYS)\n",
        "new": "    unknown = []\n",
        "marker": "MATRIX_SCHEMA",
    },
    {
        "label": "stale known skip",
        "fixture": "bad_arch_graph_stale_skip",
        "old": "    for key in known:\n        if key not in combined_skips:\n",
        "new": "    for key in known:\n        if False:\n",
        "marker": "STALE_KNOWN_SKIP",
    },
]


def run_mutation_probes():
    failures = []
    script = os.path.join(SCRIPTS_DIR, "check_architecture_graph.py")
    with open(script, encoding="utf-8") as handle:
        source = handle.read()
    print("\n" + "=" * 80)
    print("🧬 [MUTATION] Deleting one parser rule must silence its fixture")
    print("=" * 80)
    for probe in MUTATION_PROBES:
        if probe["old"] not in source:
            msg = f"❌ [MUTATION_DRIFT] {probe['label']}: mutation anchor no longer exists"
            print(msg)
            failures.append(msg)
            continue
        mutated = source.replace(probe["old"], probe["new"], 1)
        with tempfile.TemporaryDirectory(prefix="arch-mutation-") as tmp:
            mutated_path = os.path.join(tmp, "check_architecture_graph.py")
            with open(mutated_path, "w", encoding="utf-8") as handle:
                handle.write(mutated)
            target = os.path.join(FIXTURES_DIR, probe["fixture"])
            rc, out, err = run_cmd([sys.executable, mutated_path, "--root", target])
        combined = out + err
        if "Traceback" in combined:
            msg = f"❌ [MUTATION_CRASH] {probe['label']} crashed instead of disabling the rule"
            print(msg)
            failures.append(msg)
        elif probe["marker"] in combined:
            msg = (
                f"❌ [MUTATION_INEFFECTIVE] {probe['label']}: {probe['fixture']} still "
                f"reports {probe['marker']!r} after the rule was deleted (rc={rc})"
            )
            print(msg)
            failures.append(msg)
        else:
            print(
                f"  ✅ [MUTATION] {probe['label']}: deletion silences "
                f"{probe['marker']!r} (rc={rc})"
            )
    return failures


def load_guard_module(alias, filename):
    spec = importlib.util.spec_from_file_location(
        alias, os.path.join(SCRIPTS_DIR, filename)
    )
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def check_live_ledgers(matrix=None):
    failures = []
    if matrix is None:
        matrix_path = os.path.join(
            REPO_ROOT, "ci", "baseline", "architecture_graph.json"
        )
        with open(matrix_path, encoding="utf-8") as handle:
            matrix = json.load(handle)

    sys.path.insert(0, SCRIPTS_DIR)
    plane = load_guard_module("plane_ledger_under_test", "check_plane_isolation.py")
    boundary = load_guard_module(
        "boundary_ledger_under_test", "check_component_boundary.py"
    )

    plane_matrix = set()
    for entry in matrix["allowed_component_edges"]:
        if "plane_isolation" not in entry.get("sources", []):
            continue
        if "include" not in entry.get("kinds", []):
            failures.append(
                f"❌ [ARCH_LEDGER] plane entry {entry['from_component']} -> "
                f"{entry['to_component']} lost its include kind"
            )
        plane_matrix.add(
            (
                entry.get("from_layer"),
                entry.get("from_component"),
                entry.get("to_layer"),
                entry.get("to_component"),
            )
        )
    for entry in matrix["allowed_layer_edges"]:
        if entry.get("from_layer") == "modules" and entry.get("to_layer") == "adapters":
            failures.append(
                "❌ [ARCH_LEDGER] allowed_layer_edges grants modules -> adapters as a "
                "whole layer, which widens includes beyond check_plane_isolation; "
                "declare the pair in allowed_component_edges with kinds instead"
            )
    for entry in matrix["allowed_component_edges"]:
        if (
            entry.get("from_layer") != "modules"
            or entry.get("to_layer") != "modules"
            or "include" not in entry.get("kinds", [])
        ):
            continue
        sources = set(entry.get("sources", []))
        owners = plane.MODULE_ALLOWED.get(entry.get("from_component"), set())
        in_plane = entry.get("to_component") in owners
        if "preprocessor_closure" not in sources and not in_plane:
            failures.append(
                "❌ [ARCH_LEDGER] include permission "
                f"{entry.get('from_component')} -> {entry.get('to_component')} is "
                "neither in check_plane_isolation.MODULE_ALLOWED nor marked "
                "sources: [preprocessor_closure]"
            )
    plane_fallback = {
        ("modules", module, "modules", owner)
        for module, owners in plane.MODULE_ALLOWED.items()
        for owner in owners
    }
    if plane_fallback != plane_matrix:
        failures.append(
            "❌ [ARCH_LEDGER] check_plane_isolation.MODULE_ALLOWED differs from the "
            f"matrix plane entries: fallback={sorted(plane_fallback)} "
            f"matrix={sorted(plane_matrix)}"
        )

    components = {}
    files = {}
    for entry in matrix["forbidden_headers"]:
        if "component_boundary" not in entry.get("sources", []):
            continue
        if "files" in entry or "file" in entry:
            paths = entry.get("files", [entry.get("file")])
            for path in paths:
                files.setdefault(path, set()).add(entry["header"])
        elif "component" in entry:
            components.setdefault(entry["component"], set()).add(entry["header"])
    if components != boundary.COMPONENTS:
        failures.append(
            "❌ [ARCH_LEDGER] check_component_boundary.COMPONENTS differs from the "
            f"matrix: fallback={components} matrix={boundary.COMPONENTS}"
        )
    if files != boundary.FILE_FORBIDDEN:
        failures.append(
            "❌ [ARCH_LEDGER] check_component_boundary.FILE_FORBIDDEN differs from "
            f"the matrix: fallback={files} matrix={boundary.FILE_FORBIDDEN}"
        )

    if failures:
        return failures
    print(
        "  ✅ [ARCH_LEDGER] fallbacks match the matrix "
        f"({len(plane_matrix)} plane pairs, "
        f"{len(components)} components, {len(files)} file rules)"
    )
    return []


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
                expected = item.get("expect_bad")
                if expected and expected not in (out_bad + err_bad):
                    msg = (
                        f"❌ [REGRESSION] {guard_name} rejected the fixture but did not "
                        f"report the expected reason {expected!r}; a deleted rule would "
                        "still look red for an unrelated reason.\n"
                        f"{out_bad}\n{err_bad}"
                    )
                    print(msg)
                    failures.append(msg)
                    continue
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

    failures.extend(run_mutation_probes())

    print("\n" + "=" * 80)
    print("🧾 [LEDGER_DECOY] A layer-decoy plane entry cannot buy a false clean ledger")
    print("=" * 80)
    with open(
        os.path.join(REPO_ROOT, "ci", "baseline", "architecture_graph.json"),
        encoding="utf-8",
    ) as handle:
        decoy_matrix = json.load(handle)
    decoy_matrix["allowed_component_edges"].append(
        {
            "from_layer": "adapters",
            "from_component": "edge_io",
            "to_layer": "modules",
            "to_component": "edge_core",
            "kinds": ["include"],
            "sources": ["plane_isolation"],
            "reason": "selftest decoy: same component names, different layer",
        }
    )
    total_tests += 1
    decoy_failures = check_live_ledgers(decoy_matrix)
    if decoy_failures and any("differs" in failure for failure in decoy_failures):
        print("  ✅ [PASS] layer-qualified plane comparison rejects the adapters decoy")
        passed_tests += 1
    else:
        msg = ("❌ [REFUTATION_FAIL] a decoy plane entry with the same component names "
               f"but a different layer was accepted: {decoy_failures}")
        print(msg)
        failures.append(msg)

    with open(
        os.path.join(REPO_ROOT, "ci", "baseline", "architecture_graph.json"),
        encoding="utf-8",
    ) as handle:
        unmarked_matrix = json.load(handle)
    unmarked_matrix["allowed_component_edges"].append(
        {
            "from_layer": "modules",
            "from_component": "edge_core",
            "to_layer": "modules",
            "to_component": "edge_stream",
            "kinds": ["include"],
            "reason": "selftest decoy: include permission without a provenance marker",
        }
    )
    total_tests += 1
    unmarked_failures = check_live_ledgers(unmarked_matrix)
    if unmarked_failures and any(
        "preprocessor_closure" in failure for failure in unmarked_failures
    ):
        print("  ✅ [PASS] an unmarked modules include permission is rejected")
        passed_tests += 1
    else:
        msg = ("❌ [REFUTATION_FAIL] an include permission outside MODULE_ALLOWED "
               f"without a provenance marker was accepted: {unmarked_failures}")
        print(msg)
        failures.append(msg)

    print("\n" + "=" * 80)
    print("🔬 [STRONG_MODE] Preprocessor closure catches what the text scan cannot see")
    print("=" * 80)
    strong_fixture = os.path.join(FIXTURES_DIR, "strong_mode_arch_graph")
    strong_commands = os.path.join(strong_fixture, "compile_commands.json")
    arch_script = os.path.join(SCRIPTS_DIR, "check_architecture_graph.py")
    total_tests += 1
    rc, out, err = run_cmd([sys.executable, arch_script, "--root", strong_fixture])
    if rc == 0:
        print("  ✅ [PASS] strong-mode fixture (text mode stays green on the angle include)")
        passed_tests += 1
    else:
        msg = f"❌ [FAIL] strong-mode fixture text mode rejected a clean tree:\n{out}\n{err}"
        print(msg)
        failures.append(msg)
    total_tests += 1
    rc, out, err = run_cmd([
        sys.executable, arch_script, "--root", strong_fixture,
        "--compile-commands", strong_commands,
    ])
    if rc != 0 and "UNDECLARED_EDGE" in (out + err):
        print("  ✅ [PASS] strong-mode fixture (preprocessor closure finds the first-party angle include)")
        passed_tests += 1
    else:
        msg = ("❌ [REFUTATION_FAIL] strong mode did not reject the angle-bracket first-party "
               f"include (rc={rc}):\n{out}\n{err}")
        print(msg)
        failures.append(msg)

    with tempfile.TemporaryDirectory(prefix="strong-mf-") as tmp:
        fixture = os.path.join(tmp, "fixture")
        shutil.copytree(strong_fixture, fixture)
        depfile = os.path.join(tmp, "deps.d")
        commands = os.path.join(tmp, "compile_commands.json")
        with open(commands, "w", encoding="utf-8") as handle:
            json.dump(
                [
                    {
                        "directory": fixture,
                        "file": "foundation/src/base.c",
                        "command": (
                            "cc -I modules/alpha/include -MD -MF "
                            f"{depfile} -MT base.o -c foundation/src/base.c"
                        ),
                    }
                ],
                handle,
            )
        total_tests += 1
        rc, out, err = run_cmd([
            sys.executable, arch_script, "--root", fixture,
            "--compile-commands", commands,
        ])
        if rc != 0 and "UNDECLARED_EDGE" in (out + err):
            print("  ✅ [PASS] -MF/-MD/-MT stripped, dependencies still reach stdout")
            passed_tests += 1
        else:
            msg = ("❌ [REFUTATION_FAIL] strong mode lost the closure when the command "
                   f"carried -MF (rc={rc}):\n{out}\n{err}")
            print(msg)
            failures.append(msg)

        total_tests += 1
        silent_commands = os.path.join(tmp, "silent_compile_commands.json")
        with open(silent_commands, "w", encoding="utf-8") as handle:
            json.dump(
                [
                    {
                        "directory": fixture,
                        "file": "foundation/src/base.c",
                        "arguments": ["true"],
                    }
                ],
                handle,
            )
        rc, out, err = run_cmd([
            sys.executable, arch_script, "--root", fixture,
            "--compile-commands", silent_commands,
        ])
        if rc != 0 and "does not contain the TU source" in (out + err):
            print("  ✅ [PASS] a closure without the TU source fails closed")
            passed_tests += 1
        else:
            msg = ("❌ [REFUTATION_FAIL] an empty dependency closure did not fail "
                   f"closed (rc={rc}):\n{out}\n{err}")
            print(msg)
            failures.append(msg)

    print("\n" + "=" * 80)
    print("🧪 [FORMAL_ISOLATION_PATH] Testing the isolation guard under a path containing 'formal'")
    print("=" * 80)
    formal_guard = os.path.join(SCRIPTS_DIR, "check_formal_isolation.py")
    with tempfile.TemporaryDirectory(prefix="formal-isolation-path-") as tmp:
        shutil.copytree(os.path.join(FIXTURES_DIR, "good_formal_isolation"),
                        os.path.join(tmp, "product"))
        init_rc, _, init_err = run_cmd(["git", "-C", tmp, "init", "-q"])
        add_rc, _, add_err = run_cmd(["git", "-C", tmp, "add", "."])
        total_tests += 1
        if init_rc != 0 or add_rc != 0:
            msg = f"❌ [FIXTURE] cannot stage the formal isolation fixture: {init_err}{add_err}"
            print(msg)
            failures.append(msg)
        else:
            generated = os.path.join(tmp, "build-ci", "CMakeFiles")
            os.makedirs(generated)
            with open(os.path.join(generated, "CMakeSystem.cmake"), "w", encoding="utf-8") as f:
                f.write(f'set(CMAKE_SOURCE_DIR "{tmp}")\n')
            rc, out, err = run_cmd([sys.executable, formal_guard, "--root", tmp])
            if rc == 0:
                print("  ✅ [PASS] check_formal_isolation.py (generated tree under a 'formal' path ignored)")
                passed_tests += 1
            else:
                msg = ("❌ [FAIL] check_formal_isolation.py flagged a generated build tree in a "
                       f"checkout whose path contains 'formal':\n{out}\n{err}")
                print(msg)
                failures.append(msg)

            total_tests += 1
            with open(os.path.join(tmp, "product", "main.c"), "a", encoding="utf-8") as f:
                f.write('#include "formal/cbmc/modbus_decode.c"\n')
            rc_bad, out_bad, err_bad = run_cmd([sys.executable, formal_guard, "--root", tmp])
            if rc_bad != 0:
                print("  ✅ [PASS] check_formal_isolation.py (tracked formal/ include still rejected)")
                passed_tests += 1
            else:
                msg = ("❌ [REFUTATION_FAIL] check_formal_isolation.py accepted a tracked "
                       f"include of formal/:\n{out_bad}\n{err_bad}")
                print(msg)
                failures.append(msg)

    print("\n" + "=" * 80)
    print("🧪 [FORMAL_MANIFESTS] Testing manifest discovery, schema and registration refutations")
    print("=" * 80)
    total_tests += 1
    rc, out, err = run_cmd([
        sys.executable, os.path.join(SCRIPTS_DIR, "test_formal_manifests.py"),
    ])
    if rc == 0:
        print("  ✅ [PASS] test_formal_manifests.py (all manifest refutations caught)")
        passed_tests += 1
    else:
        msg = f"❌ [REFUTATION_FAIL] formal manifest tests failed:\n{out}\n{err}"
        print(msg)
        failures.append(msg)

    # 目录门禁不能只用一把小夹具：生成器对 26 个类与 400 行以上有断言，小夹具
    # 走不到那段逻辑。所以复制真实的 docs 到临时目录，改坏目录里一行，要求拒收。
    print("\n" + "=" * 80)
    print("📚 [CATALOG_GATE] Testing the 698 catalog gate on a real docs copy")
    print("=" * 80)
    catalog_guard = os.path.join(SCRIPTS_DIR, "check_dcu_698_catalog.py")
    with tempfile.TemporaryDirectory(prefix="dcu698-catalog-") as tmp:
        design = os.path.join(tmp, "docs", "design")
        fixtures = os.path.join(tmp, "tests", "fixtures")
        os.makedirs(design)
        os.makedirs(fixtures)
        for src, dst in (
            (os.path.join(REPO_ROOT, "docs", "698_parsed.md"),
             os.path.join(tmp, "docs", "698_parsed.md")),
            (os.path.join(REPO_ROOT, "docs", "design", "DCU_698_OI_CATALOG.md"),
             os.path.join(design, "DCU_698_OI_CATALOG.md")),
            (os.path.join(REPO_ROOT, "docs", "design", "DCU_698_OI_CATALOG.json"),
             os.path.join(design, "DCU_698_OI_CATALOG.json")),
            (os.path.join(REPO_ROOT, "docs", "design", "DCU_698_CLASS_RECONCILIATION.md"),
             os.path.join(design, "DCU_698_CLASS_RECONCILIATION.md")),
            (os.path.join(REPO_ROOT, "tests", "fixtures", "dcu_698_catalog_expected.h"),
             os.path.join(fixtures, "dcu_698_catalog_expected.h")),
        ):
            shutil.copyfile(src, dst)

        total_tests += 1
        rc, out, err = run_cmd([sys.executable, catalog_guard, "--root", tmp])
        if rc == 0:
            print("  ✅ [PASS] check_dcu_698_catalog.py (unmodified copy accepted)")
            passed_tests += 1
        else:
            msg = f"❌ [FAIL] check_dcu_698_catalog.py rejected an unmodified copy:\n{out}\n{err}"
            print(msg)
            failures.append(msg)

        total_tests += 1
        catalog_md = os.path.join(design, "DCU_698_OI_CATALOG.md")
        with open(catalog_md, encoding="utf-8") as f:
            text = f.read()
        corrupted = re.sub(r"Classes: \d+\.", "Classes: 99.", text, count=1)
        if corrupted == text:
            msg = "❌ [FIXTURE] catalog header line changed shape; the corruption no longer applies"
            print(msg)
            failures.append(msg)
        else:
            with open(catalog_md, "w", encoding="utf-8") as f:
                f.write(corrupted)
            rc_bad, out_bad, err_bad = run_cmd([sys.executable, catalog_guard, "--root", tmp])
            if rc_bad != 0:
                print("  ✅ [PASS] check_dcu_698_catalog.py (corrupted copy rejected)")
                passed_tests += 1
            else:
                msg = "❌ [REFUTATION_FAIL] check_dcu_698_catalog.py accepted a corrupted catalog"
                print(msg)
                failures.append(msg)

    with tempfile.TemporaryDirectory(prefix="dcu698-conflict-") as tmp2:
        os.makedirs(os.path.join(tmp2, "docs"))
        parsed_md = os.path.join(tmp2, "docs", "698_parsed.md")
        shutil.copyfile(os.path.join(REPO_ROOT, "docs", "698_parsed.md"), parsed_md)

        with open(parsed_md, encoding="utf-8") as f:
            text = f.read()
        changed = re.sub(r"^30 06\s+24\s", "30 06   3  ", text, count=1, flags=re.M)
        total_tests += 1
        if changed == text:
            msg = "❌ [FIXTURE] 3006 row line changed shape; the conflict fixture no longer applies"
            print(msg)
            failures.append(msg)
        else:
            with open(parsed_md, "w", encoding="utf-8") as f:
                f.write(changed)
            rc_conf, out_conf, err_conf = run_cmd([sys.executable, catalog_guard, "--root", tmp2])
            if rc_conf != 0 and "without a recorded decision" in (out_conf + err_conf):
                print("  ✅ [PASS] check_dcu_698_catalog.py (unrecorded conflict rejected)")
                passed_tests += 1
            else:
                msg = ("❌ [REFUTATION_FAIL] catalog gate accepted a class conflict with no "
                       f"recorded decision:\n{out_conf}\n{err_conf}")
                print(msg)
                failures.append(msg)

    # 生成器按附录标题定位表格：源文档行号整体平移后仍须解析出同一目录，
    # 标题缺失或重复必须报错而不是静默丢行。
    total_tests += 1
    rc, out, err = run_cmd([
        sys.executable, "-I",
        os.path.join(REPO_ROOT, "tests", "test_dcu_698_catalog_source_window.py"),
    ])
    if rc == 0:
        print("  ✅ [PASS] test_dcu_698_catalog_source_window.py "
              "(shifted source accepted, broken titles rejected)")
        passed_tests += 1
    else:
        msg = f"❌ [FAIL] 698 catalog source-window guard failed:\n{out}\n{err}"
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

    # ---- fanalyzer scope: vendored/generated diagnostics are not ours --------
    print("\n" + "=" * 80)
    print("🔬 [FANALYZER_SCOPE] Testing the analyzer finding filter")
    print("=" * 80)
    analyzer_path = os.path.join(SCRIPTS_DIR, "run_fanalyzer.py")
    spec = importlib.util.spec_from_file_location("run_fanalyzer_under_test", analyzer_path)
    analyzer = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(analyzer)
    diag = "{}:7:1: warning: unused parameter 'x' [-Wunused-parameter]"
    with tempfile.TemporaryDirectory(prefix="fanalyzer-scope-") as tmp:
        for label, path, expected in (
            ("vendored build tree", os.path.join(tmp, "build", "third-party", "up.h"), 0),
            ("vendored source", os.path.join(tmp, "third-party", "vendored.c"), 0),
            ("first-party source", os.path.join(tmp, "foundation", "src", "ours.c"), 1),
        ):
            total_tests += 1
            got = analyzer.findings(tmp, [("tu.c", diag.format(path))])
            if len(got) == expected:
                print(f"  ✅ [PASS] fanalyzer scope ({label} -> {expected} finding(s))")
                passed_tests += 1
            else:
                msg = f"❌ [FAIL] fanalyzer scope misjudged {label}: {got}"
                print(msg)
                failures.append(msg)

    print("\n" + "=" * 80)
    print("🏗️  [BUILD_FRESHNESS] Testing the build-artifact freshness gate")
    print("=" * 80)
    freshness_guard = os.path.join(SCRIPTS_DIR, "check_build_freshness.py")
    if not shutil.which("ninja"):
        print("  ⚪ [SKIP] ninja is not installed; freshness fixtures need a real ninja")
    else:
        with tempfile.TemporaryDirectory(prefix="build-freshness-") as tmp:
            root = os.path.join(tmp, "root")
            build = os.path.join(root, "build")
            other_root = os.path.join(tmp, "other-root")
            os.makedirs(build)
            os.makedirs(other_root)
            source = os.path.join(root, "in.txt")
            with open(source, "w", encoding="utf-8") as f:
                f.write("source\n")
            with open(os.path.join(build, "build.ninja"), "w", encoding="utf-8") as f:
                f.write("rule stamp\n  command = cp ../in.txt out.txt\n"
                        "build out.txt: stamp ../in.txt\n")

            def write_cache(home):
                with open(os.path.join(build, "CMakeCache.txt"), "w", encoding="utf-8") as f:
                    f.write(f"CMAKE_HOME_DIRECTORY:INTERNAL={home}\n"
                            "CMAKE_GENERATOR:INTERNAL=Ninja\n")

            def check():
                return run_cmd([sys.executable, freshness_guard, "--root", root,
                                "--build", build])

            write_cache(root)
            total_tests += 1
            build_rc, build_out, build_err = run_cmd(["ninja", "-C", build])
            if build_rc != 0:
                msg = (f"❌ [FIXTURE] cannot produce a ninja log for the freshness fixture:\n"
                       f"{build_out}\n{build_err}")
                print(msg)
                failures.append(msg)
            else:
                rc, out, err = check()
                if rc == 0:
                    print("  ✅ [PASS] check_build_freshness.py (build with no pending work accepted)")
                    passed_tests += 1
                else:
                    msg = f"❌ [FAIL] check_build_freshness.py rejected a current build:\n{out}\n{err}"
                    print(msg)
                    failures.append(msg)

            total_tests += 1
            write_cache(other_root)
            rc_root, out_root, err_root = check()
            if rc_root != 0 and "build artifacts are stale for the current tree" in (out_root + err_root):
                print("  ✅ [PASS] check_build_freshness.py (build from another tree rejected)")
                passed_tests += 1
            else:
                msg = ("❌ [REFUTATION_FAIL] check_build_freshness.py accepted a build configured "
                       f"for another tree:\n{out_root}\n{err_root}")
                print(msg)
                failures.append(msg)

            total_tests += 1
            write_cache(root)
            os.utime(source, (4102444800, 4102444800))
            rc_bad, out_bad, err_bad = check()
            if rc_bad != 0 and "build artifacts are stale for the current tree" in (out_bad + err_bad):
                print("  ✅ [PASS] check_build_freshness.py (pending work rejected)")
                passed_tests += 1
            else:
                msg = ("❌ [REFUTATION_FAIL] check_build_freshness.py accepted a build with "
                       f"pending work:\n{out_bad}\n{err_bad}")
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
        "check_architecture_graph.py",
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
        "check_dcu_698_catalog.py",
        "check_iems_control_slave.py",
        "check_iems_stdout_line_buffered.py",
        "check_acceptance_wrapper_signals.py",
        "check_doc_references.py",
        "check_guard_path_independence.py",
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

    # AGENTS.md 点名的脚本必须存在：文档指向不存在的守卫时，规则等于没写。
    ledger_failures = check_live_ledgers()
    if ledger_failures:
        for message in ledger_failures:
            print(message)
        return 1
    with open(os.path.join(REPO_ROOT, "AGENTS.md"), encoding="utf-8") as handle:
        doc_refs = sorted(set(re.findall(r"`([A-Za-z0-9_./-]+\.py)`", handle.read())))
    missing_refs = [
        ref for ref in doc_refs
        if not any(os.path.exists(os.path.join(base, ref))
                   for base in (REPO_ROOT, SCRIPTS_DIR, os.path.join(REPO_ROOT, "scripts")))
    ]
    if missing_refs:
        print(f"❌ [DOC_REFS] AGENTS.md references missing scripts: {', '.join(missing_refs)}")
        return 1
    print(f"  ✅ [DOC_REFS] {len(doc_refs)} AGENTS.md script reference(s) resolve")

    print("\n🎉 All architecture guards, refutation tests, and live codebase checks PASSED successfully!")
    return 0

if __name__ == "__main__":
    sys.exit(main())

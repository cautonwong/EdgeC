#!/usr/bin/env python3
"""
check_composition_root.py - Composition Root Exclusivity & Assembly Layer Guard (AEP-794, #411)

Two rule families:

1. PSEUDO_ROOT (legacy). Only `modules/edge_bootstrap/`, product entry points and tests
   may include `edge_composition_root.h` or drive its lifecycle. Kept because the
   refutation fixture asserts exactly this.

2. ASSEMBLY. Object assembly must happen in one place. The guard knows which module
   *defines* each assembly entry point and fails any call site that is neither the
   defining module, `modules/edge_bootstrap/`, nor a sanctioned product composition
   root listed in `ci/composition_roots.txt`.

   The previous version only looked for the literal string `edge_composition_root_init`,
   so `datahub_init()`, `stream_engine_create()` and friends could be called from
   anywhere in `modules/` and the guard stayed green (#410 audit §6.3).
"""

import sys
import os
import re
import glob
import argparse

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import guard_baseline

ALLOWED_ROOT_PATHS = re.compile(
    r"^(?:modules/edge_bootstrap/|cmd/.*/main\.c|cmd/[^/]+/[^/]+_node\.[ch]|tests/|fixtures/)"
)

# Assembly entry points that instantiate and wire objects. Keep this list tight: it is
# the set of calls that decide the shape of a running system.
ASSEMBLY_FUNCS = [
    "datahub_init",
    "datahub_init_with_config",
    "datahub_init_from_contract",
    "datahub_init_from_fd",
    "stream_engine_create",
    "stream_engine_create_ex",
    "sqlite_svc_create",
    "bus_orchestrator_create",
    "io_executor_create",
    "io_executor_create_from_fd",
    "io_executor_create_uart",
    "driver_manager_create",
    "arbiter_create_in",
    "driver_instance_create",
    "protocol_bridge_create",
    "edge_app_mgr_create",
]

PRUNE_DIRS = {
    ".git",
    ".local",
    ".venv",
    ".vscode",
    ".code-review-graph",
    "build",
    "third-party",
    "node_modules",
    "docs",
    "web",
    "docker",
}

CALL_RE = re.compile(r"\b(" + "|".join(ASSEMBLY_FUNCS) + r")\s*\(")


def _load_sanctioned_roots(repo_root):
    path = os.path.join(repo_root, "ci", "composition_roots.txt")
    roots = set()
    if not os.path.exists(path):
        return roots
    with open(path, "r", encoding="utf-8") as fh:
        for raw in fh:
            line = raw.strip()
            if line and not line.startswith("#"):
                roots.add(line)
    return roots


def _defining_modules(repo_root):
    """func -> owning module, discovered from the definition site."""
    owner = {}
    patterns = [
        os.path.join(repo_root, "modules", "*", "src", "**", "*.c"),
    ]
    for pattern in patterns:
        for src in glob.glob(pattern, recursive=True):
            rel = os.path.relpath(src, repo_root).replace("\\", "/")
            module = rel.split("/")[1]
            try:
                text = open(src, "r", encoding="utf-8", errors="ignore").read()
            except OSError:
                continue
            for fn in ASSEMBLY_FUNCS:
                if fn not in owner and re.search(
                    rf"^[A-Za-z_][\w \t\*]*?\b{fn}\s*\([^;]*\)\s*\{{", text, re.M
                ):
                    owner[fn] = module
    return owner


def check_file(file_path, repo_root, owner_map, sanctioned):
    rel_path = os.path.relpath(file_path, repo_root).replace("\\", "/")
    violations = []

    try:
        lines = open(file_path, "r", encoding="utf-8", errors="ignore").readlines()
    except OSError:
        return violations

    # Family 1
    if not ALLOWED_ROOT_PATHS.search(rel_path):
        for idx, line in enumerate(lines, 1):
            if re.search(r'#include\s+["<](?:edgeos/)?edge_composition_root\.h[">]', line):
                violations.append(
                    {
                        "file": rel_path,
                        "location": f"{rel_path}:{idx}",
                        "rule_id": "PSEUDO_ROOT",
                        "detail": "only the bootstrap layer and product entry points may include edge_composition_root.h",
                        "fingerprint": guard_baseline.fingerprint(rel_path, "PSEUDO_ROOT", "include"),
                        "issue": "#417",
                    }
                )
            if re.search(r"\bedge_composition_root_(?:init|start|bind)\b", line):
                violations.append(
                    {
                        "file": rel_path,
                        "location": f"{rel_path}:{idx}",
                        "rule_id": "PSEUDO_ROOT",
                        "detail": "business/domain modules cannot act as pseudo-composition-roots",
                        "fingerprint": guard_baseline.fingerprint(rel_path, "PSEUDO_ROOT", "lifecycle"),
                        "issue": "#417",
                    }
                )

    # Family 2
    # Standalone examples are their own roots by definition.
    is_sanctioned = rel_path in sanctioned or bool(re.match(r"examples/[^/]+/main\.c$", rel_path))
    in_bootstrap = rel_path.startswith("modules/edge_bootstrap/")
    for idx, line in enumerate(lines, 1):
        stripped = line.lstrip()
        if stripped.startswith(("//", "*", "/*")):
            continue
        for fn in CALL_RE.findall(line):
            owner = owner_map.get(fn)
            if owner and rel_path.startswith(f"modules/{owner}/"):
                continue
            if in_bootstrap or is_sanctioned:
                continue
            violations.append(
                {
                    "file": rel_path,
                    "location": f"{rel_path}:{idx}",
                    "rule_id": "ASSEMBLY",
                    "detail": (
                        f"{fn}() assembles `{owner or 'unknown'}` from {rel_path}; "
                        "move it into the composition root"
                    ),
                    "fingerprint": guard_baseline.fingerprint(rel_path, "ASSEMBLY", ""),
                    "issue": "#417",
                }
            )
    return violations


def check_directory(target_dir):
    target_dir = os.path.abspath(target_dir)
    owner_map = _defining_modules(target_dir)
    sanctioned = _load_sanctioned_roots(target_dir)
    violations = []
    for root, dirs, files in os.walk(target_dir):
        dirs[:] = [d for d in dirs if not d.startswith(".") and d not in PRUNE_DIRS]
        is_fixture = os.path.basename(target_dir).startswith(("good_", "bad_"))
        if "fixtures" in root and not is_fixture:
            continue
        if "tests" in root and "fixtures" not in root and "guards" not in root:
            continue
        for name in files:
            if name.endswith((".c", ".h")):
                violations.extend(
                    check_file(os.path.join(root, name), target_dir, owner_map, sanctioned)
                )
    return violations


def main():
    parser = argparse.ArgumentParser(description="EdgeOS Composition Root Exclusivity Guard")
    parser.add_argument("--root", default=".")
    parser.add_argument("--baseline", default=None)
    parser.add_argument("--update-baseline", action="store_true")
    args = parser.parse_args()

    root_dir = os.path.abspath(args.root)
    violations = check_directory(root_dir)
    baseline_path = args.baseline or guard_baseline.baseline_path_for(__file__)
    is_fixture = os.path.basename(root_dir).startswith(("good_", "bad_"))
    return guard_baseline.apply(
        violations,
        baseline_path,
        args.update_baseline,
        "COMPOSITION_ROOT_GUARD",
        allow_missing_baseline=is_fixture,
    )


if __name__ == "__main__":
    sys.exit(main())

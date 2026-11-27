#!/usr/bin/env python3
"""P5-03d c1 classification gate: strategy-classification.tsv vs the contract.

The table is the single machine-readable source. This gate holds the
independent expectation derived from CONTRACT §1.2 (13 v3 forms and their
entry symbols), CONTRACT §4.3 (period bounds), CONTRACT §4.4 (6 classes) and
ADR-001 §2.2 (families, app quotas, safety families), plus the ticket's
single-source owners. It rejects any drift in kind/class/family/safety/
period/owner, whitespace or non-token kinds, any safety family or over-quota
row targeted at app, missing rows, duplicate decision keys (after trimming),
a coordinator acting as a TOU/demand decision source, and stale doc counts.

--selftest replays the A2 negative cases in memory and requires every one of
them to be rejected.
"""

import argparse
import copy
import os
import re
import sys

COLUMNS = [
    "form",
    "kind",
    "class",
    "adr_family",
    "target",
    "effective",
    "role",
    "period_ms",
    "decision_owner",
    "execution_owner",
    "safety",
    "points_per_cycle",
    "events_per_s",
    "subscriptions",
    "notes",
]

APP_FAMILIES = {
    "TOU",
    "DEMAND_FIXED",
    "DEMAND_DYNAMIC",
    "ANTI_REFLUX",
    "PV_PRIORITY",
    "SOC_KEEP",
    "PF",
    "EV_CAPACITY",
}
MODULE_FAMILIES = {
    "FIRE",
    "BMS_CLUSTER",
    "PCS_COMMAND",
    "LOOP_10MS",
    "PROTECTION",
    "OVER_CAPACITY",
}
SAFETY_FAMILY_VALUE = {
    "FIRE": "fire",
    "PROTECTION": "protection",
    "BMS_CLUSTER": "bms_cluster",
    "PCS_COMMAND": "pcs_command",
    "ANTI_REFLUX": "anti_reflux_limit",
    "LOOP_10MS": "loop_10ms",
}
SAFETY_LABELS = set(SAFETY_FAMILY_VALUE.values()) | {"none", "capacity"}
CLASSES = {"TOU", "DEMAND", "LIMIT", "PF", "SOC", "EV"}

# form -> independent audit expectation. kind is the CONTRACT §1.2 entry
# symbol of that row; period_ms bounds come from CONTRACT §4.3 (demand_dynamic
# keeps §1.2's 500ms control period under §4.3's 1000ms window ceiling; §1.2
# supplies the rows §4.3 has no line for). Owners follow the ticket's
# single-source rule (TOU/demand = iems_td_evaluate, coordinator executes only)
# and the planned CONTRACT §4.4/§4.7 symbols. This duplicates the TSV on
# purpose: the TSV is the working table, EXPECTED is the copy a drift must
# contradict.
EXPECTED = {
    "tou_current_ac": {
        "kind": "ac_Cutpeak_Fillvalley",
        "class": "TOU",
        "adr_family": "TOU",
        "safety": "none",
        "period_ms": (500, 500),
        "decision_owner": "iems_td_evaluate",
        "execution_owner": "iems_microgrid_coordinator",
    },
    "tou_legacy_loop": {
        "kind": "ems_stratagy_period_ctrl",
        "class": "TOU",
        "adr_family": "TOU",
        "safety": "none",
        "period_ms": (500, 500),
        "decision_owner": "iems_td_evaluate",
        "execution_owner": "iems_microgrid_coordinator",
    },
    "demand_fixed": {
        "kind": "ems_demand_ctrl",
        "class": "DEMAND",
        "adr_family": "DEMAND_FIXED",
        "safety": "none",
        "period_ms": (500, 500),
        "decision_owner": "iems_td_evaluate",
        "execution_owner": "iems_microgrid_coordinator",
    },
    "demand_dynamic": {
        "kind": "demand_control_handler",
        "class": "DEMAND",
        "adr_family": "DEMAND_DYNAMIC",
        "safety": "none",
        "period_ms": (500, 1000),
        "decision_owner": "iems_td_evaluate",
        "execution_owner": "iems_microgrid_coordinator",
    },
    "anti_reflux_limit": {
        "kind": "ems_anti_backflow_with_pv_handler",
        "class": "LIMIT",
        "adr_family": "ANTI_REFLUX",
        "safety": "anti_reflux_limit",
        "period_ms": (200, 200),
        "decision_owner": "iems_microgrid_coordinator",
        "execution_owner": "iems_microgrid_coordinator",
    },
    "pf_loop": {
        "kind": "pf_ctrl_thread",
        "class": "PF",
        "adr_family": "PF",
        "safety": "none",
        "period_ms": (1000, 10000),
        "decision_owner": "iems_strategy_pf",
        "execution_owner": "iems_microgrid_coordinator",
    },
    "soc_keep": {
        "kind": "Strategy_SocKeep_Check",
        "class": "SOC",
        "adr_family": "SOC_KEEP",
        "safety": "none",
        "period_ms": (500, 500),
        "decision_owner": "iems_soc_keep_evaluate",
        "execution_owner": "iems_microgrid_coordinator",
    },
    "pcs_low_soc_protect": {
        "kind": "Pcs_ctrl",
        "class": "SOC",
        "adr_family": "PROTECTION",
        "safety": "protection",
        "period_ms": (0, 0),
        "decision_owner": "iems_pcs_controller",
        "execution_owner": "iems_pcs_controller",
    },
    "ev_capacity": {
        "kind": "ChargingPilesControl_Management",
        "class": "EV",
        "adr_family": "EV_CAPACITY",
        "safety": "none",
        "period_ms": (500, 500),
        "decision_owner": "iems_strategy_ev_capacity",
        "execution_owner": "iems_microgrid_coordinator",
    },
    "ev_allocation": {
        "kind": "evhub_ctrl_loop",
        "class": "EV",
        "adr_family": "OVER_CAPACITY",
        "safety": "capacity",
        "period_ms": (2000, 2000),
        "decision_owner": "iems_ev_allocator",
        "execution_owner": "iems_ev_allocator",
    },
    "pv_storage_self_use": {
        "kind": "ac_Photovol_Absorp",
        "class": "TOU",
        "adr_family": "PV_PRIORITY",
        "safety": "none",
        "period_ms": (500, 500),
        "decision_owner": "iems_td_evaluate",
        "execution_owner": "iems_microgrid_coordinator",
    },
    "standby_backup": {
        "kind": "ac_Meet_Standby",
        "class": "SOC",
        "adr_family": "SOC_KEEP",
        "safety": "none",
        "period_ms": (500, 500),
        "decision_owner": "iems_strategy_group_step",
        "execution_owner": "iems_microgrid_coordinator",
    },
    "pv_priority": {
        "kind": "ems_pv_ctrl",
        "class": "TOU",
        "adr_family": "PV_PRIORITY",
        "safety": "none",
        "period_ms": (1000, 1000),
        "decision_owner": "iems_strategy_pv",
        "execution_owner": "iems_microgrid_coordinator",
    },
}

TOU_OWNER = "iems_td_evaluate"
TOU_DEMAND_FORMS = {
    form for form, expected in EXPECTED.items() if expected["decision_owner"] == TOU_OWNER
}
TOU_DEMAND_CLASSES = {"TOU", "DEMAND"}
COORDINATOR_OWNER = "iems_microgrid_coordinator"
LEGAL_KIND_RE = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")

APP_QUOTA = {"period_ms": 100, "points_per_cycle": 100, "events_per_s": 1000, "subscriptions": 32}

MODULE_TARGET_FAMILIES = MODULE_FAMILIES | {"ANTI_REFLUX"}
APP_TARGET_FAMILIES = APP_FAMILIES - {"ANTI_REFLUX"}

ROLE_BY_FAMILY = {"ANTI_REFLUX": "LIMIT", "SOC_KEEP": "OVERRIDE", "PROTECTION": "OVERRIDE"}

DOC_COUNT_RE = re.compile(r"forms=(\d+)\s+classes=(\d+)\s+adr_families=(\d+)")


def load_table(path):
    with open(path, encoding="utf-8") as handle:
        lines = [line.rstrip("\r\n") for line in handle]
    lines = [line for line in lines if line.strip() and not line.lstrip().startswith("#")]
    if not lines:
        return None, [], [("SCHEMA", "table is empty")]
    header = lines[0].split("\t")
    if header != COLUMNS:
        return header, [], [
            ("SCHEMA", f"header mismatch: {header!r}, expected {COLUMNS!r}")
        ]
    rows = []
    failures = []
    for index, line in enumerate(lines[1:], start=2):
        cells = line.split("\t")
        if len(cells) != len(COLUMNS):
            failures.append(
                ("SCHEMA", f"line {index}: {len(cells)} columns, expected {len(COLUMNS)}")
            )
            continue
        rows.append(dict(zip(COLUMNS, cells)))
    return header, rows, failures


def read_doc_counts(md_path):
    try:
        with open(md_path, encoding="utf-8") as handle:
            match = DOC_COUNT_RE.search(handle.read())
    except OSError:
        return None
    if match is None:
        return None
    return tuple(int(value) for value in match.groups())


def as_int(row, name, failures):
    text = row.get(name, "")
    try:
        value = int(text)
    except ValueError:
        failures.append(("NOT_INT", f"{row.get('form', '?')}: {name}={text!r} is not an integer"))
        return None
    return value


def validate(rows, doc_counts):
    failures = []

    forms = [row["form"] for row in rows]
    kinds = [row["kind"].strip() for row in rows]
    if len(rows) != len(EXPECTED):
        failures.append(("ROW_COUNT", f"{len(rows)} rows, expected {len(EXPECTED)}"))
    missing = sorted(set(EXPECTED) - set(forms))
    if missing:
        failures.append(("MISSING_FORM", f"missing forms: {', '.join(missing)}"))
    unknown = sorted(set(forms) - set(EXPECTED))
    if unknown:
        failures.append(("UNKNOWN_FORM", f"unknown forms: {', '.join(unknown)}"))
    duplicates = sorted({form for form in forms if forms.count(form) > 1})
    if duplicates:
        failures.append(("DUPLICATE_FORM", f"duplicate forms: {', '.join(duplicates)}"))
    duplicate_kinds = sorted({kind for kind in kinds if kinds.count(kind) > 1})
    if duplicate_kinds:
        failures.append(
            ("DOUBLE_SOURCE", f"one decision key on two rows: {', '.join(duplicate_kinds)}")
        )

    for row in rows:
        form = row["form"]
        raw_kind = row["kind"]
        if raw_kind != raw_kind.strip() or any(ch.isspace() for ch in raw_kind):
            failures.append(
                ("KIND_WHITESPACE", f"{form}: kind={raw_kind!r} carries whitespace")
            )
        if not LEGAL_KIND_RE.match(raw_kind):
            failures.append(
                ("BAD_KIND", f"{form}: kind={raw_kind!r} is not a legal C-identifier token")
            )
        expected = EXPECTED.get(form)
        if expected is None:
            continue
        expected_class = expected["class"]
        expected_family = expected["adr_family"]
        expected_safety = expected["safety"]
        if raw_kind.strip() != expected["kind"]:
            failures.append(
                (
                    "WRONG_KIND",
                    f"{form}: kind={row['kind']!r}, expected {expected['kind']} "
                    "(CONTRACT 1.2)",
                )
            )
        if row["class"] != expected_class:
            failures.append(
                ("WRONG_CLASS", f"{form}: class={row['class']}, expected {expected_class}")
            )
        if row["adr_family"] != expected_family:
            failures.append(
                ("WRONG_FAMILY", f"{form}: adr_family={row['adr_family']}, expected {expected_family}")
            )
        if row["safety"] != expected_safety:
            failures.append(
                ("SAFETY_LABEL", f"{form}: safety={row['safety']}, expected {expected_safety}")
            )

        if row["class"] not in CLASSES:
            failures.append(("UNKNOWN_CLASS", f"{form}: class={row['class']} not in CONTRACT 4.4"))
        if row["adr_family"] not in APP_FAMILIES | MODULE_FAMILIES:
            failures.append(("UNKNOWN_FAMILY", f"{form}: adr_family={row['adr_family']} not in ADR 2.2"))
        if row["target"] not in {"app", "module"}:
            failures.append(("BAD_TARGET", f"{form}: target={row['target']}"))
        if row["effective"] not in {"app", "module"}:
            failures.append(("BAD_EFFECTIVE", f"{form}: effective={row['effective']}"))
        if row["role"] not in {"DECIDE", "LIMIT", "OVERRIDE"}:
            failures.append(("BAD_ROLE", f"{form}: role={row['role']}"))
        if row["safety"] not in SAFETY_LABELS:
            failures.append(("BAD_SAFETY", f"{form}: safety={row['safety']}"))
        if row["effective"] != "module":
            failures.append(
                ("EFFECTIVE_MODULE", f"{form}: effective={row['effective']}; c1 must not switch targets")
            )

        family = row["adr_family"]
        if family in MODULE_TARGET_FAMILIES and row["target"] != "module":
            code = "SAFETY_MODULE" if family in SAFETY_FAMILY_VALUE else "MODULE_FAMILY"
            failures.append((code, f"{form}: {family} must target module, got {row['target']}"))
        if family in APP_TARGET_FAMILIES and row["target"] != "app":
            failures.append(
                ("APP_TARGET", f"{form}: {family} is app-eligible, target={row['target']}")
            )
        if row["safety"] != "none" and row["target"] != "module":
            failures.append(
                ("SAFETY_MODULE", f"{form}: safety={row['safety']} must target module")
            )

        expected_role = ROLE_BY_FAMILY.get(family, "DECIDE")
        if row["role"] != expected_role:
            failures.append(
                ("WRONG_ROLE", f"{form}: role={row['role']}, expected {expected_role}")
            )

        numbers = {}
        for name in ("period_ms", "points_per_cycle", "events_per_s", "subscriptions"):
            value = as_int(row, name, failures)
            numbers[name] = value
            if value is not None and value < 0:
                failures.append(("QUOTA_NEGATIVE", f"{form}: {name}={value}"))
        period_min, period_max = expected["period_ms"]
        period = numbers["period_ms"]
        if period is not None and not (period_min <= period <= period_max):
            failures.append(
                (
                    "PERIOD_RANGE",
                    f"{form}: period_ms={period}, expected {period_min}..{period_max} "
                    "(CONTRACT 4.3)",
                )
            )
        if row["target"] == "app":
            for name, limit in APP_QUOTA.items():
                value = numbers[name]
                if value is None:
                    continue
                if name == "period_ms":
                    if value < limit:
                        failures.append(
                            ("APP_PERIOD", f"{form}: period_ms={value} < {limit} cannot run as app")
                        )
                elif value > limit:
                    code = {
                        "points_per_cycle": "APP_POINTS",
                        "events_per_s": "APP_EVENTS",
                        "subscriptions": "APP_SUBS",
                    }[name]
                    failures.append((code, f"{form}: {name}={value} > {limit} cannot run as app"))

        if row["decision_owner"] != expected["decision_owner"]:
            failures.append(
                (
                    "WRONG_OWNER",
                    f"{form}: decision_owner={row['decision_owner']!r}, "
                    f"expected {expected['decision_owner']}",
                )
            )
        if row["execution_owner"] != expected["execution_owner"]:
            failures.append(
                (
                    "WRONG_OWNER",
                    f"{form}: execution_owner={row['execution_owner']!r}, "
                    f"expected {expected['execution_owner']}",
                )
            )
        if form in TOU_DEMAND_FORMS and row["decision_owner"] != TOU_OWNER:
            failures.append(
                (
                    "TOU_OWNER",
                    f"{form}: decision_owner={row['decision_owner']}, "
                    f"TOU/demand single source is {TOU_OWNER}",
                )
            )
        if row["class"] in TOU_DEMAND_CLASSES and row["decision_owner"] == COORDINATOR_OWNER:
            failures.append(
                (
                    "TOU_OWNER",
                    f"{form}: coordinator cannot be a TOU/demand decision source; "
                    f"decision is {TOU_OWNER}",
                )
            )
        if not row["decision_owner"] or not row["execution_owner"]:
            failures.append(("OWNER_EMPTY", f"{form}: decision_owner/execution_owner is empty"))

    actual_counts = (
        len(rows),
        len({row["class"] for row in rows}),
        len({row["adr_family"] for row in rows}),
    )
    if doc_counts is None:
        failures.append(("DOC_COUNTS", "strategy-classification.md has no forms/classes/adr_families count line"))
    elif doc_counts != actual_counts:
        failures.append(
            ("DOC_COUNTS", f"doc says {doc_counts}, table has {actual_counts}")
        )

    return failures


def set_cell(rows, form, name, value):
    for row in rows:
        if row["form"] == form:
            row[name] = value
            return
    raise KeyError(form)


def forge_all_kinds(rows):
    for row in rows:
        row["kind"] = "forged_" + row["form"]


def point_all_tou_rows_at_coordinator(rows):
    for form in TOU_DEMAND_FORMS:
        set_cell(rows, form, "decision_owner", "iems_microgrid_coordinator")


def run_selftest(rows, doc_counts):
    mutations = [
        ("missing_row", lambda r: r.remove(next(row for row in r if row["form"] == "pf_loop")), "MISSING_FORM"),
        ("safety_family_targets_app", lambda r: set_cell(r, "anti_reflux_limit", "target", "app"), "SAFETY_MODULE"),
        ("safety_label_missing", lambda r: set_cell(r, "pcs_low_soc_protect", "safety", "none"), "SAFETY_LABEL"),
        ("period_10ms_app", lambda r: set_cell(r, "tou_current_ac", "period_ms", "10"), "APP_PERIOD"),
        ("points_over_100_app", lambda r: set_cell(r, "ev_capacity", "points_per_cycle", "101"), "APP_POINTS"),
        ("events_over_1000_app", lambda r: set_cell(r, "ev_capacity", "events_per_s", "1001"), "APP_EVENTS"),
        ("subs_over_32_app", lambda r: set_cell(r, "ev_capacity", "subscriptions", "33"), "APP_SUBS"),
        ("double_decision_source", lambda r: set_cell(r, "demand_dynamic", "kind", "ems_demand_ctrl"), "DOUBLE_SOURCE"),
        ("tou_owner_coordinator", lambda r: set_cell(r, "tou_current_ac", "decision_owner", "iems_microgrid_coordinator"), "TOU_OWNER"),
        ("effective_switched_app", lambda r: set_cell(r, "soc_keep", "effective", "app"), "EFFECTIVE_MODULE"),
        ("app_family_target_module", lambda r: set_cell(r, "soc_keep", "target", "module"), "APP_TARGET"),
        ("class_swapped", lambda r: set_cell(r, "pf_loop", "class", "EV"), "WRONG_CLASS"),
        ("family_swapped", lambda r: set_cell(r, "pv_priority", "adr_family", "UNKNOWN"), "WRONG_FAMILY"),
        ("role_swapped", lambda r: set_cell(r, "anti_reflux_limit", "role", "DECIDE"), "WRONG_ROLE"),
        ("forged_kind_all", forge_all_kinds, "WRONG_KIND"),
        ("kind_trailing_space_duplicate",
         lambda r: set_cell(r, "demand_dynamic", "kind", "ems_demand_ctrl "), "DOUBLE_SOURCE"),
        ("kind_embedded_space",
         lambda r: set_cell(r, "soc_keep", "kind", "Strategy SocKeep"), "KIND_WHITESPACE"),
        ("kind_illegal_token",
         lambda r: set_cell(r, "soc_keep", "kind", "9bad"), "BAD_KIND"),
        ("forged_decision_owner",
         lambda r: set_cell(r, "soc_keep", "decision_owner", "forged_owner"), "WRONG_OWNER"),
        ("forged_execution_owner",
         lambda r: set_cell(r, "pv_priority", "execution_owner", "forged_owner"), "WRONG_OWNER"),
        ("pv_storage_owner_coordinator",
         lambda r: set_cell(r, "pv_storage_self_use", "decision_owner",
                            "iems_microgrid_coordinator"), "TOU_OWNER"),
        ("tou_rows_all_coordinator", point_all_tou_rows_at_coordinator, "TOU_OWNER"),
        ("pf_period_over_contract_max",
         lambda r: set_cell(r, "pf_loop", "period_ms", "50000"), "PERIOD_RANGE"),
    ]

    baseline = validate(rows, doc_counts)
    if baseline:
        print("SELFTEST FAIL baseline table is rejected:")
        for code, message in baseline:
            print(f"  {code}: {message}")
        return 1

    print(f"SELFTEST PASS baseline rc=0 ({len(rows)} rows)")
    failed = 0
    for name, mutate, expected_code in mutations:
        candidate = copy.deepcopy(rows)
        mutate(candidate)
        failures = validate(candidate, doc_counts)
        codes = {code for code, _ in failures}
        if expected_code in codes:
            print(f"SELFTEST PASS mutation={name} rc=1 code={expected_code}")
        else:
            failed += 1
            print(
                f"SELFTEST FAIL mutation={name} expected rc=1 code={expected_code}, "
                f"got {sorted(codes) if codes else 'PASS (gate went blind)'}"
            )

    doc_candidate = (doc_counts[0] + 1, doc_counts[1], doc_counts[2])
    doc_failures = validate(copy.deepcopy(rows), doc_candidate)
    if "DOC_COUNTS" in {code for code, _ in doc_failures}:
        print("SELFTEST PASS mutation=doc_counts_changed rc=1 code=DOC_COUNTS")
    else:
        failed += 1
        print("SELFTEST FAIL mutation=doc_counts_changed expected rc=1 code=DOC_COUNTS")

    total = len(mutations) + 1
    if failed:
        print(f"SELFTEST FAIL {failed}/{total} negative cases were not rejected")
        return 1
    print(f"SELFTEST PASS {total}/{total} negative cases rejected; baseline rc=0")
    return 0


def main():
    parser = argparse.ArgumentParser(description="Check IEMS strategy classification table.")
    parser.add_argument("--root", default=".", help="repository root")
    parser.add_argument("--table", help="override the TSV path")
    parser.add_argument("--md", help="override the explanation markdown path")
    parser.add_argument("--selftest", action="store_true", help="replay the A2 negative cases")
    args = parser.parse_args()

    root = os.path.abspath(args.root)
    table_path = args.table or os.path.join(root, "docs", "design", "strategy-classification.tsv")
    md_path = args.md or os.path.join(root, "docs", "design", "strategy-classification.md")

    try:
        _header, rows, failures = load_table(table_path)
    except OSError as error:
        print(f"❌ [IEMS_STRATEGY_CLASSIFICATION] cannot read {table_path}: {error}")
        return 1

    doc_counts = read_doc_counts(md_path)
    if args.selftest:
        return run_selftest(rows, doc_counts)

    failures = failures + validate(rows, doc_counts)
    if failures:
        print("❌ [IEMS_STRATEGY_CLASSIFICATION] table rejected:")
        for code, message in failures:
            print(f"  FAIL [{code}] {message}")
        return 1

    for row in rows:
        print(
            f"  PASS row {row['form']} class={row['class']} "
            f"family={row['adr_family']} target={row['target']} role={row['role']}"
        )
    classes = sorted({row["class"] for row in rows})
    families = sorted({row["adr_family"] for row in rows})
    app_rows = [row for row in rows if row["target"] == "app"]
    module_rows = [row for row in rows if row["target"] == "module"]
    print(
        f"  PASS [COVERAGE] forms={len(rows)}/{len(EXPECTED)} "
        f"classes={len(classes)}/{len(CLASSES)} adr_families={len(families)}/10"
    )
    print(f"  PASS [FAMILIES] {', '.join(families)}")
    print(
        f"  PASS [SAFETY] {len(module_rows)} module rows; "
        f"{len([row for row in module_rows if row['safety'] != 'none'])} safety/capacity rows"
    )
    print(
        f"  PASS [APP_QUOTA] {len(app_rows)} app rows: period>={APP_QUOTA['period_ms']}, "
        f"points<={APP_QUOTA['points_per_cycle']}, events<={APP_QUOTA['events_per_s']}, "
        f"subs<={APP_QUOTA['subscriptions']}"
    )
    print(
        f"  PASS [SINGLE_SOURCE] {len({row['kind'].strip() for row in rows})} unique kinds; "
        f"{len(TOU_DEMAND_FORMS)} TOU/demand rows owned by {TOU_OWNER}"
    )
    print(
        f"  PASS [EXPECTATIONS] {len(EXPECTED)}/{len(EXPECTED)} rows match CONTRACT "
        f"1.2 kinds, 4.3 period bounds, and pinned owners"
    )
    print(f"  PASS [EFFECTIVE] {len(rows)}/{len(rows)} module (c1 does not switch targets)")
    print(
        f"  PASS [DOC_COUNTS] forms={doc_counts[0]} classes={doc_counts[1]} "
        f"adr_families={doc_counts[2]}"
    )
    print(
        f"✅ [IEMS_STRATEGY_CLASSIFICATION] 13 forms, 6 classes, 10 ADR families, "
        f"{len(app_rows)} app / {len(module_rows)} module rows"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())

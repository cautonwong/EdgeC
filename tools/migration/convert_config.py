#!/usr/bin/env python3
"""Convert iems_v3 config families into versioned P6-04a documents.

Covers every family in config/schema/file_families.json. The schema is
the whitelist: known keys are copied, x-deprecated/x-ignored keys are dropped
and counted, and any other key fails conversion. Plaintext x-secret values
become deterministic `$secret:<family>.<relative-path>.<json-path>` references
(path separators and the .json suffix become dots); existing `$secret:`/
`${ENV}` references and empty values are preserved. The original secret value
is never printed or written.

oneOf/anyOf nodes pick the branch that validates with the source Validator,
ignoring secret-policy errors (anyOf: first matching branch). Zero oneOf
matches fail ONEOF_NO_MATCH; several fail ONEOF_AMBIGUOUS.

Modes:
  (default write)  write converted files under --out
  --dry-run        print the per-file report, write nothing
  --check          convert in memory and compare byte-for-byte with --out
                   (default: tools/migration/golden/<family>)

Fixtures may carry a `_convert_case.json` at the root to supply --family,
--schema-dir and the dry-run/check mode for the one-argument guard-selftest
harness.
"""

import argparse
import json
import os
import re
import sys

TOOLS_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, TOOLS_DIR)
import config_inventory
import config_schema

REPO_ROOT = os.path.dirname(os.path.dirname(TOOLS_DIR))
DEFAULT_SCHEMA_DIR = os.path.join(REPO_ROOT, "config", "schema")
GOLDEN_DIR = os.path.join(TOOLS_DIR, "golden")
CASE_FILE = "_convert_case.json"
CASE_KEYS = {"family", "mode", "out", "schema_dir", "description"}
SECRET_REF = config_schema.SECRET_REF
ENV_REF = config_schema.ENV_REF
HOST_PORT_REF = config_schema.HOST_PORT_REF


class ConvertError(Exception):
    pass


class Stats:
    def __init__(self):
        self.kept = 0
        self.dropped = 0
        self.deprecated = 0
        self.ignored = 0
        self.secret_replaced = 0
        self.secret_unset = 0
        self.unknown = 0

    def add(self, other):
        self.kept += other.kept
        self.dropped += other.dropped
        self.deprecated += other.deprecated
        self.ignored += other.ignored
        self.secret_replaced += other.secret_replaced
        self.secret_unset += other.secret_unset
        self.unknown += other.unknown

    def line(self):
        return (f"kept={self.kept} dropped={self.dropped} "
                f"deprecated={self.deprecated} ignored={self.ignored} "
                f"secret_replaced={self.secret_replaced} "
                f"secret_unset={self.secret_unset} unknown={self.unknown}")


def json_path(parts):
    return ".".join(str(part) for part in parts)


def secret_placeholder(family, relative_path, parts):
    relative = os.path.splitext(relative_path)[0]
    prefix = re.sub(r"[^A-Za-z0-9_.-]", "_", relative.replace("/", "."))
    return f"$secret:{family}.{prefix}.{json_path(parts)}"


# Secret policy is a value rule, not shape: a plaintext secret must not hide
# the only branch that structurally matches.
BRANCH_VALUE_ERRORS = {"PLAINTEXT_SECRET", "SECRET_UNSET"}


def select_branch(value, schema, path, relative_path):
    """Pick the schema branch for a oneOf/anyOf node."""
    keyword = "oneOf" if "oneOf" in schema else "anyOf"
    where = json_path(path) or "$"
    validator = config_schema.Validator("source", None)
    matches = []
    for index, branch in enumerate(schema[keyword]):
        report = validator.validate(value, branch, where)
        if not any(entry["kind"] not in BRANCH_VALUE_ERRORS
                   for entry in report.errors):
            matches.append(index)
    if not matches:
        raise ConvertError(
            f"{keyword.upper()}_NO_MATCH {relative_path}: {where}")
    if keyword == "oneOf" and len(matches) > 1:
        raise ConvertError(
            f"ONEOF_AMBIGUOUS {relative_path}: {where} matches branches "
            f"{matches}")
    return schema[keyword][matches[0]]


def replace_secrets(value, schema, path, family, relative_path, stats):
    """Copy the document, replacing plaintext x-secret values with references."""
    if isinstance(schema, dict) and ("oneOf" in schema or "anyOf" in schema):
        schema = select_branch(value, schema, path, relative_path)
    if isinstance(schema, dict) and "properties" in schema and isinstance(value, dict):
        secrets = set(schema.get("x-secret", []))
        properties = schema["properties"]
        result = {}
        for key, item in value.items():
            child = path + [key]
            if key in secrets and isinstance(item, str):
                if item == "":
                    stats.secret_unset += 1
                elif not (SECRET_REF.match(item) or ENV_REF.match(item)):
                    item = secret_placeholder(family, relative_path, child)
                    stats.secret_replaced += 1
            if key in properties:
                item = replace_secrets(
                    item, properties[key], child, family, relative_path, stats)
            result[key] = item
        return result
    if isinstance(schema, dict) and "items" in schema and isinstance(value, list):
        return [replace_secrets(item, schema["items"], path + [index],
                                family, relative_path, stats)
                for index, item in enumerate(value)]
    return value


def convert_value(value, schema, path, family, relative_path, stats):
    """Copy value through the schema whitelist, dropping deprecated/ignored keys."""
    if not isinstance(schema, dict):
        raise ConvertError(f"{relative_path}: no schema at {json_path(path) or '$'}")

    if "oneOf" in schema or "anyOf" in schema:
        branch = select_branch(value, schema, path, relative_path)
        return convert_value(value, branch, path, family, relative_path, stats)

    if isinstance(value, dict) and schema.get("type") == "object":
        properties = schema.get("properties", {})
        deprecated = set(schema.get("x-deprecated", []))
        ignored = set(schema.get("x-ignored", []))
        result = {}
        for key, item in value.items():
            child = path + [key]
            if key in deprecated:
                stats.dropped += 1
                stats.deprecated += 1
                continue
            if key in ignored:
                stats.dropped += 1
                stats.ignored += 1
                continue
            if key not in properties:
                stats.unknown += 1
                raise ConvertError(
                    f"UNKNOWN_KEY {relative_path}: {json_path(child)}")
            result[key] = convert_value(
                item, properties[key], child, family, relative_path, stats)
        return result

    if isinstance(value, list) and schema.get("type") == "array":
        prefix = schema.get("prefixItems", [])
        items = schema.get("items")
        if not prefix and not isinstance(items, dict):
            if value:
                raise ConvertError(
                    f"{relative_path}: array without item schema at "
                    f"{json_path(path) or '$'}")
            stats.kept += 1
            return []
        result = []
        for index, item in enumerate(value):
            child = prefix[index] if index < len(prefix) else items
            if not isinstance(child, dict):
                raise ConvertError(
                    f"{relative_path}: no item schema at "
                    f"{json_path(path + [index])}")
            result.append(convert_value(
                item, child, path + [index], family, relative_path, stats))
        return result

    if isinstance(value, (dict, list)):
        raise ConvertError(
            f"{relative_path}: source shape does not match schema at "
            f"{json_path(path) or '$'}")
    stats.kept += 1
    return value


def inject_version(document, schema, version):
    forms = config_schema.root_forms(schema)
    if isinstance(document, list):
        if "array" not in forms:
            raise ConvertError("array document against a non-array root schema")
        return {"schema_version": version, "config": document}
    if isinstance(document, dict):
        if "object" not in forms:
            raise ConvertError("object document against a non-object root schema")
        result = dict(document)
        result["schema_version"] = version
        return result
    raise ConvertError(f"unsupported root type {type(document).__name__}")


def format_document(document):
    return json.dumps(
        document, sort_keys=True, ensure_ascii=False, indent=2) + "\n"


def load_case(root):
    path = os.path.join(root, CASE_FILE)
    if not os.path.isfile(path):
        return {}
    try:
        case = config_schema.load_json(path)
    except config_schema.SchemaError as exc:
        raise ConvertError(str(exc)) from exc
    if not isinstance(case, dict):
        raise ConvertError(f"{path}: expected an object")
    unknown = set(case) - CASE_KEYS
    if unknown:
        raise ConvertError(f"{path}: unknown key(s) {sorted(unknown)}")
    family = case.get("family")
    if not isinstance(family, str) or not family:
        raise ConvertError(f"{path}: family must be a non-empty string")
    mode = case.get("mode")
    if mode not in ("dry-run", "check"):
        raise ConvertError(f"{path}: mode must be dry-run or check")
    for key in ("out", "schema_dir", "description"):
        if key in case and not isinstance(case[key], str):
            raise ConvertError(f"{path}: {key} must be a string")
    return case


def resolve_from(root, value):
    return value if os.path.isabs(value) else os.path.join(root, value)


def family_files(root, family_entry):
    files = []
    for relative_path in config_schema.scan_files(root):
        matched = config_schema.match_family(relative_path, [family_entry])
        if matched:
            files.append(relative_path)
    return files


def load_port_names(root):
    path = os.path.join(root, "port_config.json")
    if not os.path.isfile(path):
        return None
    try:
        document = config_schema.load_json(path)
    except config_schema.SchemaError:
        return None
    return config_schema.port_names({"port_config.json": document})


def check_device_channels(root, documents, failures_by_file):
    ports = load_port_names(root)
    if ports is None:
        return ("UNCHECKED", "port_config.json missing or unparseable")
    checked = 0
    endpoints = 0
    for relative_path, document in sorted(documents.items()):
        for class_index, device_index, _, device in config_schema.device_entries(document):
            channel = device.get("channel")
            if not isinstance(channel, str):
                continue
            if HOST_PORT_REF.match(channel):
                endpoints += 1
                continue
            checked += 1
            path = f"dev_class[{class_index}].devs[{device_index}].channel"
            if channel not in ports:
                failures_by_file[relative_path].append(
                    f"BAD_REFERENCE {path}: {channel!r} is not a port name "
                    f"in port_config.json")
    detail = (f"{checked} symbolic channel(s) in port set; "
              f"{endpoints} host:port endpoint(s) not local ports")
    if any(failures_by_file.get(path) for path in documents):
        return ("FAIL", detail)
    return ("PASS", detail)


def template_notice(documents):
    count = 0
    for relative_path in sorted(documents):
        document = documents[relative_path]
        for _, _, _, device in config_schema.device_entries(document):
            if isinstance(device.get("template"), str):
                count += 1
    if not count:
        return None
    return (f"UNCHECKED device.template -> template tree: {count} template(s) "
            f"not checked (B1 template tree missing)")


def check_dev_group_references(root, converted_documents, errors_by_file):
    """Run config_schema's mp/no reference check against the source siblings."""
    relative_path = "dev_group_cfg.json"
    if relative_path not in converted_documents:
        return None
    documents = {}
    for related in (relative_path, "ems_lc_list.json"):
        try:
            documents[related] = config_schema.load_json(
                os.path.join(root, related))
        except config_schema.SchemaError:
            pass
    source = documents.get(relative_path)
    if isinstance(source, list):
        for group in source:
            devices = group.get("devices") if isinstance(group, dict) else None
            if not isinstance(devices, list):
                continue
            for device in devices:
                if not isinstance(device, dict):
                    continue
                mp = device.get("mp")
                if not isinstance(mp, str):
                    continue
                related = f"lc_devs/{mp}/device_config.json"
                if related not in documents:
                    try:
                        documents[related] = config_schema.load_json(
                            os.path.join(root, related))
                    except config_schema.SchemaError:
                        pass
    issues = {relative_path: {"errors": []}}
    result = config_schema.check_dev_group(documents, issues)
    if result is None:
        return None
    errors_by_file[relative_path].extend(
        config_schema.format_issue(entry)
        for entry in issues[relative_path]["errors"])
    return result["status"], result["label"], result["detail"]


def automatic_control_notice(documents):
    count = 0
    for relative_path in sorted(documents):
        document = documents[relative_path]
        rules = document.get("config") if isinstance(document, dict) else document
        if not isinstance(rules, list):
            continue
        for rule in rules:
            infos = (rule.get("conditionInfo")
                     if isinstance(rule, dict) else None)
            if not isinstance(infos, list):
                continue
            for info in infos:
                conditions = (info.get("conditionList")
                              if isinstance(info, dict) else None)
                if not isinstance(conditions, list):
                    continue
                for condition in conditions:
                    and_list = (condition.get("and_list")
                                if isinstance(condition, dict) else None)
                    if not isinstance(and_list, list):
                        continue
                    count += sum(
                        1 for item in and_list
                        if isinstance(item, dict)
                        and isinstance(item.get("name"), str))
    if not count:
        return ("UNCHECKED automatic_control.and_list.name -> template tree: "
                "no point references (B1 template tree missing)")
    return (f"UNCHECKED automatic_control.and_list.name -> template tree: "
            f"{count} point reference(s) not checked (B1 template tree missing)")


def bmser_tag_notice(documents):
    count = 0
    for document in documents.values():
        for dev in document.get("dev", []):
            for block in dev.get("blocks", []):
                for sub in block.get("sub_id", []):
                    if isinstance(sub.get("tag"), str):
                        count += 1
    return (f"UNCHECKED bmser_ipc_bin.tag -> device points: {count} tag "
            f"reference(s) not checked (B1 template tree missing)")


def check_bmser_devices(document):
    """dev_id/module/type are optional in the schema but build topic segments."""
    problems = []
    for index, dev in enumerate(document.get("dev", [])):
        for key in ("dev_id", "module", "type"):
            if not dev.get(key):
                problems.append(
                    f"MISSING_VALUE dev[{index}].{key}: empty or missing; "
                    "topic segment would be empty")
    return problems


def convert_family(family, root, schema_dir, family_map):
    families = config_schema.load_families(schema_dir, family_map)
    entry = next((item for item in families if item["name"] == family), None)
    if entry is None:
        raise ConvertError(f"family {family!r} is not in {family_map}")

    relative_files = family_files(root, entry)
    if not relative_files:
        raise ConvertError(f"no {family} source files under {root}")

    schema = entry["schema"]
    version = entry["schema_version"]
    outputs = {}
    stats_by_file = {}
    errors_by_file = {}
    converted_documents = {}
    for relative_path in relative_files:
        stats = Stats()
        stats_by_file[relative_path] = stats
        errors_by_file[relative_path] = []
        path = os.path.join(root, relative_path)
        try:
            document = config_schema.load_json(path)
        except config_schema.SchemaError as exc:
            errors_by_file[relative_path].append(str(exc))
            continue
        replaced = replace_secrets(
            document, schema, [], family, relative_path, stats)
        source_report = config_schema.Validator("source", version).document(
            replaced, schema)
        if source_report.errors:
            errors_by_file[relative_path].extend(
                config_schema.format_issue(item)
                for item in source_report.errors)
            continue
        if family == "bmser_ipc_bin":
            problems = check_bmser_devices(replaced)
            if problems:
                errors_by_file[relative_path].extend(problems)
                continue
        try:
            converted = convert_value(
                replaced, schema, [], family, relative_path, stats)
        except ConvertError as exc:
            errors_by_file[relative_path].append(str(exc))
            continue
        converted = inject_version(converted, schema, version)
        converted_report = config_schema.Validator("converted", version).document(
            converted, schema)
        if converted_report.errors:
            errors_by_file[relative_path].extend(
                config_schema.format_issue(item)
                for item in converted_report.errors)
            continue
        outputs[relative_path] = format_document(converted)
        converted_documents[relative_path] = converted

    cross = []
    notices = []
    if family == "device":
        failures = {path: [] for path in converted_documents}
        if converted_documents:
            cross_status, detail = check_device_channels(
                root, converted_documents, failures)
            for relative_path, messages in failures.items():
                errors_by_file[relative_path].extend(messages)
            cross.append(
                (cross_status, "device.channel -> port_config ports", detail))
        notices.append(
            template_notice(converted_documents)
            or "UNCHECKED device.template -> template tree: no device templates")
    elif family == "dev_group":
        reference = check_dev_group_references(
            root, converted_documents, errors_by_file)
        if reference:
            cross.append(reference)
    elif family == "automatic_control":
        notices.append(automatic_control_notice(converted_documents))
    elif family == "bmser_ipc_bin":
        notices.append(bmser_tag_notice(converted_documents))
    elif family == "mqtt_pub_dev_list":
        reference = config_schema.check_pub_tags(
            None, converted_documents, {"mqtt_pub_dev_list": relative_files})
        if reference:
            cross.append((reference["status"], reference["label"],
                          reference["detail"]))
    elif family == "north_map":
        failures = {path: {"errors": []} for path in converted_documents}
        reference = config_schema.check_north_map_addresses(
            converted_documents, {"north_map": relative_files}, failures)
        for relative_path, record in failures.items():
            errors_by_file[relative_path].extend(
                config_schema.format_issue(entry) for entry in record["errors"])
        if reference:
            cross.append((reference["status"], reference["label"],
                          reference["detail"]))

    return {
        "family": family,
        "version": version,
        "files": relative_files,
        "outputs": outputs,
        "stats": stats_by_file,
        "errors": errors_by_file,
        "cross": cross,
        "notices": notices,
    }


def print_report(result):
    total = Stats()
    for relative_path in result["files"]:
        stats = result["stats"][relative_path]
        total.add(stats)
        errors = result["errors"][relative_path]
        status = "FAIL" if errors or relative_path not in result["outputs"] else "OK"
        print(f"file {relative_path}: {stats.line()} -> {status}")
        for message in errors:
            print(f"FAIL {relative_path}: {message}")
    for status, label, detail in result["cross"]:
        print(f"{status:<9} {label}: {detail}")
    for notice in result["notices"]:
        print(notice)


def has_errors(result):
    return any(result["errors"][path] for path in result["files"])


def write_outputs(result, out_dir):
    for relative_path in sorted(result["outputs"]):
        target = os.path.join(out_dir, relative_path)
        os.makedirs(os.path.dirname(target) or out_dir, exist_ok=True)
        with open(target, "w", encoding="utf-8", newline="\n") as handle:
            handle.write(result["outputs"][relative_path])
    print(f"WROTE {len(result['outputs'])} file(s) -> {out_dir}")


def scan_expected(expected_dir):
    files = []
    for dirpath, dirnames, filenames in os.walk(expected_dir):
        dirnames.sort()
        for name in sorted(filenames):
            if name.endswith(".json") and not name.startswith("_"):
                absolute = os.path.join(dirpath, name)
                files.append(
                    os.path.relpath(absolute, expected_dir).replace(os.sep, "/"))
    return sorted(files)


def check_expected(result, expected_dir):
    family = result["family"]
    version = result["version"]
    families = config_schema.load_families(
        result["schema_dir"], result["family_map"])
    schema = next(item["schema"] for item in families if item["name"] == family)

    failures = 0
    if not os.path.isdir(expected_dir):
        print(f"FAIL expected directory missing: {expected_dir}")
        return 1
    expected_files = scan_expected(expected_dir)
    produced = set(result["outputs"])
    expected = set(expected_files)
    for relative_path in sorted(expected - produced):
        print(f"FAIL {relative_path}: not produced by the converter")
        failures += 1
    for relative_path in sorted(produced - expected):
        print(f"FAIL {relative_path}: missing from {expected_dir}")
        failures += 1
    for relative_path in sorted(produced & expected):
        expected_path = os.path.join(expected_dir, relative_path)
        try:
            document = config_schema.load_json(expected_path)
        except config_schema.SchemaError as exc:
            print(f"FAIL {relative_path}: {exc}")
            failures += 1
            continue
        report = config_schema.Validator("converted", version).document(
            document, schema)
        for item in report.errors:
            print(f"FAIL {relative_path}: {config_schema.format_issue(item)}")
            failures += 1
        with open(expected_path, "rb") as handle:
            expected_bytes = handle.read()
        if expected_bytes != result["outputs"][relative_path].encode("utf-8"):
            print(f"FAIL {relative_path}: bytes differ from {expected_dir}")
            failures += 1
        else:
            print(f"OK {relative_path}: bytes match")
    if failures:
        print(f"RESULT CHECK FAIL: {failures} problem(s)")
        return 1
    print(f"RESULT CHECK OK: {len(expected_files)} file(s)")
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--family", default=None,
                        help="config family to convert "
                             "(see config/schema/file_families.json)")
    parser.add_argument("--root", default=config_inventory.DEFAULT_ROOT,
                        help=f"config root (default: {config_inventory.DEFAULT_ROOT})")
    parser.add_argument("--out", default=None,
                        help="write target, or expected directory for --check")
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--dry-run", action="store_true",
                      help="print the conversion report, write nothing")
    mode.add_argument("--check", action="store_true",
                      help="compare fresh conversion with --out or the golden set")
    parser.add_argument("--schema-dir", default=None,
                        help="schema directory (default: config/schema)")
    parser.add_argument("--family-map", default=None,
                        help="family map JSON (default: <schema-dir>/file_families.json)")
    args = parser.parse_args()

    root = os.path.abspath(args.root)
    if not os.path.isdir(root):
        print(f"FAIL root is not a directory: {root}")
        return 1

    try:
        case = load_case(root)
    except ConvertError as exc:
        print(f"FAIL {exc}")
        return 1

    family = args.family or case.get("family")
    if not family:
        print("FAIL --family is required (or _convert_case.json must name one)")
        return 1

    schema_dir = args.schema_dir
    if schema_dir is None and case.get("schema_dir"):
        schema_dir = resolve_from(root, case["schema_dir"])
    schema_dir = os.path.abspath(schema_dir or DEFAULT_SCHEMA_DIR)
    family_map = args.family_map or os.path.join(schema_dir, "file_families.json")

    if args.dry_run:
        if args.out:
            print("FAIL --out cannot be combined with --dry-run")
            return 1
        mode_name = "dry-run"
    elif args.check:
        mode_name = "check"
    elif args.out:
        mode_name = "write"
    else:
        mode_name = case.get("mode", "write")
        if mode_name == "write":
            print("FAIL --out is required to write converted files")
            return 1

    try:
        result = convert_family(family, root, schema_dir, family_map)
    except (ConvertError, config_schema.SchemaError) as exc:
        print(f"FAIL {exc}")
        return 1
    result["schema_dir"] = schema_dir
    result["family_map"] = family_map

    print(f"== convert {family} (root={root}, schema_version={result['version']})")
    print_report(result)

    if has_errors(result):
        print(f"RESULT FAIL: {family} conversion rejected")
        return 1

    if mode_name == "dry-run":
        print(f"RESULT OK: {len(result['outputs'])} file(s) converted in memory")
        return 0

    if mode_name == "check":
        expected_dir = args.out
        if expected_dir is None and case.get("out"):
            expected_dir = resolve_from(root, case["out"])
        expected_dir = os.path.abspath(
            expected_dir or os.path.join(GOLDEN_DIR, family))
        print(f"== check {family} against {expected_dir}")
        return check_expected(result, expected_dir)

    out_dir = os.path.abspath(args.out)
    write_outputs(result, out_dir)
    return 0


if __name__ == "__main__":
    sys.exit(main())

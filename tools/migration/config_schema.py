#!/usr/bin/env python3
"""Validate P6-04a config schemas against an iems_v3 config root.

Supported JSON Schema subset:
  type, required, properties, additionalProperties, enum, pattern, items,
  prefixItems, minItems, maxItems, minimum, maximum, const, oneOf, anyOf
Extensions:
  x-schema-version (root), x-deprecated, x-ignored, x-secret

Unknown object keys are rejected unless they appear in properties,
x-deprecated or x-ignored. additionalProperties defaults to false so a schema
cannot silently accept unknown keys. prefixItems (fixed tuples) without an
items fallback must bound the array with maxItems so no position escapes
validation.

Modes:
  source     source files have no schema_version; it is optional if present.
  converted  object roots must carry schema_version; array roots must use the
             {"schema_version": "...", "config": [...]} envelope. P1-03b injects
             both. This is enforced here because a JSON array cannot carry a
             sibling version field.
"""

import argparse
import fnmatch
import json
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import config_inventory

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DEFAULT_SCHEMA_DIR = os.path.join(REPO_ROOT, "config", "schema")
SECRET_REF = re.compile(r"^\$secret:[A-Za-z0-9_.-]+$")
ENV_REF = re.compile(r"^\$\{[A-Za-z_][A-Za-z0-9_]*\}$")
HOST_PORT_REF = re.compile(r"^[A-Za-z0-9_.-]+:\d+$")
TYPE_NAMES = ("object", "array", "string", "integer", "number", "boolean", "null")
SCHEMA_KEYWORDS = {
    "$schema", "$id", "$comment", "title", "description",
    "type", "required", "properties", "additionalProperties", "enum",
    "pattern", "items", "prefixItems", "minItems", "maxItems",
    "minimum", "maximum", "const", "oneOf", "anyOf",
    "x-schema-version", "x-deprecated", "x-ignored", "x-secret",
}


class SchemaError(Exception):
    pass


class Report:
    def __init__(self, errors=None, warnings=None):
        self.errors = errors or []
        self.warnings = warnings or []


def issue(kind, path, detail=""):
    return {"kind": kind, "path": path or "$", "detail": detail}


def format_issue(entry):
    text = f"{entry['kind']} {entry['path']}"
    if entry["detail"]:
        text += f": {entry['detail']}"
    return text


def join_path(path, key):
    return f"{path}.{key}" if path else key


def index_path(path, index):
    return f"{path}[{index}]" if path else f"[{index}]"


def is_integer(value):
    if isinstance(value, bool):
        return False
    return isinstance(value, int) or (isinstance(value, float) and value.is_integer())


def is_number(value):
    return isinstance(value, (int, float)) and not isinstance(value, bool)


def type_ok(value, name):
    if name == "object":
        return isinstance(value, dict)
    if name == "array":
        return isinstance(value, list)
    if name == "string":
        return isinstance(value, str)
    if name == "integer":
        return is_integer(value)
    if name == "number":
        return is_number(value)
    if name == "boolean":
        return isinstance(value, bool)
    if name == "null":
        return value is None
    return False


def json_type_name(value):
    if value is None:
        return "null"
    if isinstance(value, bool):
        return "boolean"
    if isinstance(value, int):
        return "integer"
    if isinstance(value, float):
        return "number"
    if isinstance(value, str):
        return "string"
    if isinstance(value, list):
        return "array"
    if isinstance(value, dict):
        return "object"
    return type(value).__name__


def json_equal(left, right):
    if isinstance(left, bool) or isinstance(right, bool):
        return isinstance(left, bool) and isinstance(right, bool) and left == right
    if is_number(left) and is_number(right):
        return left == right
    return type(left) is type(right) and left == right


def load_json(path):
    try:
        with open(path, "r", encoding="utf-8") as handle:
            return json.load(handle)
    except OSError as exc:
        raise SchemaError(f"{path}: {exc}") from exc
    except (UnicodeDecodeError, json.JSONDecodeError) as exc:
        raise SchemaError(f"{path}: invalid JSON: {exc}") from exc


def check_schema(node, path):
    if not isinstance(node, dict):
        return [f"{path}: schema node must be an object"]
    errors = []
    for key in sorted(set(node) - SCHEMA_KEYWORDS):
        errors.append(f"{path}: unsupported keyword {key!r}")

    if "type" in node:
        declared = node["type"]
        names = declared if isinstance(declared, list) else [declared]
        if not isinstance(declared, (str, list)) or not names:
            errors.append(f"{path}.type: expected a type name or non-empty list")
        else:
            for name in names:
                if name not in TYPE_NAMES:
                    errors.append(f"{path}.type: unknown type {name!r}")

    if "required" in node:
        required = node["required"]
        if (not isinstance(required, list) or not required
                or not all(isinstance(key, str) and key for key in required)
                or len(set(required)) != len(required)):
            errors.append(f"{path}.required: expected unique non-empty strings")

    if "properties" in node:
        properties = node["properties"]
        if not isinstance(properties, dict):
            errors.append(f"{path}.properties: expected an object")
        else:
            for key, subschema in properties.items():
                if not isinstance(key, str) or not key:
                    errors.append(f"{path}.properties: key must be a non-empty string")
                else:
                    errors.extend(check_schema(subschema, f"{path}.properties.{key}"))

    if "additionalProperties" in node:
        extra = node["additionalProperties"]
        if extra is not False:
            errors.append(
                f"{path}.additionalProperties: must be false; unknown keys must "
                "be listed in properties, x-deprecated or x-ignored")

    if "items" in node:
        errors.extend(check_schema(node["items"], f"{path}.items"))

    if "prefixItems" in node:
        branches = node["prefixItems"]
        if not isinstance(branches, list) or not branches:
            errors.append(f"{path}.prefixItems: expected a non-empty list")
        else:
            for index, branch in enumerate(branches):
                errors.extend(check_schema(branch, f"{path}.prefixItems[{index}]"))
            if "items" not in node:
                max_items = node.get("maxItems")
                if not is_integer(max_items) or max_items > len(branches):
                    errors.append(
                        f"{path}.prefixItems: without items, maxItems must be "
                        f"<= {len(branches)} so every position is validated")

    if "enum" in node:
        if not isinstance(node["enum"], list) or not node["enum"]:
            errors.append(f"{path}.enum: expected a non-empty list")

    if "pattern" in node:
        if not isinstance(node["pattern"], str):
            errors.append(f"{path}.pattern: expected a string")
        else:
            try:
                re.compile(node["pattern"])
            except re.error as exc:
                errors.append(f"{path}.pattern: invalid regex: {exc}")

    for key in ("minItems", "maxItems"):
        if key in node and (not is_integer(node[key]) or node[key] < 0):
            errors.append(f"{path}.{key}: expected a non-negative integer")
    if ("minItems" in node and "maxItems" in node
            and is_integer(node["minItems"]) and is_integer(node["maxItems"])
            and node["minItems"] > node["maxItems"]):
        errors.append(f"{path}: minItems is greater than maxItems")

    for key in ("minimum", "maximum"):
        if key in node and not is_number(node[key]):
            errors.append(f"{path}.{key}: expected a number")
    if ("minimum" in node and "maximum" in node
            and is_number(node["minimum"]) and is_number(node["maximum"])
            and node["minimum"] > node["maximum"]):
        errors.append(f"{path}: minimum is greater than maximum")

    for key in ("oneOf", "anyOf"):
        if key in node:
            branches = node[key]
            if not isinstance(branches, list) or not branches:
                errors.append(f"{path}.{key}: expected a non-empty list")
            else:
                for index, branch in enumerate(branches):
                    errors.extend(check_schema(branch, f"{path}.{key}[{index}]"))

    for key in ("x-deprecated", "x-ignored", "x-secret"):
        if key in node:
            names = node[key]
            if (not isinstance(names, list)
                    or not all(isinstance(name, str) and name for name in names)
                    or len(set(names)) != len(names)):
                errors.append(f"{path}.{key}: expected unique non-empty strings")

    if "x-schema-version" in node and not (
            isinstance(node["x-schema-version"], str) and node["x-schema-version"]):
        errors.append(f"{path}.x-schema-version: expected a non-empty string")

    properties = node.get("properties")
    if isinstance(properties, dict):
        for name in node.get("x-secret", []):
            if name not in properties:
                errors.append(f"{path}.x-secret: {name!r} is not in properties")

    return errors


def root_branches_with_paths(schema, path):
    if "oneOf" in schema:
        for index, branch in enumerate(schema["oneOf"]):
            yield from root_branches_with_paths(branch, f"{path}.oneOf[{index}]")
    elif "anyOf" in schema:
        for index, branch in enumerate(schema["anyOf"]):
            yield from root_branches_with_paths(branch, f"{path}.anyOf[{index}]")
    else:
        yield path, schema


def root_forms(schema):
    forms = set()
    for _, branch in root_branches_with_paths(schema, ""):
        declared = branch.get("type")
        if isinstance(declared, list):
            forms.update(declared)
        elif isinstance(declared, str):
            forms.add(declared)
    return forms


def check_root_schema(schema, name):
    errors = check_schema(schema, name)
    if not isinstance(schema, dict):
        return errors
    version = schema.get("x-schema-version")
    if not isinstance(version, str) or not version:
        errors.append(f"{name}: x-schema-version must be a non-empty string")
        version = None
    if not any(key in schema for key in ("type", "oneOf", "anyOf")):
        errors.append(f"{name}: root needs type, oneOf or anyOf")
    for branch_name, branch in root_branches_with_paths(schema, name):
        if branch.get("type") != "object":
            continue
        properties = branch.get("properties")
        if not isinstance(properties, dict) or "schema_version" not in properties:
            errors.append(f"{branch_name}: object root must define properties.schema_version")
            continue
        declared = properties["schema_version"]
        if not isinstance(declared, dict) or declared.get("const") != version:
            errors.append(
                f"{branch_name}: properties.schema_version.const must be {version!r}")
    return errors


class Validator:
    def __init__(self, mode, schema_version):
        self.mode = mode
        self.schema_version = schema_version

    def document(self, document, schema):
        if self.mode == "source":
            return self.validate(document, schema, "")
        forms = root_forms(schema)
        if (isinstance(document, dict) and "array" in forms
                and set(document) == {"schema_version", "config"}):
            errors = []
            if not json_equal(document["schema_version"], self.schema_version):
                errors.append(issue(
                    "SCHEMA_VERSION_MISMATCH", "schema_version",
                    f"expected {self.schema_version!r}"))
            report = self.validate(document["config"], schema, "config")
            return Report(errors + report.errors, report.warnings)
        if isinstance(document, dict):
            if "object" in forms:
                if "schema_version" not in document:
                    return Report([issue(
                        "MISSING_SCHEMA_VERSION", "$",
                        "converted object root requires schema_version")], [])
                return self.validate(document, schema, "")
            return Report([issue(
                "MISSING_SCHEMA_VERSION", "$",
                "converted array root requires a {schema_version, config} envelope")], [])
        if "object" in forms and "array" not in forms:
            return self.validate(document, schema, "")
        return Report([issue(
            "MISSING_SCHEMA_VERSION", "$",
            "converted array root requires a {schema_version, config} envelope")], [])

    def validate(self, value, schema, path):
        if "oneOf" in schema:
            return self._combine(value, schema["oneOf"], path, exclusive=True)
        if "anyOf" in schema:
            return self._combine(value, schema["anyOf"], path, exclusive=False)
        errors = []
        warnings = []

        declared = schema.get("type")
        if declared is not None:
            names = declared if isinstance(declared, list) else [declared]
            if not any(type_ok(value, name) for name in names):
                errors.append(issue(
                    "TYPE_MISMATCH", path,
                    f"expected {'/'.join(names)}, got {json_type_name(value)}"))
                return Report(errors, warnings)

        if "const" in schema and not json_equal(value, schema["const"]):
            errors.append(issue(
                "CONST_MISMATCH", path, f"expected {schema['const']!r}"))
        if "enum" in schema and not any(
                json_equal(value, candidate) for candidate in schema["enum"]):
            errors.append(issue(
                "ENUM_MISMATCH", path, f"{value!r} is not in {schema['enum']!r}"))
        if isinstance(value, str) and "pattern" in schema:
            if re.search(schema["pattern"], value) is None:
                errors.append(issue(
                    "PATTERN_MISMATCH", path,
                    f"does not match {schema['pattern']!r}"))
        if is_number(value):
            if "minimum" in schema and value < schema["minimum"]:
                errors.append(issue(
                    "MINIMUM", path, f"{value!r} is below {schema['minimum']!r}"))
            if "maximum" in schema and value > schema["maximum"]:
                errors.append(issue(
                    "MAXIMUM", path, f"{value!r} is above {schema['maximum']!r}"))

        if isinstance(value, dict):
            report = self._object(value, schema, path)
            errors.extend(report.errors)
            warnings.extend(report.warnings)
        elif isinstance(value, list):
            report = self._array(value, schema, path)
            errors.extend(report.errors)
            warnings.extend(report.warnings)
        return Report(errors, warnings)

    def _combine(self, value, branches, path, exclusive):
        results = [self.validate(value, branch, path) for branch in branches]
        valid = [report for report in results if not report.errors]
        if valid:
            if exclusive and len(valid) > 1:
                return Report([issue(
                    "ONEOF_AMBIGUOUS", path,
                    f"{len(valid)} branches matched")], [])
            return valid[0]
        best = min(results, key=lambda report: len(report.errors))
        return best

    def _object(self, value, schema, path):
        errors = []
        warnings = []
        properties = schema.get("properties", {})
        deprecated = set(schema.get("x-deprecated", []))
        ignored = set(schema.get("x-ignored", []))
        secrets = set(schema.get("x-secret", []))

        for key in schema.get("required", []):
            if key not in value:
                errors.append(issue("MISSING_REQUIRED", join_path(path, key)))

        for key, item in value.items():
            child = join_path(path, key)
            if key in properties:
                report = self.validate(item, properties[key], child)
                errors.extend(report.errors)
                warnings.extend(report.warnings)
            elif key in deprecated or key in ignored:
                pass
            else:
                errors.append(issue("UNKNOWN_KEY", child))

            if key in deprecated:
                warnings.append(issue("DEPRECATED_KEY", child))
            elif key in ignored:
                warnings.append(issue("IGNORED_KEY", child))

            if key in secrets:
                self._secret(item, child, errors, warnings)
        return Report(errors, warnings)

    def _secret(self, value, path, errors, warnings):
        if not isinstance(value, str):
            errors.append(issue("SECRET_NOT_STRING", path, "must be a string"))
        elif value == "":
            warnings.append(issue("SECRET_UNSET", path, "empty value treated as unset"))
        elif SECRET_REF.match(value) or ENV_REF.match(value):
            pass
        else:
            errors.append(issue(
                "PLAINTEXT_SECRET", path,
                "plaintext value withheld; use $secret:<name> or ${ENV_NAME}"))

    def _array(self, value, schema, path):
        errors = []
        warnings = []
        if "minItems" in schema and len(value) < schema["minItems"]:
            errors.append(issue(
                "MIN_ITEMS", path, f"{len(value)} < {schema['minItems']}"))
        if "maxItems" in schema and len(value) > schema["maxItems"]:
            errors.append(issue(
                "MAX_ITEMS", path, f"{len(value)} > {schema['maxItems']}"))
        prefix = schema.get("prefixItems", [])
        fallback = schema.get("items")
        for index, item in enumerate(value):
            if index < len(prefix):
                child_schema = prefix[index]
            elif fallback is not None:
                child_schema = fallback
            else:
                continue
            report = self.validate(item, child_schema, index_path(path, index))
            errors.extend(report.errors)
            warnings.extend(report.warnings)
        return Report(errors, warnings)


def load_families(schema_dir, family_map_path):
    document = load_json(family_map_path)
    if not isinstance(document, dict) or not isinstance(document.get("families"), list):
        raise SchemaError(f"{family_map_path}: expected an object with a families list")
    families = []
    names = set()
    for index, entry in enumerate(document["families"]):
        where = f"{family_map_path}.families[{index}]"
        if not isinstance(entry, dict):
            raise SchemaError(f"{where}: expected an object")
        name = entry.get("name")
        schema_name = entry.get("schema")
        patterns = entry.get("patterns")
        if not isinstance(name, str) or not name:
            raise SchemaError(f"{where}: name must be a non-empty string")
        if name in names:
            raise SchemaError(f"{where}: duplicate family {name!r}")
        if not isinstance(schema_name, str) or not schema_name:
            raise SchemaError(f"{where}: schema must be a non-empty string")
        if (not isinstance(patterns, list) or not patterns
                or not all(isinstance(pattern, str) and pattern for pattern in patterns)):
            raise SchemaError(f"{where}: patterns must be non-empty strings")
        schema_path = os.path.join(schema_dir, schema_name)
        schema = load_json(schema_path)
        errors = check_root_schema(schema, schema_name)
        if errors:
            raise SchemaError("\n".join(errors))
        names.add(name)
        families.append({
            "name": name,
            "schema": schema,
            "schema_version": schema["x-schema-version"],
            "patterns": patterns,
        })
    return families


def load_metadata(root):
    path = os.path.join(root, "_schema_case.json")
    if not os.path.isfile(path):
        return {}
    metadata = load_json(path)
    if not isinstance(metadata, dict):
        raise SchemaError(f"{path}: expected an object")
    unknown = set(metadata) - {"mode", "template_root", "manifest", "description"}
    if unknown:
        raise SchemaError(f"{path}: unknown metadata key(s) {sorted(unknown)}")
    mode = metadata.get("mode", "source")
    if mode not in ("source", "converted"):
        raise SchemaError(f"{path}: mode must be source or converted")
    for key in ("template_root", "manifest"):
        if key in metadata and not isinstance(metadata[key], str):
            raise SchemaError(f"{path}: {key} must be a string")
    return metadata


def match_family(relative_path, families):
    for family in families:
        if any(fnmatch.fnmatchcase(relative_path, pattern)
               for pattern in family["patterns"]):
            return family
    return None


def scan_files(root, template_root=None):
    # Template-root JSON files are not config files even when the tree lives
    # under the config root (fixtures do this to keep one --root argument).
    template_prefix = None
    if template_root:
        template_prefix = os.path.realpath(template_root) + os.sep
    files = []
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames.sort()
        for name in sorted(filenames):
            if not name.endswith(".json") or name.startswith("_"):
                continue
            absolute = os.path.join(dirpath, name)
            if template_prefix and os.path.realpath(absolute).startswith(template_prefix):
                continue
            files.append(os.path.relpath(absolute, root).replace(os.sep, "/"))
    return sorted(files)


def port_names(documents):
    document = documents.get("port_config.json")
    if not isinstance(document, dict):
        return None
    config = document.get("dev_cfg")
    if not isinstance(config, dict):
        return set()
    names = set()
    for section in ("uart", "lan", "wan", "can", "di", "do"):
        port = config.get(section)
        if not isinstance(port, dict):
            continue
        attrs = port.get("attr")
        if not isinstance(attrs, list):
            continue
        for attr in attrs:
            if isinstance(attr, dict) and isinstance(attr.get("name"), str) and attr["name"]:
                names.add(attr["name"])
    return names


def device_entries(document):
    if not isinstance(document, dict):
        return
    classes = document.get("dev_class")
    if not isinstance(classes, list):
        return
    for class_index, device_class in enumerate(classes):
        if not isinstance(device_class, dict):
            continue
        devices = device_class.get("devs")
        if not isinstance(devices, list):
            continue
        for device_index, device in enumerate(devices):
            if isinstance(device, dict):
                yield class_index, device_index, device_class, device


def cross_result(status, label, detail="", errors=None):
    return {"status": status, "label": label, "detail": detail,
            "errors": errors or []}


def check_device_channels(documents, family_files, issues_by_file):
    files = family_files.get("device", [])
    if not files:
        return None
    ports = port_names(documents)
    checked = 0
    endpoints = 0
    failures = 0
    for relative_path in files:
        document = documents.get(relative_path)
        if document is None:
            continue
        for class_index, device_index, _, device in device_entries(document):
            channel = device.get("channel")
            if not isinstance(channel, str):
                continue
            if HOST_PORT_REF.match(channel):
                endpoints += 1
                continue
            if ports is None:
                continue
            checked += 1
            if channel not in ports:
                issues_by_file[relative_path]["errors"].append(issue(
                    "BAD_REFERENCE",
                    f"dev_class[{class_index}].devs[{device_index}].channel",
                    f"{channel!r} is not a port name in port_config.json"))
                failures += 1
    label = "device.channel -> port_config ports"
    if ports is None:
        return cross_result(
            "UNCHECKED", label, "port_config.json missing or unparseable")
    if failures:
        return cross_result(
            "FAIL", label,
            f"{failures} bad reference(s); {checked} symbolic channel(s) checked")
    return cross_result(
        "PASS", label,
        f"{checked} symbolic channel(s) in port set; "
        f"{endpoints} host:port endpoint(s) not local ports")


def check_dev_group(documents, issues_by_file):
    relative_path = "dev_group_cfg.json"
    document = documents.get(relative_path)
    if not isinstance(document, list):
        return None
    label = "dev_group.mp/no -> ems_lc_list / lc_devs"
    lc_list = documents.get("ems_lc_list.json")
    if not isinstance(lc_list, dict):
        return cross_result("UNCHECKED", label, "ems_lc_list.json missing or unparseable")
    lc_numbers = {
        device.get("no") for device in lc_list.get("devices", [])
        if isinstance(device, dict) and isinstance(device.get("no"), str)
    }
    failures = 0
    checked = 0
    unresolved = 0
    if isinstance(document, list):
        for group_index, group in enumerate(document):
            if not isinstance(group, dict):
                continue
            devices = group.get("devices")
            if not isinstance(devices, list):
                continue
            for device_index, device in enumerate(devices):
                if not isinstance(device, dict):
                    continue
                mp = device.get("mp")
                number = device.get("no")
                if not isinstance(mp, str):
                    continue
                if mp not in lc_numbers:
                    issues_by_file[relative_path]["errors"].append(issue(
                        "BAD_REFERENCE", f"[{group_index}].devices[{device_index}].mp",
                        f"{mp!r} is not an ems_lc_list no"))
                    failures += 1
                    continue
                lc_relative = f"lc_devs/{mp}/device_config.json"
                lc_device_config = documents.get(lc_relative)
                if not isinstance(lc_device_config, dict):
                    unresolved += 1
                    continue
                device_numbers = {
                    candidate.get("no")
                    for _, _, _, candidate in device_entries(lc_device_config)
                    if isinstance(candidate.get("no"), str)
                }
                if isinstance(number, str):
                    checked += 1
                    if number not in device_numbers:
                        issues_by_file[relative_path]["errors"].append(issue(
                            "BAD_REFERENCE",
                            f"[{group_index}].devices[{device_index}].no",
                            f"{number!r} is not in {lc_relative}"))
                        failures += 1
    if failures:
        return cross_result(
            "FAIL", label,
            f"{failures} bad reference(s); {checked} device no(s) checked; "
            f"{unresolved} LC config(s) missing")
    if unresolved:
        return cross_result(
            "UNCHECKED", label,
            f"{unresolved} LC device_config.json missing; {checked} device no(s) checked")
    return cross_result("PASS", label, f"{checked} device no(s) checked")


def check_device_templates(template_root, documents, family_files, issues_by_file):
    files = family_files.get("device", [])
    if not files:
        return None
    label = "device.template -> template tree"
    if not template_root:
        return cross_result(
            "UNCHECKED", label,
            "--template-root not given (B1 template tree missing)")
    checked = 0
    failures = 0
    for relative_path in files:
        document = documents.get(relative_path)
        if document is None:
            continue
        for class_index, device_index, _, device in device_entries(document):
            template = device.get("template")
            if not isinstance(template, str):
                continue
            resolved = (template if os.path.isabs(template)
                        else os.path.join(template_root, template))
            checked += 1
            if not os.path.isfile(resolved):
                issues_by_file[relative_path]["errors"].append(issue(
                    "BAD_REFERENCE",
                    f"dev_class[{class_index}].devs[{device_index}].template",
                    f"template not found: {template!r}"))
                failures += 1
    if failures:
        return cross_result(
            "FAIL", label, f"{failures} missing template(s); {checked} checked")
    return cross_result("PASS", label, f"{checked} template(s) found")


def template_tags(path):
    try:
        document = load_json(path)
    except SchemaError:
        return None
    tags = set()
    if isinstance(document, dict):
        points = document.get("points")
        if isinstance(points, list):
            for point in points:
                if not isinstance(point, dict):
                    continue
                for key in ("tag", "name"):
                    tag = point.get(key)
                    if isinstance(tag, str) and tag:
                        tags.add(tag)
        declared = document.get("tags")
        if isinstance(declared, list):
            for tag in declared:
                if isinstance(tag, str) and tag:
                    tags.add(tag)
    return tags or None


def device_type_templates(documents, family_files):
    mapping = {}
    for relative_path in family_files.get("device", []):
        document = documents.get(relative_path)
        if document is None:
            continue
        classes = document.get("dev_class")
        if not isinstance(classes, list):
            continue
        for device_class in classes:
            if not isinstance(device_class, dict):
                continue
            class_type = device_class.get("type")
            devices = device_class.get("devs")
            if not isinstance(class_type, str) or not isinstance(devices, list):
                continue
            for device in devices:
                if not isinstance(device, dict):
                    continue
                template = device.get("template")
                if not isinstance(template, str):
                    continue
                templates = mapping.setdefault(class_type, [])
                if template not in templates:
                    templates.append(template)
    return mapping


def pub_tag_count(document):
    entries = document.get("pub_dev_list")
    if not isinstance(entries, list):
        return 0
    total = 0
    for entry in entries:
        if isinstance(entry, dict) and isinstance(entry.get("tags"), list):
            total += sum(1 for tag in entry["tags"] if isinstance(tag, str))
    return total


def check_pub_tags(template_root, documents, family_files):
    document = documents.get("mqtt_pub_dev_list.json")
    if not isinstance(document, dict):
        return None
    label = "mqtt_pub_dev_list.tags -> templates"
    if not template_root:
        return cross_result(
            "UNCHECKED", label,
            f"{pub_tag_count(document)} tag(s) unverified; --template-root "
            "not given (B1 template tree missing)")
    entries = document.get("pub_dev_list")
    if not isinstance(entries, list):
        return cross_result("UNCHECKED", label, "pub_dev_list missing or not an array")
    mapping = device_type_templates(documents, family_files)
    failures = []
    unchecked = []
    checked = 0
    for entry_index, entry in enumerate(entries):
        if not isinstance(entry, dict):
            continue
        class_type = entry.get("type")
        tags = entry.get("tags")
        if not isinstance(class_type, str) or not isinstance(tags, list):
            continue
        templates = mapping.get(class_type, [])
        if not templates:
            unchecked.append(f"pub_dev_list[{entry_index}] type {class_type!r} has no device template")
            continue
        resolved_tags = set()
        found = False
        for template in templates:
            resolved = (template if os.path.isabs(template)
                        else os.path.join(template_root, template))
            if not os.path.isfile(resolved):
                unchecked.append(f"template {template!r} is missing")
                continue
            tags_from_template = template_tags(resolved)
            if tags_from_template is None:
                unchecked.append(f"template {template!r} has no recognizable point list")
                continue
            found = True
            resolved_tags |= tags_from_template
        if not found:
            continue
        for tag in tags:
            if not isinstance(tag, str):
                continue
            checked += 1
            if tag not in resolved_tags:
                failures.append(
                    f"tag {tag!r} for type {class_type!r} not found in "
                    f"templates {templates}")
    if failures:
        return cross_result(
            "FAIL", label, f"{len(failures)} bad tag reference(s)",
            failures)
    if unchecked:
        return cross_result(
            "UNCHECKED", label,
            f"{len(unchecked)} template(s) unverifiable; {checked} tag(s) checked")
    return cross_result("PASS", label, f"{checked} tag(s) checked")


MODBUS_ADDRESS_SPACES = {
    1: "start_bits",
    2: "start_input_bits",
    3: "start_registers",
    4: "start_input_registers",
}


def check_north_map_addresses(documents, family_files, issues_by_file):
    files = family_files.get("north_map", [])
    if not files:
        return None
    label = "north_map.point address -> start_* space"
    checked = 0
    failures = 0
    skipped = 0
    for relative_path in files:
        document = documents.get(relative_path)
        if not isinstance(document, dict):
            continue
        points = document.get("pts")
        if not isinstance(points, list):
            continue
        for pt_index, pt in enumerate(points):
            if not isinstance(pt, dict):
                continue
            point_info = pt.get("pointInfo")
            if not isinstance(point_info, list):
                continue
            if "commonAddr" in pt:
                skipped += 1
                continue
            starts = {}
            for function_code, start_key in MODBUS_ADDRESS_SPACES.items():
                start = pt.get(start_key)
                if isinstance(start, str) and start.isdigit():
                    starts[function_code] = int(start)
            for row_index, row in enumerate(point_info):
                if not isinstance(row, list) or len(row) < 6:
                    continue
                function_code = row[1]
                address = row[2]
                if not is_integer(function_code) or function_code not in starts:
                    continue
                if not isinstance(address, str) or not address.isdigit():
                    continue
                checked += 1
                # P4-04 offsets device sections by 500 beyond a per-device nb_*,
                # so only the lower bound holds for every generated section.
                start = starts[function_code]
                if int(address) < start:
                    issues_by_file[relative_path]["errors"].append(issue(
                        "BAD_REFERENCE",
                        f"pts[{pt_index}].pointInfo[{row_index}][2]",
                        f"address {address!r} is below "
                        f"{MODBUS_ADDRESS_SPACES[function_code]} {start}"))
                    failures += 1
    if failures:
        return cross_result(
            "FAIL", label,
            f"{failures} address(es) below their function-code space start; "
            f"{checked} checked")
    if not checked:
        return cross_result(
            "UNCHECKED", label,
            f"no Modbus point rows; {skipped} IEC104 group(s) skipped")
    return cross_result(
        "PASS", label,
        f"{checked} point address(es) at or above their function-code start; "
        f"{skipped} IEC104 group(s) skipped")


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--root", default=config_inventory.DEFAULT_ROOT,
                        help=f"config root (default: {config_inventory.DEFAULT_ROOT})")
    parser.add_argument("--schema-dir", default=None,
                        help="schema directory (default: config/schema next to the repo)")
    parser.add_argument("--family-map", default=None,
                        help="family map JSON (default: <schema-dir>/file_families.json)")
    parser.add_argument("--mode", choices=("source", "converted"), default=None,
                        help="source (default) or converted schema_version policy")
    parser.add_argument("--template-root", default=None,
                        help="template tree root; enables template/tag checks")
    parser.add_argument("--manifest", default=None,
                        help="snapshot manifest for --check")
    parser.add_argument("--check", action="store_true",
                        help="verify the snapshot manifest before validating")
    args = parser.parse_args()

    root = os.path.abspath(args.root)
    if not os.path.isdir(root):
        print(f"FAIL root is not a directory: {root}")
        return 1

    if args.family_map:
        family_map = args.family_map
        schema_dir = args.schema_dir or os.path.dirname(os.path.abspath(family_map))
    else:
        schema_dir = args.schema_dir or DEFAULT_SCHEMA_DIR
        family_map = os.path.join(schema_dir, "file_families.json")

    try:
        metadata = load_metadata(root)
        mode = args.mode or metadata.get("mode", "source")
        if args.template_root:
            template_root = os.path.abspath(args.template_root)
        elif metadata.get("template_root"):
            template_root = os.path.join(root, metadata["template_root"])
        else:
            template_root = None
        families = load_families(schema_dir, family_map)
    except SchemaError as exc:
        print(f"FAIL schema load: {exc}")
        return 1

    if args.check:
        manifest = args.manifest
        if manifest is None and metadata.get("manifest"):
            manifest = os.path.join(root, metadata["manifest"])
        if manifest is None:
            if os.path.realpath(root) == os.path.realpath(config_inventory.DEFAULT_ROOT):
                manifest = config_inventory.REPO_MANIFEST
            else:
                candidate = os.path.join(root, "config_inventory.tsv")
                manifest = candidate if os.path.isfile(candidate) else None
        print("== snapshot manifest")
        if manifest is None:
            print(f"FAIL no manifest for {root}; pass --manifest")
            return 1
        rows = config_inventory.scan(root)
        if config_inventory.check_manifest(manifest, rows) != 0:
            print("FAIL snapshot manifest does not match; refusing family checks")
            return 1

    issues_by_file = {}
    family_files = {family["name"]: [] for family in families}
    documents = {}
    uncovered = []
    for relative_path in scan_files(root, template_root):
        family = match_family(relative_path, families)
        if family is None:
            uncovered.append(relative_path)
            try:
                documents[relative_path] = load_json(os.path.join(root, relative_path))
            except SchemaError:
                pass
            continue
        family_files[family["name"]].append(relative_path)
        issues_by_file[relative_path] = {"errors": [], "warnings": []}
        try:
            document = load_json(os.path.join(root, relative_path))
        except SchemaError as exc:
            issues_by_file[relative_path]["errors"].append(issue(
                "PARSE_ERROR", "$", str(exc)))
            continue
        documents[relative_path] = document
        report = Validator(mode, family["schema_version"]).document(
            document, family["schema"])
        issues_by_file[relative_path]["errors"].extend(report.errors)
        issues_by_file[relative_path]["warnings"].extend(report.warnings)

    cross_checks = []
    for check in (check_device_channels, check_north_map_addresses):
        result = check(documents, family_files, issues_by_file)
        if result:
            cross_checks.append(result)
    result = check_dev_group(documents, issues_by_file)
    if result:
        cross_checks.append(result)
    result = check_device_templates(
        template_root, documents, family_files, issues_by_file)
    if result:
        cross_checks.append(result)
    pub_result = check_pub_tags(template_root, documents, family_files)
    if pub_result:
        cross_checks.append(pub_result)

    print("== families")
    for family in families:
        files = family_files[family["name"]]
        if not files:
            continue
        errors = sum(len(issues_by_file[path]["errors"]) for path in files)
        warnings = sum(len(issues_by_file[path]["warnings"]) for path in files)
        status = "FAIL" if errors else "WARN" if warnings else "PASS"
        print(f"{status:<9} {family['name']:<20} {len(files)} file(s)")
        for path in files:
            for entry in issues_by_file[path]["errors"]:
                print(f"  FAIL {path}: {format_issue(entry)}")
            for entry in issues_by_file[path]["warnings"]:
                print(f"  WARN {path}: {format_issue(entry)}")

    if uncovered:
        print("== uncovered (no schema family)")
        for path in uncovered:
            print(f"SKIP      {path}: no schema family")

    print("== cross-checks")
    for result in cross_checks:
        line = f"{result['status']:<9} {result['label']}"
        if result["detail"]:
            line += f": {result['detail']}"
        print(line)
        for message in result["errors"]:
            print(f"  FAIL {message}")

    error_count = sum(
        len(issues_by_file[path]["errors"]) for path in issues_by_file)
    warning_count = sum(
        len(issues_by_file[path]["warnings"]) for path in issues_by_file)
    for result in cross_checks:
        error_count += len(result["errors"])
    if error_count:
        print(f"RESULT FAIL: {error_count} error(s), {warning_count} warning(s), "
              f"{len(uncovered)} uncovered file(s)")
        return 1
    print(f"RESULT OK: 0 error(s), {warning_count} warning(s), "
          f"{len(uncovered)} uncovered file(s)")
    return 0


if __name__ == "__main__":
    sys.exit(main())

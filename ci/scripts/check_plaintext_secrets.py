#!/usr/bin/env python3
"""Reject plaintext secrets in tracked configuration and script files.

Scans every `git ls-files` path under --root and selects config/script text
(json, yaml/yml, ini, toml, conf, env, sh, py, Dockerfile). A sensitive key
name (pass/password/passwd/secret/token/api_key/access_key/private_key/
secret_key/client_secret, case-insensitive whole word) with a non-empty
string value is red unless the value is `$secret:<name>`, `${ENV_NAME}` or
`$ENV_NAME`. Empty values are warnings. An exact-path allowlist at
`<root>/ci/baseline/plaintext_secret_allowlist.txt` suppresses reviewed
findings; every entry must still suppress one finding or the guard reports
STALE_ALLOWLIST (shrink-only). Values are never printed.

Known limits: YAML is parsed line by line (no block scalars, anchors or
multi-line flow values); scripts are checked for literal assignments only
(not dict literals, call arguments or command substitutions); URL-embedded
credentials, encoded secrets and non-selected extensions are not scanned.
JSON line numbers are best effort when a key repeats.
"""

import argparse
import json
import os
import re
import shlex
import subprocess
import sys

SENSITIVE_WORDS = {"pass", "password", "passwd", "secret", "token"}
SENSITIVE_COMPOUNDS = {
    "apikey", "accesskey", "privatekey", "secretkey", "clientsecret",
}
KEY_PREFIXES = {"api", "access", "private", "secret"}
STATUS_VALUES = {
    "0", "1", "true", "false", "yes", "no", "ok", "fail", "failed",
    "passed", "success", "error", "skip", "skipped", "n/a", "none", "null",
}

SECRET_REF = re.compile(r"^\$secret:[A-Za-z0-9_.-]+$")
ENV_BRACED = re.compile(r"^\$\{[A-Za-z_][A-Za-z0-9_]*\}$")
ENV_BARE = re.compile(r"^\$[A-Za-z_][A-Za-z0-9_]*$")
GHA_EXPR = re.compile(r"^\$\{\{.*\}\}$")
LINE_KEY = re.compile(r'^\s*(?:-\s*)?["\']?([A-Za-z0-9_.\-]+)["\']?\s*[:=]\s*(.*?)\s*$')
ENV_ASSIGN = re.compile(r'^\s*(?:export\s+)?([A-Za-z_][A-Za-z0-9_.]*)\s*=\s*(.*?)\s*$')
SCRIPT_ASSIGN = re.compile(
    r'^\s*(?:(?:export|local|readonly)\s+|declare\s+(?:-[A-Za-z]+\s+)?)?'
    r'([A-Za-z_][A-Za-z0-9_]*)\s*=\s*(.*?)\s*$')
PY_LITERAL = re.compile(r'^([rbfuRBFU]{0,2})("""|\'\'\'|"|\')(.*)\2\s*(?:#.*)?$')
DOCKER_ENV = re.compile(r'^\s*(?:ENV|ARG)\s+(.+?)\s*$', re.IGNORECASE)

ALLOWLIST = os.path.join("ci", "baseline", "plaintext_secret_allowlist.txt")


def tokens(name):
    name = re.sub(r"(?<=[a-z0-9])(?=[A-Z])", "_", name)
    return [part for part in re.split(r"[^a-z0-9]+", name.lower()) if part]


def is_sensitive(name):
    if name.startswith(("x-", "$")):
        return False
    parts = tokens(name)
    if not parts:
        return False
    joined = "".join(parts)
    if joined in SENSITIVE_COMPOUNDS:
        return True
    if len(parts) >= 2 and parts[-1] == "key" and parts[-2] in KEY_PREFIXES:
        return True
    return parts[-1] in SENSITIVE_WORDS


def is_status_value(name, value):
    return tokens(name) == ["pass"] and value.strip().lower() in STATUS_VALUES


def classify(value, allow_bare_env):
    value = value.strip()
    if len(value) >= 2 and value[0] == value[-1] and value[0] in "\"'":
        value = value[1:-1]
    if value == "":
        return "empty"
    if SECRET_REF.match(value) or ENV_BRACED.match(value) or GHA_EXPR.match(value):
        return "placeholder"
    if allow_bare_env and ENV_BARE.match(value):
        return "placeholder"
    return "plaintext"


def add_value(findings, warnings, lineno, key, value, allow_bare_env):
    kind = classify(value, allow_bare_env)
    if kind == "empty":
        warnings.append(("WARN_SECRET_UNSET", lineno, f"key={key} empty value treated as unset"))
    elif kind == "plaintext" and not is_status_value(key, value):
        findings.append((lineno, key))


def scan_json(text):
    findings, warnings = [], []
    try:
        document = json.loads(text)
    except json.JSONDecodeError as exc:
        warnings.append(("WARN_PARSE_ERROR", exc.lineno, f"{exc.msg} (line scan fallback)"))
        fallback_findings, fallback_warnings = scan_line_config(text)
        return fallback_findings, warnings + fallback_warnings

    raw = []

    def walk(node):
        if isinstance(node, dict):
            for key, value in node.items():
                if isinstance(value, str) and is_sensitive(key):
                    raw.append((key, value))
                walk(value)
        elif isinstance(node, list):
            for item in node:
                walk(item)

    walk(document)
    if not raw:
        return findings, warnings

    key_lines = {}
    for lineno, line in enumerate(text.splitlines(), 1):
        for match in re.finditer(r'"([^"\\]+)"\s*:', line):
            key_lines.setdefault(match.group(1), []).append(lineno)
    used = {}
    for key, value in raw:
        index = used.get(key, 0)
        candidates = key_lines.get(key, [])
        if index < len(candidates):
            lineno = candidates[index]
        else:
            lineno = candidates[-1] if candidates else 1
        used[key] = index + 1
        add_value(findings, warnings, lineno, key, value, False)
    return findings, warnings


def yaml_scalar(raw):
    """Return (value, is_non_string). Line-based YAML/INI/TOML/CONF subset."""
    raw = raw.strip()
    if raw and raw[0] in "\"'":
        end = raw.find(raw[0], 1)
        if end < 0:
            return "", True  # multi-line string: outside the line parser
        return raw[1:end], False
    raw = re.sub(r"\s+#.*$", "", raw).strip()
    if raw in {"|", "|-", "|+", ">", ">-", ">+"}:
        return "", True
    if raw.startswith(("{", "[")):
        return "", True
    if re.match(r"^(?:true|false|null|~|[-+]?\d+(?:\.\d+)?)$", raw):
        return "", True
    return raw, False


def scan_line_config(text):
    findings, warnings = [], []
    for lineno, line in enumerate(text.splitlines(), 1):
        stripped = line.strip()
        if not stripped or stripped.startswith(("#", ";")):
            continue
        match = LINE_KEY.match(line)
        if not match:
            continue
        key = match.group(1)
        if not is_sensitive(key):
            continue
        value, non_string = yaml_scalar(match.group(2))
        if non_string:
            continue
        add_value(findings, warnings, lineno, key, value, False)
    return findings, warnings


def scan_env(text):
    findings, warnings = [], []
    for lineno, line in enumerate(text.splitlines(), 1):
        stripped = line.strip()
        if not stripped or stripped.startswith("#"):
            continue
        match = ENV_ASSIGN.match(line)
        if not match or not is_sensitive(match.group(1)):
            continue
        raw = match.group(2).strip()
        if raw and raw[0] in "\"'":
            end = raw.find(raw[0], 1)
            raw = raw[1:end] if end >= 0 else raw[1:]
        else:
            raw = re.sub(r"\s+#.*$", "", raw)
        add_value(findings, warnings, lineno, match.group(1), raw, True)
    return findings, warnings


def scan_docker(text):
    findings, warnings = [], []
    for lineno, line in enumerate(text.splitlines(), 1):
        stripped = line.strip()
        if not stripped or stripped.startswith("#"):
            continue
        match = DOCKER_ENV.match(line)
        if not match:
            continue
        try:
            parts = shlex.split(match.group(1))
        except ValueError:
            parts = match.group(1).split()
        if not parts:
            continue
        pairs = []
        if "=" in parts[0]:
            pairs = [part.split("=", 1) for part in parts if "=" in part]
        else:
            pairs = [(parts[0], " ".join(parts[1:]))]
        for key, value in pairs:
            if is_sensitive(key):
                add_value(findings, warnings, lineno, key, value, True)
    return findings, warnings


def python_literal(raw):
    match = PY_LITERAL.match(raw)
    if not match:
        return None
    prefix, _, inner = match.groups()
    if "f" in prefix.lower() and re.fullmatch(r"(?:\{[^{}]*\})+", inner):
        return None
    return inner


def shell_literal(raw):
    raw = raw.strip()
    if raw and raw[0] in "\"'":
        end = raw.find(raw[0], 1)
        return raw[1:end] if end >= 0 else None
    if raw.startswith("$(") or raw.startswith("`"):
        return None
    return re.sub(r"\s+#.*$", "", raw).strip()


def scan_script(text, shell):
    findings, warnings = [], []
    for lineno, line in enumerate(text.splitlines(), 1):
        stripped = line.strip()
        if not stripped or stripped.startswith("#"):
            continue
        match = SCRIPT_ASSIGN.match(line)
        if not match or not is_sensitive(match.group(1)):
            continue
        literal = shell_literal(match.group(2)) if shell else python_literal(match.group(2))
        if literal is not None:
            add_value(findings, warnings, lineno, match.group(1), literal, shell)
    return findings, warnings


def kind_of(path):
    base = os.path.basename(path)
    ext = os.path.splitext(base)[1].lower()
    if ext == ".json":
        return "json"
    if ext in {".yaml", ".yml", ".ini", ".toml", ".conf"}:
        return "line"
    if ext == ".env" or base == ".env" or base.startswith(".env."):
        return "env"
    if ext in {".sh", ".bash"}:
        return "shell"
    if ext == ".py":
        return "python"
    if base == "Dockerfile" or base.startswith("Dockerfile.") or base.endswith("Dockerfile") \
            or ext == ".dockerfile":
        return "docker"
    return None


def load_allowlist(root):
    path = os.path.join(root, ALLOWLIST)
    display = ALLOWLIST.replace(os.sep, "/")
    entries = {}
    errors = []
    if not os.path.exists(path):
        return entries, errors
    try:
        with open(path, encoding="utf-8-sig") as handle:
            lines = handle.read().splitlines()
    except (OSError, UnicodeDecodeError) as exc:
        errors.append(f"CONFIG_ERROR {display}: unreadable ({type(exc).__name__})")
        return entries, errors
    for lineno, line in enumerate(lines, 1):
        stripped = line.strip()
        if not stripped or stripped.startswith("#"):
            continue
        if "\t" not in line:
            errors.append(f"CONFIG_ERROR {display}:{lineno} entry needs '<path>\\t<reason>'")
            continue
        entry, reason = (field.strip() for field in line.split("\t", 1))
        if not entry or not reason:
            errors.append(f"CONFIG_ERROR {display}:{lineno} entry needs a path and a reason")
            continue
        if os.path.isabs(entry) or entry.startswith("..") or "/../" in entry:
            errors.append(f"CONFIG_ERROR {display}:{lineno} path must stay inside the root")
            continue
        while entry.startswith("./"):
            entry = entry[2:]
        entry = entry.replace("\\", "/")
        if entry in entries:
            errors.append(f"CONFIG_ERROR {display}:{lineno} duplicate entry for {entry}")
            continue
        entries[entry] = reason
    return entries, errors


def main():
    parser = argparse.ArgumentParser(description="Reject plaintext secrets in tracked configs and scripts.")
    parser.add_argument("--root", default=".", help="repository root")
    args = parser.parse_args()
    root = os.path.abspath(args.root)
    if not os.path.isdir(root):
        print(f"SCAN_ERROR {args.root}: not a directory")
        return 2

    proc = subprocess.run(
        ["git", "-C", root, "ls-files", "-z"],
        stdout=subprocess.PIPE, stderr=subprocess.PIPE,
    )
    if proc.returncode != 0:
        detail = proc.stderr.decode("utf-8", "replace").strip().splitlines()
        print(f"SCAN_ERROR {args.root}: git ls-files failed" + (f" ({detail[0]})" if detail else ""))
        return 2

    paths = sorted({os.fsdecode(raw) for raw in proc.stdout.split(b"\0") if raw})
    allowlist, errors = load_allowlist(root)

    findings = []
    warnings = []
    skipped = {"binary": 0, "decode": 0, "symlink": 0, "unreadable": 0}
    scanned = 0
    for path in paths:
        kind = kind_of(path)
        if not kind:
            continue
        full = os.path.join(root, path)
        if os.path.islink(full):
            skipped["symlink"] += 1
            continue
        try:
            with open(full, "rb") as handle:
                payload = handle.read()
        except OSError:
            skipped["unreadable"] += 1
            continue
        if b"\0" in payload:
            skipped["binary"] += 1
            continue
        try:
            text = payload.decode("utf-8-sig")
        except UnicodeDecodeError:
            skipped["decode"] += 1
            continue
        scanned += 1
        if kind == "json":
            file_findings, file_warnings = scan_json(text)
        elif kind == "env":
            file_findings, file_warnings = scan_env(text)
        elif kind == "docker":
            file_findings, file_warnings = scan_docker(text)
        elif kind == "shell":
            file_findings, file_warnings = scan_script(text, True)
        elif kind == "python":
            file_findings, file_warnings = scan_script(text, False)
        else:
            file_findings, file_warnings = scan_line_config(text)
        findings.extend((path, lineno, key) for lineno, key in file_findings)
        warnings.extend((path, lineno, kind_name, detail) for kind_name, lineno, detail in file_warnings)

    raw_paths = {path for path, _, _ in findings}
    stale = sorted(entry for entry in allowlist if entry not in raw_paths)
    active = [(path, lineno, key) for path, lineno, key in findings if path not in allowlist]
    suppressed = len(findings) - len(active)

    for path, lineno, key in sorted(active):
        print(f"PLAINTEXT_SECRET {path}:{lineno} key={key}")
    for entry in stale:
        print(f"STALE_ALLOWLIST {entry} reason={allowlist[entry]}")
    for error in errors:
        print(error)
    for path, lineno, kind_name, detail in sorted(warnings):
        print(f"{kind_name} {path}:{lineno} {detail}")
    print(
        f"SCANNED {scanned} files, suppressed {suppressed} allowlisted finding(s), "
        f"skipped {skipped['binary']} binary, {skipped['decode']} undecodable, "
        f"{skipped['symlink']} symlink, {skipped['unreadable']} unreadable"
    )
    return 1 if (active or stale or errors) else 0


if __name__ == "__main__":
    sys.exit(main())

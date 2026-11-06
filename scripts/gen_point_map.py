#!/usr/bin/env python3
"""Generate a C header from a product point-map JSON file.

The JSON is the data source. The header is a build artifact, so product
adapters no longer hard-code point tables in cmd/ source.
"""

import argparse
import json
import os


def emit_table(name, table):
    typ = table["type"]
    style = table.get("style", "named")
    lines = [f"static const {typ} {name}[] = {{"]
    for entry in table["entries"]:
        if style == "positional":
            lines.append("    {" + ", ".join(entry) + "},")
        else:
            fields = ", ".join(f".{key} = {value}" for key, value in entry.items())
            lines.append("    {" + fields + "},")
    lines.append("};")
    lines.append(
        f"static const size_t {name}_count = sizeof({name}) / sizeof({name}[0]);"
    )
    return "\n".join(lines)


def generate(input_path, output_path):
    with open(input_path, encoding="utf-8") as handle:
        data = json.load(handle)

    stem = os.path.splitext(os.path.basename(output_path))[0].upper()
    lines = [
        f"/* Generated from {os.path.basename(input_path)}. Do not edit. */",
        f"#ifndef {stem}_H",
        f"#define {stem}_H",
        "",
    ]
    for name, table in data["tables"].items():
        lines.append(emit_table(name, table))
        lines.append("")
    lines.append(f"#endif /* {stem}_H */")
    lines.append("")

    with open(output_path, "w", encoding="utf-8") as handle:
        handle.write("\n".join(lines))


def main():
    parser = argparse.ArgumentParser(description="Generate a point-map C header.")
    parser.add_argument("--input", required=True, help="point-map JSON")
    parser.add_argument("--output", required=True, help="generated header")
    args = parser.parse_args()
    generate(args.input, args.output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

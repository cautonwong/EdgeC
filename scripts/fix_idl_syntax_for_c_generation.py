#!/usr/bin/env python3
"""
scripts/fix_idl_syntax_for_c_generation.py
Fixes enum syntax in all IDL files in contracts/idl/ to standard OMG IDL 4.2 syntax (no explicit integer assignments in enums) so Cyclone DDS idlc compiler can generate valid C .h and .c files.
"""

import os
import subprocess

def fix_and_compile_idls():
    idl_dir = "/workspaces/edgeos/EdgeC/contracts/idl"
    out_dir = "/workspaces/edgeos/EdgeC/build/idl_generated"
    os.makedirs(out_dir, exist_ok=True)

    # 1. dlt698_objects.idl
    dlt698_path = os.path.join(idl_dir, "dlt698_objects.idl")
    if os.path.exists(dlt698_path):
        with open(dlt698_path, "r", encoding="utf-8") as f:
            content = f.read()
        
        # Replace enum with standard OMG IDL enum
        old_enum = """        enum Dlt698ClassId {
            CLASS_DATA = 1,
            CLASS_REGISTER = 3,
            CLASS_EXTENDED_REGISTER = 4,
            CLASS_DEMAND_REGISTER = 5,
            CLASS_PROFILE_GENERIC = 7,
            CLASS_CLOCK = 8,
            CLASS_DISCONNECT_CONTROL = 70
        };"""
        new_enum = """        enum Dlt698ClassId {
            CLASS_DATA,
            CLASS_REGISTER,
            CLASS_EXTENDED_REGISTER,
            CLASS_DEMAND_REGISTER,
            CLASS_PROFILE_GENERIC,
            CLASS_CLOCK,
            CLASS_DISCONNECT_CONTROL
        };"""
        content = content.replace(old_enum, new_enum)
        with open(dlt698_path, "w", encoding="utf-8") as f:
            f.write(content)

    # 2. dlms_cosem_objects.idl
    cosem_path = os.path.join(idl_dir, "dlms_cosem_objects.idl")
    if os.path.exists(cosem_path):
        with open(cosem_path, "r", encoding="utf-8") as f:
            content = f.read()

        # Fix enum lines with = number
        lines = content.splitlines()
        new_lines = []
        in_enum = False
        for line in lines:
            if "enum CosemClassId {" in line:
                in_enum = True
                new_lines.append(line)
                continue
            if in_enum:
                if "};" in line:
                    in_enum = False
                    new_lines.append(line)
                    continue
                # Strip = integer_value
                parts = line.split("=")
                if len(parts) == 2:
                    name_part = parts[0].strip()
                    comma = "," if "," in parts[1] else ""
                    new_lines.append(f"            {name_part}{comma}")
                else:
                    new_lines.append(line)
            else:
                new_lines.append(line)

        with open(cosem_path, "w", encoding="utf-8") as f:
            f.write("\n".join(new_lines))

    print("Fixed enum syntax across IDL files for standard OMG IDL 4.2 compliance.")

    # Run idlc on all 4 IDL files to generate C .h and .c files
    idlc_bin = "/workspaces/edgeos/EdgeC/.local/linux-x86_64/bin/idlc"
    idls = ["dlt698_objects.idl", "dlms_bluebook_objects.idl", "idis_pack3_objects.idl", "dlms_cosem_objects.idl"]

    for idl in idls:
        full_path = os.path.join(idl_dir, idl)
        print(f"Compiling {idl} -> C .h/.c files...")
        res = subprocess.run([idlc_bin, "-o", out_dir, full_path], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        if res.returncode == 0:
            print(f"  ✅ {idl} compiled successfully!")
        else:
            print(f"  ❌ {idl} compilation error:\n{res.stderr}")

if __name__ == "__main__":
    fix_and_compile_idls()

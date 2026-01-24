#!/usr/bin/env python3
import subprocess
import json
import os
import sys

# 更加鲁棒的路径探测
current_file = os.path.abspath(__file__)
tests_dir = os.path.dirname(current_file)
compiler_dir = os.path.abspath(os.path.join(tests_dir, ".."))
project_root = os.path.abspath(os.path.join(compiler_dir, "../../"))

BIN = os.path.join(compiler_dir, "contract-compiler")
CASES_DIR = os.path.join(tests_dir, "cases")
GEN_DIR = os.path.join(tests_dir, "generated")

def run_compiler(schema, product):
    cmd = [
        BIN,
        "-schema", os.path.join(CASES_DIR, schema),
        "-product", os.path.join(CASES_DIR, product),
        "-base", project_root,
        "-out", GEN_DIR
    ]
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode != 0:
        print(f"❌ Compiler Panic:\n{res.stderr}")
        return False
    return True

def test_basic_consistency():
    print("🧪 Testing Basic Consistency (Modbus V5.0)...")
    if not run_compiler("base_tags.yaml", "product_v5.yaml"):
        sys.exit(1)
    
    manifest_path = os.path.join(GEN_DIR, "omnis_manifest.json")
    with open(manifest_path, 'r') as f:
        data = json.load(f)
    assert data["product"] == "TEST_COMPILER_PROD"
    print("  ✅ Manifest JSON Schema: OK")

    h_path = os.path.join(GEN_DIR, "contracts.h")
    with open(h_path, 'r') as f:
        content = f.read()
        assert "TAG_TEST_VOLTAGE = 1001" in content
    print("  ✅ C Enum Header: OK")

def test_compilability():
    print("🧪 Testing Code Compilability (GCC Verification)...")
    test_c = os.path.join(GEN_DIR, "verify_build.c")
    with open(test_c, "w") as f:
        f.write("#include \"contracts.h\"\n")
        f.write("#include \"edge_common.h\"\n")
        f.write("int main() { return 0; }\n")
    
    cmd = [
        "gcc", "-c", test_c,
        "-I", GEN_DIR,
        "-I", os.path.join(project_root, "include"),
        "-o", os.path.join(GEN_DIR, "verify_build.o")
    ]
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode != 0:
        print(f"❌ Compilation Error in Header Check:\n{res.stderr}")
        sys.exit(1)
    print("  ✅ GCC Syntax Check (Header): OK")

    # [Upgrade] 增加对 contracts.c 实现文件的编译验证
    print("🧪 Testing Implementation Compilability (contracts.c)...")
    c_src = os.path.join(GEN_DIR, "contracts.c")
    cmd_impl = [
        "gcc", "-c", c_src,
        "-D_GNU_SOURCE", # 启用标准扩展
        "-I", GEN_DIR,
        "-I", os.path.join(project_root, "include"),
        "-o", os.path.join(GEN_DIR, "contracts.o")
    ]
    res_impl = subprocess.run(cmd_impl, capture_output=True, text=True)
    if res_impl.returncode != 0:
        print(f"❌ Compilation Error in contracts.c:\n{res_impl.stderr}")
        sys.exit(1)
    print("  ✅ GCC Syntax Check (Implementation): OK")

if __name__ == "__main__":
    os.makedirs(GEN_DIR, exist_ok=True)
    test_basic_consistency()
    test_compilability()
    print("\n✨ All Compiler Integration Tests Passed!")
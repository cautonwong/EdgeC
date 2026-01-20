#!/usr/bin/env python3
import pytest
import sys
import os
import argparse

# 确保能找到 tools/matrix 库
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

def main():
    parser = argparse.ArgumentParser(description="Omnis Matrix Test Runner")
    parser.add_argument("--server", default="localhost:50051", help="Matrix Server endpoint")
    parser.add_argument("--product", required=True, help="Product name (e.g. iems)")
    parser.add_argument("--scenario", help="Path to YAML scenario file")
    parser.add_argument("pytest_args", nargs=argparse.REMAINDER, help="Additional pytest arguments")
    
    args = parser.parse_args()

    # 将参数注入到环境变量中，供 conftest.py 读取
    os.environ["OMNIS_SERVER"] = args.server
    os.environ["OMNIS_PRODUCT"] = args.product
    if args.scenario:
        os.environ["OMNIS_SCENARIO"] = args.scenario

    # 构造 pytest 命令
    # 默认运行产品目录下的 matrix 测试
    target_dir = f"cmd/{args.product}/matrix"
    pytest_cmd = [target_dir] + args.pytest_args

    print(f"🚀 Omnis Test Runner: Connecting to {args.server} for {args.product}...")
    return pytest.main(pytest_cmd)

if __name__ == "__main__":
    sys.exit(main())

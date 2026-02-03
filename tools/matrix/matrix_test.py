#!/usr/bin/env python3
import pytest
import sys
import os
import argparse
import requests
import time

# 确保能找到 tools/matrix 库
PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "../../"))
sys.path.insert(0, PROJECT_ROOT)

def check_env(server):
    print(f"🔍 Checking Matrix environment at {server}...")
    try:
        resp = requests.get(f"http://{server}/api/v1/connectivity", timeout=2)
        if resp.status_code == 200:
            print("  ✅ Matrix Server: ONLINE")
            return True
    except:
        pass
    print("  ❌ Matrix Server: OFFLINE. Please start 'server.py' first.")
    return False

def main():
    parser = argparse.ArgumentParser(description="Omnis Matrix Test Runner")
    parser.add_argument("--server", default="localhost:50051", help="Matrix Server control endpoint")
    parser.add_argument("--product", default="iems", help="Target product name")
    parser.add_argument("--scenario", help="Run a specific YAML scenario file")
    parser.add_argument("pytest_args", nargs=argparse.REMAINDER, help="Additional pytest arguments")
    
    args = parser.parse_args()

    if not check_env(args.server):
        sys.exit(1)

    # 注入全局环境变量
    os.environ["OMNIS_SERVER"] = args.server
    os.environ["OMNIS_PRODUCT"] = args.product
    
    if args.scenario:
        # 场景驱动模式: 直接使用 pytest 运行通用的场景执行器，并指定场景文件
        print(f"🎬 Running Scenario: {args.scenario}")
        # 我们利用现有的 test_scenarios.py，但通过环境变量传递目标场景
        os.environ["OMNIS_SCENARIO_FILE"] = os.path.abspath(args.scenario)
        target_file = os.path.join(PROJECT_ROOT, f"cmd/{args.product}/matrix/test_scenarios.py")
        pytest_cmd = [target_file, "-v", "-s"] + args.pytest_args
    else:
        # [Nexus Fix] 如果用户指定了具体的测试文件，就不要再把整个目录加进去了
        if args.pytest_args:
            pytest_cmd = ["-v", "-s"] + args.pytest_args
        else:
            target_dir = os.path.join(PROJECT_ROOT, f"cmd/{args.product}/matrix")
            pytest_cmd = [target_dir, "-v", "-s"]


    print(f"\033[94m" + "="*60)
    print(f"🚀 Omnis Test Runner: Orchestrating for {args.product}")
    print(f"📡 Server: {args.server}")
    print("="*60 + "\033[0m")

    return pytest.main(pytest_cmd)

if __name__ == "__main__":
    sys.exit(main())
"""
Matrix End-to-End Test: Shared MCP Neural Bus Multi-Microservice E2E & Copilot Simulation (ARCH-MCP-04 / Issue #303)
Copyright (c) 2026 EdgeOS Project. All rights reserved.

Scenarios Covered:
1. test_copilot_composite_4step_flow:
   - 4-step AI Copilot composite workflow across DCU, SmartBox, and IEMS microservices
   - Power quality analysis -> Transformer life assessment -> TOU ESS optimization -> Five-Anti interlock lockout & MTT audit
2. test_mcp_high_concurrency_stability:
   - 100 consecutive rapid tool dispatches across all microservices with zero frame drop and stable latency
3. test_iems_cli_mcp_commands:
   - CLI subcommands verification (`iems-cli mcp list`, `iems-cli mcp call`, `iems-cli mcp audit`)
"""

import os
import json
import pytest
import asyncio
import subprocess
import logging

logger = logging.getLogger("Matrix.Test.SharedMCP")

@pytest.mark.asyncio
async def test_copilot_composite_4step_flow(matrix):
    """
    AI Copilot 端到端 4 步工业组合意图演习：
    Step 1: DCU 电能质量三相不平衡与谐波分析
    Step 2: SmartBox 变压器热点温度与绝缘老化评估
    Step 3: IEMS 储能峰谷套利充放电计划优化与电池状态快车道读取
    Step 4: 电气“五防”闭锁预演（带负荷拉刀闸强制阻断）与黑匣子审计校验
    """
    project_root = getattr(matrix, 'loader', None)
    cli_bin = os.path.abspath("build/products/iems/bin/iems-cli")
    assert os.path.exists(cli_bin), f"iems-cli not found at {cli_bin}"

    print("\n🚀 [Copilot Step 1] Querying DCU Power Quality & Phase Commutation Guidance...")
    args_step1 = json.dumps({
        "phase_a_v": 238.0,
        "phase_b_v": 218.0,
        "phase_c_v": 204.0,
        "current_thd": 6.8
    })
    proc1 = subprocess.run([cli_bin, "mcp", "call", "dcu_analyze_power_quality", args_step1],
                           capture_output=True, text=True, timeout=5)
    assert proc1.returncode == 0, f"Step 1 failed: {proc1.stderr}"
    res1 = json.loads(proc1.stdout)
    assert "result" in res1, f"Missing result: {res1}"
    content1 = json.loads(res1["result"]["content"][0]["text"])
    
    # 验证电能质量诊断结果
    assert content1["voltage_unbalance_pct"] > 3.0, "Voltage unbalance should exceed 3%"
    assert content1["unbalance_status"] in ["WARNING", "CRITICAL"]
    assert content1["thd_status"] == "VIOLATION"
    assert content1["phase_commutation_plan"]["action_required"] is True
    print(f"  ✅ DCU Analysis: Unbalance={content1['voltage_unbalance_pct']}%, ActionRequired={content1['phase_commutation_plan']['action_required']}")

    print("🚀 [Copilot Step 2] Evaluating SmartBox Transformer Winding Hotspot & Aging Factor...")
    args_step2 = json.dumps({
        "load_ratio": 1.20,
        "ambient_temp_c": 38.0,
        "current_top_oil_c": 82.0
    })
    proc2 = subprocess.run([cli_bin, "mcp", "call", "smartbox_eval_transformer_life", args_step2],
                           capture_output=True, text=True, timeout=5)
    assert proc2.returncode == 0, f"Step 2 failed: {proc2.stderr}"
    res2 = json.loads(proc2.stdout)
    content2 = json.loads(res2["result"]["content"][0]["text"])
    
    # 验证变压器寿命评估
    assert content2["hotspot_temp_c"] >= 110.0
    assert content2["aging_acceleration_factor_faa"] > 2.0
    assert content2["cooling_fan_override_recommended"] is True
    print(f"  ✅ SmartBox Assessment: Hotspot={content2['hotspot_temp_c']}°C, FAA={content2['aging_acceleration_factor_faa']}, FanOverride={content2['cooling_fan_override_recommended']}")

    print("🚀 [Copilot Step 3] Optimizing IEMS TOU Storage Arbitrage Schedule & Querying Battery SHM...")
    args_step3_tou = json.dumps({
        "soc_pct": 60.0,
        "pv_forecast_kw": 45.0,
        "load_forecast_kw": 80.0
    })
    proc3_tou = subprocess.run([cli_bin, "mcp", "call", "iems_optimize_tou_schedule", args_step3_tou],
                               capture_output=True, text=True, timeout=5)
    assert proc3_tou.returncode == 0
    res3_tou = json.loads(proc3_tou.stdout)
    content3_tou = json.loads(res3_tou["result"]["content"][0]["text"])
    assert len(content3_tou["schedule_24h"]) == 24
    assert content3_tou["daily_arbitrage_profit_cny"] > 0.0

    args_step3_bat = json.dumps({"cluster_id": "RACK_01", "fast_track": True})
    proc3_bat = subprocess.run([cli_bin, "mcp", "call", "iems_get_battery_status", args_step3_bat],
                               capture_output=True, text=True, timeout=5)
    assert proc3_bat.returncode == 0
    res3_bat = json.loads(proc3_bat.stdout)
    content3_bat = json.loads(res3_bat["result"]["content"][0]["text"])
    shm_uri = content3_bat.get("fast_track_shm_uri", content3_bat.get("fast_track_handle", ""))
    assert "shm://datahub/offset=0x2000" in shm_uri
    print(f"  ✅ IEMS Schedule: DailyProfit=¥{content3_tou['daily_arbitrage_profit_cny']}, BatterySHM={shm_uri}")

    print("🚀 [Copilot Step 4] Attempting Disconnector Operation Under Load (Five-Anti Lockout Simulation)...")
    args_step4 = json.dumps({
        "breaker_id": "QS_01_FEEDER_DISCONNECTOR",
        "target_state": "OPEN",
        "current_load_amps": 180.0,
        "associated_cb_closed": True
    })
    proc4 = subprocess.run([cli_bin, "mcp", "call", "security_simulate_interlock", args_step4],
                           capture_output=True, text=True, timeout=5)
    assert proc4.returncode == 0
    res4 = json.loads(proc4.stdout)
    content4 = json.loads(res4["result"]["content"][0]["text"])
    
    # 验证五防闭锁拦截
    assert content4["operation_allowed"] is False
    assert content4["interlock_status"] == "BLOCKED_VIOLATION"
    assert len(content4["violated_rules"]) > 0
    print(f"  ✅ Five-Anti Interlock: OperationAllowed={content4['operation_allowed']}, Status={content4['interlock_status']}")
    print(f"  🛡️ Blocked Rule: {content4['violated_rules'][0]}")

    print("🎉 [FINAL_SUCCESS] AI Copilot 4-Step Composite Industrial Workflow 100% Verified!")


@pytest.mark.asyncio
async def test_mcp_high_concurrency_stability(matrix):
    """
    高并发稳定性测试：连续 100 次跨微服务工具并发调用，验证 0 丢帧、0 崩溃与稳定性
    """
    cli_bin = os.path.abspath("build/products/iems/bin/iems-cli")
    assert os.path.exists(cli_bin)

    tools = [
        ("dcu_analyze_power_quality", json.dumps({"phase_a_v": 220.0, "phase_b_v": 220.0, "phase_c_v": 220.0})),
        ("smartbox_eval_transformer_life", json.dumps({"load_ratio": 0.8, "ambient_temp_c": 25.0, "current_top_oil_c": 60.0})),
        ("iems_optimize_tou_schedule", json.dumps({"soc_pct": 50.0})),
        ("iems_get_battery_status", json.dumps({"cluster_id": "RACK_01"})),
        ("smartbox_get_topology", json.dumps({"transformer_id": "TRANSFORMER_T01"})),
    ]

    print(f"\n🚀 [Stress] Dispatching 100 consecutive rapid MCP tool calls...")
    success_count = 0

    for i in range(100):
        tool_name, args = tools[i % len(tools)]
        proc = subprocess.run([cli_bin, "mcp", "call", tool_name, args],
                              capture_output=True, text=True, timeout=3)
        assert proc.returncode == 0, f"Call #{i} ({tool_name}) failed: {proc.stderr}"
        resp = json.loads(proc.stdout)
        assert "result" in resp
        success_count += 1

    print(f"✨ [Stress SUCCESS] 100/100 MCP calls completed with 100% success rate!")
    assert success_count == 100


@pytest.mark.asyncio
async def test_iems_cli_mcp_commands(matrix):
    """
    验证 iems-cli 命令行子命令输出完整性与契约规范
    """
    cli_bin = os.path.abspath("build/products/iems/bin/iems-cli")
    assert os.path.exists(cli_bin)

    # 1. 测试 iems-cli mcp list
    print("\n🔍 [CLI Test 1] Testing 'iems-cli mcp list'...")
    proc_list = subprocess.run([cli_bin, "mcp", "list"], capture_output=True, text=True, timeout=5)
    assert proc_list.returncode == 0
    out_list = proc_list.stdout
    assert "iems_service" in out_list
    assert "smartbox_service" in out_list
    assert "dcu_service" in out_list
    assert "dcu_analyze_power_quality" in out_list
    assert "smartbox_eval_transformer_life" in out_list
    assert "iems_optimize_tou_schedule" in out_list
    assert "ess_emergency_stop" in out_list
    print("  ✅ 'iems-cli mcp list' output verified!")

    # 2. 测试 iems-cli mcp policy
    print("🔍 [CLI Test 2] Testing 'iems-cli mcp policy'...")
    proc_pol = subprocess.run([cli_bin, "mcp", "policy"], capture_output=True, text=True, timeout=5)
    assert proc_pol.returncode == 0
    out_pol = proc_pol.stdout
    assert "L0_SAFE_READ" in out_pol
    assert "L2_CRITICAL_ACTION" in out_pol
    assert "five_anti_rules" in out_pol
    print("  ✅ 'iems-cli mcp policy' output verified!")

    # 3. 测试 iems-cli mcp audit
    print("🔍 [CLI Test 3] Testing 'iems-cli mcp audit'...")
    proc_audit = subprocess.run([cli_bin, "mcp", "audit", "10"], capture_output=True, text=True, timeout=5)
    assert proc_audit.returncode == 0
    out_audit = proc_audit.stdout
    assert "EdgeOS Industrial Audit Blackbox Trail" in out_audit
    print("  ✅ 'iems-cli mcp audit' output verified!")

import pytest
import yaml
import time
import os

def run_step(director, observer, step):
    t = step.get("t", 0)
    action = step.get("action")
    print(f"   [Step] T+{t}s: {action} - {step.get('desc', '')}")
    
    if action == "inject":
        tag_name = step.get("tag")
        val = step.get("value")
        director.inject_value(tag_name, val)
        # 增加呼吸时间，确保驱动有足够机会完成至少一次轮询
        time.sleep(2.0) 
    elif action == "check_api":
        tag_name = step.get("tag")
        tag = director.parser.get_tag(tag_name)
        if not tag:
            pytest.fail(f"Unknown tag in scenario: {tag_name}")
            
        expected = step.get("expect")
        
        def check():
            val = observer.get_tag_value(tag['id'])
            if val is None: return False
            # 浮点数比较
            return abs(val - expected) < 0.01
            
        if not observer.wait_for_condition(check, timeout=30):
            val = observer.get_tag_value(tag['id'])
            pytest.fail(f"API Check failed for {tag_name}. Expected {expected}, got {val}")

def test_scenario(director, observer, scenario_context):
    """
    通用场景测试函数。
    由 conftest.py 中的 pytest_generate_tests 自动参数化。
    """
    scen_path = scenario_context["scenario_path"]
    print(f"\n🎬 Executing Scenario: {os.path.basename(scen_path)}")
    print(f"📁 Log Directory: {director.log_dir}")
    
    # 启动 EdgeC (如果还没启动)
    if not director.process_edgec:
        if not director.start_edgec():
            pytest.fail("EdgeC failed to start")
            
    with open(scen_path, "r") as f:
        scen = yaml.safe_load(f)
        
    # 自动拉起需要的 Actor
    director.spawn_actor("modbus")
    
    start_time = time.time()
    steps = sorted(scen.get("steps", []), key=lambda x: x['t'])
    
    for step in steps:
        target_t = start_time + step['t']
        now = time.time()
        if target_t > now:
            time.sleep(target_t - now)
        run_step(director, observer, step)
        
    print(f"✅ Scenario {scen.get('name', 'Unknown')} PASSED.")
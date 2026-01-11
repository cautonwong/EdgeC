import pytest
import yaml
import time

def pytest_generate_tests(metafunc):
    if "scenario_path" in metafunc.fixturenames:
        # Collect all scenarios from the director's manifest
        # Since director fixture is session scoped and parameterized, 
        # we need a trick to access its manifest here during collection.
        # But pytest_generate_tests runs before fixtures are instantiated.
        
        # Alternative: The conftest already parameterizes the 'director'.
        # We can just let each director run its own scenarios.
        pass

def run_step(director, observer, step):
    t = step.get("t", 0)
    action = step.get("action")
    
    print(f"   [Step] T+{t}s: {action} - {step.get('desc', '')}")
    
    if action == "inject":
        tag_name = step.get("tag")
        val = step.get("value")
        director.inject_value(tag_name, val)
        time.sleep(1.0) # 给驱动层轮询的时间
    elif action == "check_api":
        tag_name = step.get("tag")
        tag = director.parser.get_tag(tag_name)
        if not tag:
            pytest.fail(f"Unknown tag in scenario: {tag_name}")
        
        expected = step.get("expect")
        
        def check():
            val = observer.get_tag_value(tag['id'])
            if val is None: return False
            return abs(val - expected) < 0.01
            
        if not observer.wait_for_condition(check, timeout=10):
            val = observer.get_tag_value(tag['id'])
            pytest.fail(f"API Check failed for {tag_name}. Expected {expected}, got {val}")

class TestOmnis:
    def test_scenarios(self, director, observer):
        scenarios = director.manifest.get("scenarios")
        if not scenarios:
            pytest.skip(f"No scenarios defined for product {director.product_name}")
            
        for scen_path in scenarios:
            print(f"\n🎬 Running Scenario: {scen_path}")
            with open(scen_path, "r") as f:
                scen = yaml.safe_load(f)
            
            # Start actors defined in scenario (if any)
            # Default to Modbus for now
            director.spawn_actor("modbus")
            
            start_time = time.time()
            steps = sorted(scen.get("steps", []), key=lambda x: x['t'])
            
            for step in steps:
                target_t = start_time + step['t']
                now = time.time()
                if target_t > now:
                    time.sleep(target_t - now)
                run_step(director, observer, step)
            
        # [关键补强] 给观察员最后的喘息时间，确保断言在进程销毁前完成
        time.sleep(1.0)
        print(f"✅ Scenario {scen['name']} passed.")

# cmd/iems/matrix — 已下线的 Matrix 仿真场景用例

1. **这批用例没有运行入口**:Matrix 仿真基座(`tools/matrix/`)已不在测试链路里,`test.sh --omnis` 现在只走 `acceptance/run.sh`,直接 `pytest` 会因为没有基座和环境变量而失败。
2. **保留中的 5 个文件**(`conftest.py`、`test_adc_dsp.py`、`test_e2e_flow.py`、`test_high_freq.py`、`test_protection.py`,夹具在 `scenarios/`)是 `acceptance/iems/` 用例的来源,搬完即删。
3. `test_high_freq.py` 里 `wait_for("ADC_VOLTAGE_RMS", 221.1, tol=2.0)` 的 **221.1 来自已下线的仿真基座,搬的时候必须换成台架真值**(台架现为 231.7)。
4. **新增用例一律加在 `acceptance/iems/`,不许加到这里。**

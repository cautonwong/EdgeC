结论先行：**强烈建议使用 `pytest`。**

面对 **“多协议、多设备、复杂交互”** 的 P2 阶段（集成测试），`pytest` 不是唯一的选择，但它是目前工业界 **性价比最高、扩展性最强** 的选择。

如果不使用 `pytest`（或类似的 Python 框架），你将陷入 **“为每个协议写一套测试工具”** 的泥潭。

以下是针对你 **“协议多”** 这一痛点的 **通用测试架构设计**。

---

### 一、 为什么要用 pytest？（针对多协议场景）

你担心的点是：“我有 Modbus, DLMS, IEC104, CAN... 用 pytest 能搞定吗？”

**答案是：正因为协议多，才必须用 pytest。**

1.  **统一的测试编排**：
    *   不管是 Modbus 还是 DLMS，测试的**业务逻辑**是一样的：
        `模拟器设值 -> 网关采集 -> 验证 MQTT 结果`。
    *   pytest 允许你写 **一套测试逻辑**，然后通过 **参数化 (Parametrization)** 驱动不同的协议模拟器。

2.  **丰富的协议库生态**：
    *   Python 拥有最全的协议栈支持：
        *   Modbus -> `pymodbus`
        *   DLMS -> `gurux_dlms`
        *   CAN -> `python-can`
        *   IEC104 -> `iec104_python` (或类似库)
    *   **用 C 写模拟器太慢了**，用 Python 几行代码就能起一个 Server。

3.  **数据驱动 (Data-Driven)**：
    *   pytest 可以直接读取你的 **YAML 契约文件**，自动生成几百个测试用例。你不需要手写 `test_voltage`, `test_current`...

---

### 二、 P2 阶段架构设计：统一模拟器抽象层

为了应对“多协议”，我们需要在 Python 测试代码中设计一个 **适配器层 (Adapter Layer)**。

#### 1. 架构图

```text
[ Contract YAML ] --(加载)--> [ Pytest Runner ]
                                     |
           +-------------------------+-------------------------+
           | (统一控制接口: set_value / get_value)             |
           v                                                 v
[ Modbus Simulator ]      [ DLMS Simulator ]       [ MQTT Checker ]
(基于 pymodbus)            (基于 gurux)             (paho-mqtt)
           |                       |                         ^
           | (TCP:502)             | (TCP:4059)              |
           v                       v                         |
+------------------------------------------------------------+
|                    EdgeOS (被测对象)                        |
| [Modbus Driver]       [DLMS Driver]          [Export Svc]  |
+------------------------------------------------------------+
```

#### 2. Python 代码实现：模拟器抽象

在 `tests/common/simulators.py` 中：

```python
from abc import ABC, abstractmethod

# 1. 定义通用模拟器接口
class BaseSimulator(ABC):
    @abstractmethod
    def start(self):
        pass

    @abstractmethod
    def stop(self):
        pass

    @abstractmethod
    def set_tag_value(self, tag_name: str, value):
        """
        根据 YAML 定义，将 Tag 的物理值写入模拟器的寄存器
        """
        pass

# 2. Modbus 实现
class ModbusSim(BaseSimulator):
    def __init__(self, yaml_config):
        self.map = load_modbus_map(yaml_config)
        # 初始化 pymodbus ServerContext...

    def set_tag_value(self, tag_name, value):
        reg_info = self.map[tag_name]
        # 自动处理 Scale (例如 220.5 -> 2205)
        raw_val = int(value / reg_info['scale'])
        self.context.setValues(3, reg_info['addr'], [raw_val])

# 3. DLMS 实现
class DlmsSim(BaseSimulator):
    def __init__(self, yaml_config):
        # 初始化 Gurux Server...
        pass
        
    def set_tag_value(self, tag_name, value):
        obis = self.map[tag_name]['obis']
        # 调用 Gurux 库更新内部对象的值
        self.server.objects.findByLN(obis).value = value
```

---

### 三、 编写“万能”测试用例

有了上面的抽象，你只需要写 **一个** 测试文件，就能测所有协议。

**`tests/integration/test_data_flow.py`**

```python
import pytest
import yaml

# 1. 从 YAML 加载测试数据
# 格式: [(protocol, tag_name, test_value), ...]
def load_test_cases():
    cases = []
    # 读取 iems.yaml, sg_el.yaml...
    # 解析出所有 enabled=true 的 Tag
    # 生成测试数据：("modbus", "VOLTAGE_A", 220.0), ("dlms", "ENERGY_TOTAL", 1000.5)
    return cases

@pytest.fixture(scope="module")
def env(request):
    # 启动 EdgeOS 和 所有模拟器
    sims = {
        "modbus": ModbusSim("modbus.yaml"),
        "dlms": DlmsSim("dlms.yaml")
    }
    edgeos = EdgeOSProcess()
    edgeos.start()
    yield sims, edgeos
    edgeos.stop()

# 2. 参数化测试：这是 pytest 的杀手锏
# 它会自动为列表中的每一项生成一个独立的测试报告
@pytest.mark.parametrize("proto, tag, val", load_test_cases())
def test_acquisition(env, proto, tag, val):
    sims, _ = env
    mqtt = MqttClient()
    
    # A. 在模拟器中造数据
    sims[proto].set_tag_value(tag, val)
    
    # B. 在 MQTT 等待数据
    # 期望收到: { "name": "VOLTAGE_A", "val": 220.0 ... }
    topic = f"telemetry/{tag}"
    payload = mqtt.wait_for_message(topic, timeout=5)
    
    # C. 断言
    assert payload is not None, f"Timeout waiting for {tag}"
    assert abs(payload['val'] - val) < 0.001, "Value mismatch!"
```

---

### 四、 针对复杂协议（如 DLMS）的特殊优势

如果用 C 语言写测试桩（Mock），对于 Modbus 这种简单的寄存器数组还行，但对于 **DLMS** 这种有复杂状态机、关联握手、分帧逻辑的协议，**用 C 写一个模拟器简直是灾难**。

*   **C 语言写 DLMS 模拟器**：你需要处理 AARQ 解析、HLS 密文计算、帧拼接... 开发工作量巨大。
*   **Python (pytest) 写**：直接引用 `gurux_dlms` 库，它已经封装好了 Server 对象，你只需要 `server.add_object(obis, value)`，它自动处理所有的握手和加密细节。

**结论**：
在 P2 阶段，**Python (pytest) 是唯一能以低成本模拟复杂协议（DLMS/IEC104）的方案**。

---

### 五、 总结

1.  **必须用 pytest**：它是粘合 YAML 契约、Python 协议库、MQTT 校验和 EdgeOS 二进制的最佳胶水。
2.  **不要为每个协议写测试**：编写一个通用的 `BaseSimulator`，利用 pytest 的 `parametrize` 功能，让测试用例由 YAML 自动生成。
3.  **核心价值**：这套测试框架一旦搭好，哪怕以后新增了 CAN 协议，你只需要写一个 `CanSim` 类，其他的测试逻辑、CI 流程完全不用动。

这是**“契约驱动测试 (Contract-Driven Testing)”**的最佳落地姿势。
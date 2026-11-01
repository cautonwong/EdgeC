"""项目侧 conftest:补上"按设备参数化"的那套钩子。

平台插件只提供 composition_root 等会话级 fixture;把同一个用例按清单里每台
同类型设备各展开一次,靠的是项目自己的 pytest_generate_tests + 泛型 fixture。
少了这一段,用例照样能跑,但报告里 devices 是 0/0 —— 平台不知道这条用例
验的是哪台设备,结论只能落到"未完成"。
"""
from __future__ import annotations

import pytest

# import 组合根会连带 import 所有 driver 模块,让 @register_device 真的执行一遍。
from testplatform.application.event_relay import EventEmitFailed, drain_device_events
from testplatform.cases import case_of
from testplatform.composition.root import TEST_INVALID_PREFIX, UpstreamFault  # noqa: F401
from testplatform.config.loader import load_device_config
from testplatform.config.manifest import parse_manifest
from testplatform.drivers.registry import registered_types

# 北向用例要用的 broker 与观察器在平台仓库的 tests_support 下（那是平台侧的基础设施，
# 不该复制到产品仓库里）。acceptance/run.sh 只把 PLATFORM/src 放进 PYTHONPATH，
# 平台仓库根不在里面，所以这里自己补一次。
import os as _os
import sys as _sys


def _platform_root() -> str:
    candidates = [
        _os.environ.get("TESTPLATFORM_ROOT"),
        _os.environ.get("TESTPLATFORM"),
        "/workspaces/test-platform",
    ]
    for c in candidates:
        if c and _os.path.isdir(_os.path.join(c, "tests_support")):
            return c
    return ""


_PLATFORM_ROOT = _platform_root()
if _PLATFORM_ROOT and _PLATFORM_ROOT not in _sys.path:
    _sys.path.insert(0, _PLATFORM_ROOT)


@pytest.fixture(autouse=True)
def _drain_device_events_after_test(composition_root, event_sink, request):
    """每条用例结束后把设备事件落进证据,并带上这条用例的 @case 身份。

    少了这一步,读数会记成 case=None:失败时归因找不到对应的读数,只能判
    「未落证据的失败」——结论落在"未完成"而不是"设备不合格"。
    """
    yield
    # 不碰设备的用例(纯通路冒烟)跳过:record_evidence 会给清单里每台设备都开一个
    # 证据文件,而空文件在归因里读成"这台设备这轮没有任何读数"—— 冒烟任务会被
    # 自己的收尾动作拖成"未完成"。
    if not [name for name in request.node.fixturenames if name in set(registered_types())]:
        return
    events = drain_device_events(composition_root.all_drivers(), event_sink)
    composition_root.record_evidence(identity=case_of(request.node), events=events)


def _device_ids_of_type(config: pytest.Config, device_type: str) -> list[str]:
    manifest = parse_manifest(load_device_config(config.getoption("--device-config")))
    return list(manifest.ids_of_type(device_type))


def pytest_generate_tests(metafunc: pytest.Metafunc) -> None:
    for device_type in registered_types():
        if device_type in metafunc.fixturenames or f"{device_type}_id" in metafunc.fixturenames:
            metafunc.parametrize(
                device_type, _device_ids_of_type(metafunc.config, device_type), indirect=True
            )


def _make_type_fixture(device_type: str):
    def _driver(request, composition_root):
        try:
            return composition_root.get_ready(request.param)
        except UpstreamFault as exc:
            pytest.skip(f"{TEST_INVALID_PREFIX}{exc}")

    return pytest.fixture(name=device_type)(_driver)


def _make_type_id_fixture(device_type: str):
    def _device_id(request):
        return request.getfixturevalue(device_type).device.id

    return pytest.fixture(name=f"{device_type}_id")(_device_id)


for _device_type in registered_types():
    globals()[_device_type] = _make_type_fixture(_device_type)
    globals()[f"{_device_type}_id"] = _make_type_id_fixture(_device_type)
del _device_type

# ==============================================================================
# 北向 E2E 的观察器
#
# broker 由 acceptance/run.sh 起（NB_E2E=1）：产品是 run_task.py 在 pytest
# **之前**拉起的，broker 与 MQTT_BROKER 必须在那之前就绪。这里只负责连上去观察。
#
# 已知限制：观察器在 pytest 启动时才订阅，而产品已经在跑 —— **上线帧看不到**
# （每会话只发一次）。所以 BIRTH 相关的断言放在"重连"场景里做，不在这里。
# ==============================================================================

_nb_probe = None


def pytest_configure(config):
    global _nb_probe
    broker_url = _os.environ.get("MQTT_BROKER", "")
    if not broker_url:
        return
    host, _, port = broker_url.replace("tcp://", "").rpartition(":")
    try:
        from testplatform.transport.real_transports import RealMqttTransport
        from tests_support.northbound_probe import NorthboundProbe

        _nb_probe = NorthboundProbe(
            RealMqttTransport(host or "127.0.0.1", int(port)), "#"
        ).start()
    except Exception:  # noqa: BLE001 - 观察器起不来 = 没有北向结论，fixture 里跳过
        _nb_probe = None


def pytest_unconfigure(config):
    global _nb_probe
    if _nb_probe is not None:
        _nb_probe.stop()
        _nb_probe = None


@pytest.fixture(scope="session")
def northbound_probe():
    """订阅全部主题的观察器（session 级，在 `pytest_configure` 里就订上了）。

    没有 broker（没设 NB_E2E、本机没装 mosquitto、或产品没起来）时**跳过**：
    这一轮没有有效结论，不是产品不合格。
    """
    if _nb_probe is None:
        pytest.skip("没有北向观察器（NB_E2E=1 + mosquitto 才跑北向用例）")
    return _nb_probe


@pytest.fixture(scope="session")
def northbound_publisher():
    """往北向发命令的通道（扮演主站）。

    与观察器分开：观察器只收，这个只发。合在一起会出现"自己发的自己收"，
    测试就永远绿——真正的对端可能根本没收到。
    """
    broker_url = _os.environ.get("MQTT_BROKER", "")
    if not broker_url:
        pytest.skip("没有 broker（NB_E2E=1 + mosquitto 才跑下行用例）")
    host, _, port = broker_url.replace("tcp://", "").rpartition(":")
    from testplatform.transport.real_transports import RealMqttTransport

    transport = RealMqttTransport(host or "127.0.0.1", int(port))
    transport.connect()  # 不连就 publish：_client 是 None，报 AttributeError
    return transport

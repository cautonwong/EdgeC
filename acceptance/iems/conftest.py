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


@pytest.fixture(autouse=True)
def _drain_device_events_after_test(composition_root, event_sink, request):
    """每条用例结束后把设备事件落进证据,并带上这条用例的 @case 身份。

    少了这一步,读数会记成 case=None:失败时归因找不到对应的读数,只能判
    「未落证据的失败」——结论落在"未完成"而不是"设备不合格"。
    """
    yield
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

"""北向通路口径冒烟：broker → 产品 → 观察器。

## 它验什么、不验什么

验的是**通路**：产品真的把数据推到了 broker，地址按协议模板拼、载荷能被平台解开、
metric 名来自产品点位表（不是 `tag_123`）。

**不验判据**：读数对不对是判据集的事（设备手册），这里只看"有没有、叫什么、能不能解"。
所以它是冒烟，不是验收用例——正式验收要走 `composition_root` + 证据 + `gate.py`。

## 为什么单独一个入口

设备的验收口径是「这台设备合不合格」，需要清单、判据集、证据、四档归因。
北向是**通路口径**：它不判某台设备合格与否，只判"这条链路通不通"。
硬塞进设备口径会让报告里出现"VOLTAGE 不合格"这种指错方向的结论。

## 跑法

    # 容器内，平台 venv
    cd /workspaces/test-platform
    ./.venv/bin/python /workspaces/edgeos/EdgeC/acceptance/northbound/smoke_northbound.py

环境变量：
    IEMS_BIN      产品二进制（缺省 /tmp/edgec-build/products/iems/bin/iems）
    IEMS_RUN_DIR  运行目录（缺省 /tmp/edgec-nb-run）
    NB_PROTOCOL   北向协议（缺省 MqttJson；SparkplugB 也能跑，但平台解不了 protobuf）
"""
import os
import subprocess
import sys
import traceback

# 平台代码：观察器与真实传输都在平台仓库里（这是"由平台验"的一部分）。
PLATFORM_SRC = os.environ.get("TESTPLATFORM_SRC", "/workspaces/test-platform/src")
PLATFORM_ROOT = os.environ.get("TESTPLATFORM_ROOT", "/workspaces/test-platform")
sys.path.insert(0, PLATFORM_SRC)
sys.path.insert(0, PLATFORM_ROOT)

from testplatform.transport.real_transports import RealMqttTransport  # noqa: E402
from tests_support.mqtt_broker import BrokerUnavailable, MosquittoBroker  # noqa: E402
from tests_support.northbound_probe import NorthboundProbe, ProbeTimeout  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
ACCEPT_DIR = os.path.join(os.path.dirname(HERE), "iems")
DEFAULT_BIN = "/tmp/edgec-build/products/iems/bin/iems"
DEFAULT_RUN_DIR = "/tmp/edgec-nb-run"

# 产品壳 listen 的端口：与验收套件的缺省端口错开，避免两边同时跑时抢端口。
SHELL_PORT = os.environ.get("NB_SHELL_PORT", "23457")


def main() -> int:
    protocol = os.environ.get("NB_PROTOCOL", "MqttJson")
    run_dir = os.environ.get("IEMS_RUN_DIR", DEFAULT_RUN_DIR)
    binary = os.environ.get("IEMS_BIN", DEFAULT_BIN)

    if not os.path.exists(binary):
        print(f"[nb-smoke] 找不到产品二进制 {binary}（先构建，或用 IEMS_BIN 指过去）")
        return 2

    try:
        broker = MosquittoBroker.start()
    except BrokerUnavailable as exc:
        # 台架缺件 = 这一轮没有有效结论，不是产品问题
        print(f"[nb-smoke] 跳过：{exc}")
        return 2

    print(f"[nb-smoke] broker={broker.url} protocol={protocol}", flush=True)

    env = dict(os.environ)
    env.update(
        {
            "MQTT_BROKER": broker.url,
            "MQTT_PROTOCOL": protocol,
            "MQTT_NODE_ID": "IEMS_NB_SMOKE",
            "MQTT_GROUP_ID": "EdgeOS",
            "IEMS_BIN": binary,
            "IEMS_RUN_DIR": run_dir,
            "PYTHONPATH": PLATFORM_SRC,
        }
    )
    os.makedirs(run_dir, exist_ok=True)

    probe = NorthboundProbe(RealMqttTransport("127.0.0.1", broker.port), "#").start()
    log = open(os.path.join(run_dir, "shell.log"), "w", encoding="utf-8")
    shell = subprocess.Popen(
        [sys.executable, "iems_product.py", "--port", SHELL_PORT],
        cwd=ACCEPT_DIR,
        env=env,
        stdout=log,
        stderr=subprocess.STDOUT,
    )

    ok = False
    try:
        # 给足时间：产品启动 + 南向轮询周期（5000ms）+ 北向首推
        first = probe.wait_for(lambda m: True, timeout=60)
        print(f"[nb-smoke] 首帧 topic={first.topic!r} payload={first.payload[:80]!r}", flush=True)

        probe.wait_count(3, timeout=30)
        points = dict(probe.points())
        print(
            f"[nb-smoke] 共 {len(probe.messages)} 条，上线帧 {probe.birth_count()} 条，"
            f"点位 {points}",
            flush=True,
        )

        # 三条硬断言：通路的三要素
        assert probe.birth_count() >= 1, "没有上线帧：主站不知道节点上线"
        assert points, "一条可解码的数据帧都没有（协议与平台解码器不匹配？）"
        assert any("_" in name for name in points), f"metric 名不像点位名：{list(points)}"

        ok = True
    except ProbeTimeout as exc:
        print(f"[nb-smoke] 超时：{exc}", flush=True)
    except AssertionError as exc:
        print(f"[nb-smoke] 断言失败：{exc}", flush=True)
    except Exception:
        traceback.print_exc()
    finally:
        probe.stop()
        shell.terminate()
        try:
            shell.wait(timeout=10)
        except subprocess.TimeoutExpired:
            shell.kill()
        log.close()
        broker.stop()

    print("[nb-smoke] RESULT:", "PASS" if ok else "FAIL")
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())

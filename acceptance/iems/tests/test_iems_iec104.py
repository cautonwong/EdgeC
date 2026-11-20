"""iems 北向 IEC104 端到端验收:主站视角的帧序列与落点。

平台没有 IEC104 驱动,这一面只能在协议字节上验:一份最小主站
(``iec104_client.py``)连产品自己起的 bus 11 监听,按 IEMS-NB-IEC104 条款逐项断言。

判据分层,每层都配一个可观察的落点,不只看协议应答:

1. **链路层**:STARTDT / STOPDT / TESTFR 的 ACT → CON;STOPDT 之后 TESTFR 保活
   与 STARTDT 重激活都要逐字节对上。
2. **单客户端**:第二路 TCP 连上后由对端关闭,第一路不受影响;第一路断开后新
   客户端能接管(产品只在链路错误后才允许覆盖活动 io)。
3. **总召**:ACT_CON → 数据(IOA 16386 = 台架 231.7V)→ ACT_TERM。
   这条把「台架 → 南向采集 → datahub → IEC104 编码」整条链串起来。
4. **否定面**:不支持的 QOI 回负 ACT_CON 且无数据/无 ACT_TERM;CA 不匹配回
   自己的确认/TERM 或命令 COT 46;控制点表未入库的遥控回 COT 47,且南向
   一帧写都不许出现。
5. **边界**:声明长度非法的 APCI 不许让产品倒下(REST 健康 + 进程未退出)。
6. **自发上报**:从北向 Modbus 镜像改台架电压,必须收到 COT=3 的 I 帧。

为什么只看协议应答是假绿:
  - GI 的 ACT_CON 是回显请求,产品不取数也能回得一模一样,所以数据值要单独断言。
  - 命令的 COT 47 只说明拒绝,不说明没写;南向 bus.json 的写记录才是"真的没动"。

剩余缺口:
  - 正的 C_SC 路径还没有可验的控制点:产品控制点表只有哨兵行
    (cmd/iems/iems_driver_adapter.c 的 s_iec104_controls),IOA 24577 必然回 47。
    表入库后这条用例要把"拒绝"改成"回 COT 7 + 南向写落到唯一从站"。
  - 平台门禁拿不到设备级结论:test-platform 没有 IEC104 驱动且只读,自定义
    客户端也不产生平台设备证据。任务按 smoke 跑,失败会单列但不算设备准入。
"""
import os
import socket
import time

import pytest

from bench import (BRIDGE_PORT, IEC104_PORT_FILE, RUN_DIR, SLAVE_ID, TAG_VOLTAGE,
                   VOLTAGE, VOLTAGE_REGISTER, bus_seen, iec104_port_of, latest,
                   port_of, rest)
from iec104_client import (COT_ACT_CON, COT_ACT_TERM, COT_SPONT, COT_UNKNOWN_CA,
                           COT_UNKNOWN_IOA, STARTDT_ACT, STARTDT_CON, STOPDT_ACT,
                           STOPDT_CON, TESTFR_ACT, TESTFR_CON, TYPE_C_IC_NA_1,
                           TYPE_C_SC_NA_1, TYPE_M_ME_NC_1, Iec104Client)
from testplatform.cases import case
from testplatform.protocol import modbus_rtu

TOLERANCE_V = 0.5
ADDRESS_VOLTAGE = 0  # 台架寄存器 0 = 相位 A 电压,0.1V 分辨率
IOA_VOLTAGE = 16386
IOA_TELEMETRY = 16385
IOA_CONTROL = 24577  # v3 遥控段首地址:段内但控制点表未入库
WRITE_TIMEOUT_S = 5.0
SPONT_TIMEOUT_S = 12.0
SPONT_VOLTAGE = 220.0
SPONT_RAW = int(SPONT_VOLTAGE * 10)  # 台架 0.1V 分辨率
T1_TIMEOUT_S = 20.0  # t1=15s；留出 1s tick 与调度余量


@pytest.fixture(scope="module", autouse=True)
def _open_evidence(evidence_session):
    """冒烟用例不碰 composition_root,不主动开会话就没有 run-scope.json。"""
    return evidence_session


def _wait_rest_voltage(timeout: float = 15.0) -> float:
    """等实时接口落到台架真值。

    GI 取的是 datahub 影子;影子还没被第一轮轮询写入时,总召数据会是 0 或缺失。
    先等 REST,免得把"起步慢"记成"总召编码错"。
    """
    deadline = time.time() + timeout
    last = None
    while time.time() < deadline:
        body = latest(port_of(), TAG_VOLTAGE)
        value = body.get("value")
        if body.get("tag_id") == TAG_VOLTAGE and value is not None:
            last = float(value)
            if abs(last - VOLTAGE) <= TOLERANCE_V:
                return last
        time.sleep(0.3)
    pytest.fail(
        f"REST 上的 tag {TAG_VOLTAGE} 没到台架值 {VOLTAGE}±{TOLERANCE_V}"
        f"(最后看到 {last}) —— 南向采集没起来,后面的 IEC104 总召没有可比的值"
    )


def _connect_iec104(timeout: float = 15.0) -> Iec104Client:
    """连产品 bus 11 的监听端口。端口由产品壳挑好写进 RUN_DIR/iec104_port.txt。"""
    deadline = time.time() + timeout
    last: object = None
    while time.time() < deadline:
        try:
            port = iec104_port_of()
        except (FileNotFoundError, ValueError) as exc:
            last = exc
        else:
            try:
                return Iec104Client("127.0.0.1", port)
            except OSError as exc:
                last = exc
        time.sleep(0.2)
    pytest.fail(
        f"连不上 IEC104 北向端口(端口文件 {IEC104_PORT_FILE}): {last} —— "
        "产品没起 bus 11 监听,或 IEMS_IEC104_PORT 没被认"
    )


def _connect_iec104_takeover(timeout: float = 10.0):
    """第一路断开后等产品把旧链路标成 stale。

    链路错误回调没跑完时新连接会被单客户端模式立刻关掉,所以用 STARTDT 试探,
    被拒就重连,直到拿到 STARTDT CON。返回 (client, 应答帧)。
    """
    deadline = time.time() + timeout
    last: object = None
    while time.time() < deadline:
        client = _connect_iec104()
        try:
            client.send_u(STARTDT_ACT)
            frame = client.recv_frame(timeout=1.0)
        except OSError as exc:
            last = exc
            client.close()
            time.sleep(0.2)
            continue
        if frame.kind == "U" and frame.raw == STARTDT_CON:
            return client, frame
        last = frame
        client.close()
        time.sleep(0.2)
    pytest.fail(f"第一路断开后新客户端没能接管: {last}")


def _wait_product_line(evidence, needle: str, timeout: float = 3.0) -> bool:
    """产品 stdout 由任务执行器落到证据目录的 product.out(行缓冲,写完即见)。"""
    path = evidence.directory / "product.out"
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            if needle in path.read_text(encoding="utf-8", errors="ignore"):
                return True
        except FileNotFoundError:
            pass
        time.sleep(0.1)
    return False


def _product_fd_count() -> int:
    pid = int((RUN_DIR / "product.pid").read_text(encoding="utf-8").strip())
    return len(os.listdir(f"/proc/{pid}/fd"))


def _i_frames(frames) -> list:
    return [frame for frame in frames if frame.kind == "I" and frame.asdu is not None]


def _act_term(frames) -> list:
    return [
        frame for frame in _i_frames(frames)
        if frame.asdu.type_id == TYPE_C_IC_NA_1
        and frame.asdu.cot_value == COT_ACT_TERM
        and not frame.asdu.negative
    ]


def _writes() -> list:
    return bus_seen().get("writes") or []


def _wait_write(before: int, address: int, raw: int, timeout: float = WRITE_TIMEOUT_S) -> dict:
    """等南向台架记到那一帧写。写应答只是回显,不能证明写下去了。"""
    deadline = time.time() + timeout
    while time.time() < deadline:
        hits = [
            w for w in _writes()[before:]
            if w.get("unit") == SLAVE_ID and w["address"] == address and w["value"] == raw
        ]
        if hits:
            return hits[-1]
        time.sleep(0.1)
    return {}


def _north_write(raw: int) -> int:
    """从北向 Modbus 镜像(产品对外那份)写寄存器 0,返回应答回显的原始值。"""
    request = modbus_rtu.build_write_single_request(SLAVE_ID, ADDRESS_VOLTAGE, raw)
    with socket.create_connection(("127.0.0.1", BRIDGE_PORT), timeout=3.0) as sock:
        sock.sendall(request)
        response = b""
        while len(response) < modbus_rtu.WRITE_SINGLE_REGISTER_BYTES:
            chunk = sock.recv(256)
            if not chunk:
                break
            response += chunk
    return modbus_rtu.parse_write_response(response, SLAVE_ID, ADDRESS_VOLTAGE)


@case(id="I_00_00_05_000001", clause="IEMS-NB-IEC104", tags=["验收", "北向", "IEC104"])
@pytest.mark.timeout(180)
def test_iec104_northbound_end_to_end(evidence_session):
    _wait_rest_voltage()
    client = _connect_iec104()
    try:
        # ---- 1. STARTDT ACT → CON：链路没激活，后面的数据都不算数 ----
        client.send_u(STARTDT_ACT)
        frame = client.recv_frame(timeout=5.0)
        assert frame.kind == "U" and frame.raw == STARTDT_CON, (
            f"STARTDT ACT 的应答是 {frame.raw.hex()},期望 {STARTDT_CON.hex()}"
        )

        # ---- TESTFR ACT → CON：保活帧必须由从站自己答 ----
        client.send_u(TESTFR_ACT)
        frame = client.recv_frame(timeout=5.0)
        assert frame.kind == "U" and frame.raw == TESTFR_CON, (
            f"TESTFR ACT 的应答是 {frame.raw.hex()},期望 {TESTFR_CON.hex()}"
        )

        # ---- 1b. STOPDT ACT → CON：暂停数据传送，保活与重激活仍可用 ----
        client.send_u(STOPDT_ACT)
        frame = client.recv_frame(timeout=5.0)
        assert frame.kind == "U" and frame.raw == STOPDT_CON, (
            f"STOPDT ACT 的应答是 {frame.raw.hex()},期望 {STOPDT_CON.hex()}"
        )
        client.send_u(TESTFR_ACT)
        frame = client.recv_frame(timeout=5.0)
        assert frame.kind == "U" and frame.raw == TESTFR_CON, (
            f"STOPDT 后 TESTFR ACT 的应答是 {frame.raw.hex()},期望 {TESTFR_CON.hex()}"
        )
        client.send_u(STARTDT_ACT)
        frame = client.recv_frame(timeout=5.0)
        assert frame.kind == "U" and frame.raw == STARTDT_CON, (
            f"STOPDT 后 STARTDT ACT 的应答是 {frame.raw.hex()},期望 {STARTDT_CON.hex()}"
        )

        # ---- 1c. 单客户端：第二路被对端关闭，第一路仍答 TESTFR ----
        second = _connect_iec104()
        try:
            assert second.wait_peer_close(timeout=5.0), (
                "第二路客户端没有被对端关闭 —— 单客户端模式会覆盖第一路的 io,"
                "两路主站的应答会互相接走"
            )
            assert _wait_product_line(evidence_session, "第二路客户端被拒绝"), (
                "第二路被拒时产品没有打 WARN(product.out 里找不到 "
                "'第二路客户端被拒绝')"
            )
        finally:
            second.close()
        client.send_u(TESTFR_ACT)
        frame = client.recv_frame(timeout=5.0)
        assert frame.kind == "U" and frame.raw == TESTFR_CON, (
            f"第二路被拒后第一路 TESTFR 失效: {frame.raw.hex()}"
        )

        # ---- 2. 总召 QOI=20：ACT_CON → 数据 → ACT_TERM ----
        client.send_gi(qoi=20)
        frames = client.collect(timeout=10.0, until=lambda fs: bool(_act_term(fs)))
        i_frames = _i_frames(frames)
        assert i_frames, f"总召没有任何 I 帧应答: {[str(f) for f in frames]}"

        con = i_frames[0].asdu
        assert (con.type_id, con.cot_value, con.casdu, con.qoi) == (
            TYPE_C_IC_NA_1, COT_ACT_CON, 1, 20), (
            f"总召的第一帧不是正 ACT_CON: type={con.type_id} cot={con.cot_value}"
            f" neg={con.negative} ca={con.casdu} qoi={con.qoi}"
        )

        data = [f.asdu for f in i_frames if f.asdu.type_id == TYPE_M_ME_NC_1]
        assert data, (
            f"总召没有类型 13 的数据 ASDU,收到 {[str(f) for f in i_frames]} —— "
            "datahub 影子没被写,或按类型分组的 GI 没跑"
        )
        assert all(a.cot_value == 20 for a in data), (
            f"总召数据的 COT 不是 20(INTROGEN): {[a.cot_value for a in data]}"
        )
        points = {ioa: value for a in data for (ioa, value, _qds) in a.objects}
        assert IOA_VOLTAGE in points, (
            f"总召数据里没有 IOA {IOA_VOLTAGE}(VOLTAGE_PHASE_A);"
            f"收到的 IOA 是 {sorted(points)}"
        )
        assert points[IOA_VOLTAGE] == pytest.approx(VOLTAGE, abs=TOLERANCE_V), (
            f"IOA {IOA_VOLTAGE} = {points[IOA_VOLTAGE]}V,台架给的是 {VOLTAGE}V —— "
            "链路里某一环把值改了(量纲、缩放、或者根本不是同一个点)"
        )
        terms = _act_term(frames)
        assert terms, "总召没有正 ACT_TERM: 主站无法知道这轮总召结束了"
        assert frames[-1] is terms[-1], (
            f"总召最后一帧不是 ACT_TERM: {str(frames[-1])} —— 数据帧顺序不对"
        )

        # ---- 3a. 不支持的 QOI：负 ACT_CON，无数据、无 ACT_TERM ----
        client.send_gi(qoi=21)
        frames = client.collect(timeout=1.5)
        i_frames = _i_frames(frames)
        assert len(i_frames) == 1, (
            f"QOI=21 应只回一帧负 ACT_CON,收到 {[str(f) for f in i_frames]}"
        )
        neg = i_frames[0].asdu
        assert neg.type_id == TYPE_C_IC_NA_1 and neg.cot == 0x47, (
            f"QOI=21 的应答 COT 是 {neg.cot:#04x},期望 0x47(负 ACT_CON): {str(i_frames[0])}"
        )
        assert neg.casdu == 1 and neg.qoi == 21, (
            f"负 ACT_CON 没回显 CA/QOI: ca={neg.casdu} qoi={neg.qoi}"
        )
        assert not [a for a in (f.asdu for f in i_frames) if a.type_id == TYPE_M_ME_NC_1], (
            "不支持的 QOI 仍然发了数据帧"
        )
        assert not _act_term(frames), "不支持的 QOI 仍然回了 ACT_TERM"

        # ---- 3b. CA 不匹配的总召：自己的 ACT_CON/ACT_TERM，无数据 ----
        client.send_gi(casdu=2)
        frames = client.collect(timeout=5.0, until=lambda fs: bool(_act_term(fs)))
        i_frames = _i_frames(frames)
        assert len(i_frames) == 2, (
            f"CA 2 的总召应只有 ACT_CON/ACT_TERM 两帧,收到 {[str(f) for f in i_frames]}"
        )
        con, term = i_frames[0].asdu, i_frames[1].asdu
        assert (con.type_id, con.cot_value, con.negative, con.casdu, con.qoi) == (
            TYPE_C_IC_NA_1, COT_ACT_CON, False, 2, 20), (
            f"CA 2 的 ACT_CON 不对: {str(i_frames[0])}"
        )
        assert (term.type_id, term.cot_value, term.negative, term.casdu) == (
            TYPE_C_IC_NA_1, COT_ACT_TERM, False, 2), (
            f"CA 2 的 ACT_TERM 不对: {str(i_frames[1])}"
        )
        assert not [a for a in (f.asdu for f in i_frames) if a.type_id == TYPE_M_ME_NC_1], (
            "CA 2 未配置却发了数据 —— 总召没按公共地址找点表"
        )

        # ---- 3c. CA 不匹配的命令：COT 46，不许落写 ----
        client.send_c_sc(IOA_CONTROL, casdu=2)
        frames = client.collect(timeout=3.0)
        i_frames = _i_frames(frames)
        assert len(i_frames) == 1, (
            f"CA 2 的命令应只回一帧,收到 {[str(f) for f in i_frames]}"
        )
        cmd = i_frames[0].asdu
        assert (cmd.type_id, cmd.cot_value, cmd.negative, cmd.casdu) == (
            TYPE_C_SC_NA_1, COT_UNKNOWN_CA, False, 2), (
            f"CA 2 的命令应答不是 COT 46: {str(i_frames[0])}"
        )

        # ---- 3d. 控制点表未入库：遥控/遥测地址都回 COT 47，且无南向写 ----
        before = len(_writes())
        for ioa in (IOA_CONTROL, IOA_TELEMETRY):
            client.send_c_sc(ioa)
            frames = client.collect(timeout=3.0)
            i_frames = _i_frames(frames)
            assert len(i_frames) == 1, (
                f"IOA {ioa} 的命令应只回一帧,收到 {[str(f) for f in i_frames]}"
            )
            cmd = i_frames[0].asdu
            assert (cmd.type_id, cmd.cot_value, cmd.negative, cmd.casdu, cmd.ioa) == (
                TYPE_C_SC_NA_1, COT_UNKNOWN_IOA, False, 1, ioa), (
                f"IOA {ioa} 的命令应答不是 COT 47: {str(i_frames[0])}"
            )
        new_writes = _writes()[before:]
        assert not new_writes, (
            f"没有控制点映射的命令仍产生了南向写: {new_writes} —— "
            "COT 47 必须对应真的没写,不许猜从站地址"
        )

        # ---- 5. 自发上报：改北向镜像的电压，必须出现 COT=3 ----
        # 先留两拍让从站 tick 建基线(首帧只建基线、不报)。
        time.sleep(2.5)
        client.collect(timeout=0.2)
        before = len(_writes())
        try:
            echo = _north_write(SPONT_RAW)
            assert echo == SPONT_RAW, (
                f"北向 Modbus 镜像回显 {echo},期望 {SPONT_RAW}"
            )
            wrote = _wait_write(before, ADDRESS_VOLTAGE, SPONT_RAW)
            assert wrote, (
                f"北向写没落到南向台架(新增写 {_writes()[before:]}) —— "
                f"镜像写必须经 poll_tasks/inventory 路由到 unit {SLAVE_ID}"
            )
            frames = client.collect(
                timeout=SPONT_TIMEOUT_S,
                until=lambda fs: any(
                    f.asdu is not None
                    and f.asdu.type_id == TYPE_M_ME_NC_1
                    and f.asdu.cot_value == COT_SPONT
                    and any(o[0] == IOA_VOLTAGE for o in f.asdu.objects)
                    for f in fs
                ),
            )
            spont = [
                f.asdu for f in frames
                if f.asdu is not None
                and f.asdu.type_id == TYPE_M_ME_NC_1
                and f.asdu.cot_value == COT_SPONT
                and any(o[0] == IOA_VOLTAGE for o in f.asdu.objects)
            ]
            assert spont, (
                f"{SPONT_TIMEOUT_S}s 内没有 COT=3 的电压自发上报"
                f"(收到 {[str(f) for f in frames]}) —— 从站 tick 没跑,"
                "或死区缓存没被填"
            )
            values = [v for a in spont for (ioa, v, _q) in a.objects if ioa == IOA_VOLTAGE]
            assert values[-1] == pytest.approx(SPONT_VOLTAGE, abs=TOLERANCE_V), (
                f"自发上报的电压是 {values[-1]}V,期望 {SPONT_VOLTAGE}V"
            )
        finally:
            # 还原台架读数:这个寄存器就是别的用例要读的电压
            _north_write(VOLTAGE_REGISTER)
            client.collect(timeout=5.0)

        # ---- 4. 畸形 APCI：声明长度 3(<4)，控制字节还长得像 STARTDT ACT ----
        # 产品不许倒,也不许按控制字节猜帧回一个 STARTDT CON。
        client.send_u(bytes([0x68, 0x03, 0x07, 0x00, 0x00, 0x00]))
        replies = client.collect(timeout=0.8)
        assert not replies, (
            f"声明长度 3 的畸形帧被当成了正常帧应答: {[str(f) for f in replies]}"
        )
        time.sleep(0.3)
        body = rest(port_of(), "/api/v1/system/health")
        assert "error" not in body and "http_status" not in body, (
            f"畸形 APCI 之后 REST 健康检查不可用: {body}"
        )
        assert not (RUN_DIR / "iems_exit.json").is_file(), (
            "畸形 APCI 之后产品退出了 —— 声明长度非法必须在解析 ASDU 前拒绝"
        )

        # ---- 6. 链路错误后接管：关掉第一路，新主站 STARTDT/GI 正常 ----
        client.close()
        client, frame = _connect_iec104_takeover()
        assert frame.kind == "U" and frame.raw == STARTDT_CON, (
            f"接管后的 STARTDT 应答是 {frame.raw.hex()},期望 {STARTDT_CON.hex()}"
        )
        client.send_gi(qoi=20)
        frames = client.collect(timeout=3.0, until=lambda fs: bool(_act_term(fs)))
        i_frames = _i_frames(frames)
        assert i_frames, f"接管后的总召没有 I 帧应答: {[str(f) for f in frames]}"
        con = i_frames[0].asdu
        assert (con.type_id, con.cot_value, con.casdu, con.qoi) == (
            TYPE_C_IC_NA_1, COT_ACT_CON, 1, 20), (
            f"接管后的总召第一帧不是正 ACT_CON: {str(i_frames[0])} —— "
            "active_stale 之后新主站没有真正接管"
        )
        if not _act_term(frames):
            # APCI 发送窗口不跨连接复位（另派修复）：collect 结尾已发 S 确认，
            # 隔一拍让它单独结算（引擎一次 RX 只解一帧），再补一次总召取数据；
            # 这里断言 GI 行为，不看序号。
            time.sleep(0.3)
            client.send_gi(qoi=20)
            frames += client.collect(timeout=10.0, until=lambda fs: bool(_act_term(fs)))
        points = {
            ioa: value
            for a in (f.asdu for f in frames)
            if a.type_id == TYPE_M_ME_NC_1
            for (ioa, value, _qds) in a.objects
        }
        assert points.get(IOA_VOLTAGE) == pytest.approx(VOLTAGE, abs=TOLERANCE_V), (
            f"接管后的总召 IOA {IOA_VOLTAGE} = {points.get(IOA_VOLTAGE)},"
            f"台架给的是 {VOLTAGE}V"
        )
        assert _act_term(frames), "接管后的总召没有正 ACT_TERM"

        # ---- 7. 连续 3 轮连接/断开：旧句柄在接管时销毁，fd 不累积 ----
        fd_before = _product_fd_count()
        for round_no in range(3):
            client.close()
            client, frame = _connect_iec104_takeover()
            assert frame.kind == "U" and frame.raw == STARTDT_CON, (
                f"第 {round_no + 1} 轮接管后的 STARTDT 应答是 {frame.raw.hex()},"
                f"期望 {STARTDT_CON.hex()}"
            )
        time.sleep(0.3)  # uv_close 的回调跑完，被替换句柄的 fd 才真正释放
        fd_after = _product_fd_count()
        assert abs(fd_after - fd_before) <= 1, (
            f"3 轮连接/断开后产品 fd 从 {fd_before} 涨到 {fd_after} —— "
            "旧主站句柄没有被销毁,CLOSE-WAIT 在累积"
        )
    finally:
        client.close()


@case(id="I_00_00_05_000002", clause="IEMS-NB-IEC104", tags=["验收", "北向", "IEC104"])
@pytest.mark.timeout(180)
def test_iec104_t1_timeout_disconnects_and_resets_session(evidence_session):
    """I 帧 15s 无人确认（t1 到期）：产品必须主动断链，重连后从 N(S)=0 重来。

    只读不应答:总召的 I 帧发回后一帧 S 都不发，让服务端的 t1 到期。旧实现
    t1 无人消费——连接保持打开、序号僵在原处，主站不重连就永远收不到新数据。
    """
    _wait_rest_voltage()
    client, _frame = _connect_iec104_takeover()
    try:
        client.send_gi(qoi=20)
        frames = []
        closed = False
        deadline = time.monotonic() + T1_TIMEOUT_S
        while time.monotonic() < deadline:
            try:
                frames.append(client.recv_frame(timeout=max(0.05, deadline - time.monotonic())))
            except TimeoutError:
                break
            except ConnectionError:
                closed = True
                break
        assert closed, (
            f"{T1_TIMEOUT_S}s 内没收到对端关闭（EOF/RST）：I 帧一直不确认，"
            f"t1 到期必须断链；收到 {[str(f) for f in frames]}"
        )
        i_frames = _i_frames(frames)
        assert len(i_frames) >= 3, (
            f"t1 等待期间应已收到总召的 ACT_CON/数据/ACT_TERM,"
            f"收到 {[str(f) for f in frames]}"
        )
        assert _wait_product_line(evidence_session, "t1 超时"), (
            "客户端被断开但产品没打 t1 超时告警(product.out 里找不到 't1 超时') —— "
            "断链不是 t1 到期分支做的"
        )
    finally:
        client.close()

    # 重连：会话复位后 STARTDT/GI 正常，首帧 N(S) 必须回到 0
    client, frame = _connect_iec104_takeover()
    try:
        assert frame.kind == "U" and frame.raw == STARTDT_CON, (
            f"重连后的 STARTDT 应答是 {frame.raw.hex()},期望 {STARTDT_CON.hex()}"
        )
        client.send_gi(qoi=20)
        frames = client.collect(timeout=10.0, until=lambda fs: bool(_act_term(fs)))
        i_frames = _i_frames(frames)
        assert i_frames, f"重连后的总召没有 I 帧应答: {[str(f) for f in frames]}"
        first = i_frames[0]
        assert first.control == 0, (
            f"重连后首帧 N(S)={first.control},期望 0 —— t1 断链后会话没有复位"
        )
        con = first.asdu
        assert (con.type_id, con.cot_value, con.casdu, con.qoi) == (
            TYPE_C_IC_NA_1, COT_ACT_CON, 1, 20), (
            f"重连后的总召第一帧不是正 ACT_CON: {str(first)}"
        )
        assert _act_term(frames), "重连后的总召没有正 ACT_TERM"
    finally:
        client.close()

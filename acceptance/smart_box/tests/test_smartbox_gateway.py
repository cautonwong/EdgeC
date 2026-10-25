"""smart_box 网关冒烟:只验"网关还活着并且行为是它自己的"。

不是验收。原因:平台证据层只认字节流设备,智能盒子是"产品 = HTTP 服务",
没有可挂证据的 dut;硬做只会得到"未落证据的失败",门禁永远红。

为什么断言不用 matrix 那套:
  matrix 的期望值一半是产品侧写死的常量(main.go:306 / :751 / :850),
  断言常量字串,桩也能过。这里全部改成看**副作用**:HTTP 状态码 + sqlite 行。
  已知桩(face_verify 的 matchedUser 是常量)单列一条,记录事实,不判通过。
"""
import base64

import pytest

from bench import DB_PATH, TAG_VOICE_COMMAND, count, http, login
from testplatform.cases import case

@pytest.fixture(scope="module", autouse=True)
def _open_evidence(evidence_session):
    """冒烟用例本身不用证据层,但这一轮的 run-scope.json 得有人开。

    证据会话是懒建的(platform/plugin.py:_evidence_of),没人要就一直不建;
    缺 run-scope.json 时归因会判"范围缺失",任务结论就被压成未完成。
    """
    return evidence_session


def _wait_ready(timeout: float = 10.0) -> None:
    """网关自己建表;表没建好之前别断言行数。"""
    import time

    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            count("tag_history")
            return
        except Exception:
            time.sleep(0.2)
    pytest.fail(f"网关 {timeout:.0f}s 内没有把表建起来:{DB_PATH}")


@case(id="SBX_00_00_01_000001_01", clause="SBX-RBAC-1", tags=["冒烟"])
def test_rbac_blocks_what_it_should():
    """同一个端点在三种身份下三种结果 —— 这条错了就是权限模型坏了。"""
    _wait_ready()

    status, _ = http("POST", "/api/v1/privacy/purge")
    assert status == 401, f"没有 token 时应该是 401,实际 {status}"

    status, _ = http("POST", "/api/v1/privacy/purge", token=login("USER"))
    assert status == 403, f"USER 角色应该被拒(403),实际 {status}"

    status, _ = http("POST", "/api/v1/privacy/purge", token=login("ADMIN"))
    assert status == 200, f"ADMIN 角色应该通过(200),实际 {status}"


@case(id="SBX_00_00_01_000002_01", clause="SBX-VOICE-NLU-1", tags=["冒烟"])
def test_voice_command_records_the_intent_code():
    """说"开始训练"必须在 tag_history 留一行 cmd_code=1.0。

    看的是库里那一行,不是响应里的 action 字串(action 是产品自己写死的常量)。
    """
    _wait_ready()
    token = login("USER")
    before = count("tag_history")

    status, _ = http("POST", "/api/v1/voice/command", token=token, body={"transcript": "开始训练"})
    assert status == 200, f"语音命令应该 200,实际 {status}"

    assert count("tag_history") == before + 1, "命中关键词却没有落 tag_history"
    latest = _last_tag_value()
    assert latest == pytest.approx(1.0), f"cmd_code 应该是 1.0,实际 {latest}"


@case(id="SBX_00_00_01_000003_01", clause="SBX-VOICE-NLU-1", tags=["冒烟"])
def test_voice_command_ignores_unknown_speech():
    """听不懂的话不能凭空造一行记录。"""
    _wait_ready()
    before = count("tag_history")

    status, _ = http(
        "POST", "/api/v1/voice/command", token=login("USER"), body={"transcript": "今天天气不错"}
    )
    assert status == 200
    assert count("tag_history") == before, "没命中关键词却往 tag_history 里写了一行"


@case(id="SBX_00_00_01_000004_01", clause="SBX-PRIVACY-1", tags=["冒烟"])
def test_purge_really_empties_the_tables():
    """purge 不是回一句成功就完事:三张表得真的空。"""
    _wait_ready()
    http("POST", "/api/v1/voice/command", token=login("USER"), body={"transcript": "开始训练"})
    assert count("tag_history") > 0, "前置写入没生效,这条断言无从谈起"

    status, _ = http("POST", "/api/v1/privacy/purge", token=login("ADMIN"))
    assert status == 200
    for table in ("sensitive_records", "tag_history", "blockchain_receipts"):
        assert count(table) == 0, f"purge 之后 {table} 还有 {count(table)} 行"


@case(id="SBX_00_00_01_000005_01", clause="SBX-KNOWN-STUB-1", tags=["冒烟"])
def test_face_verify_is_a_constant_stub():
    """记录事实,不判通过:换一张图,返回的还是同一个用户。

    main.go:850 `matchedUser := "ELDERLY-USER-8821"` —— 人脸识别没有真的做。
    matrix 里那条断言 `user_id == "ELDERLY-USER-8821"` 且 `confidence > 0.95`,
    传 `b"dummy_face_image_bytes_12345"` 也能过,是假绿。这里改成把事实钉住:
    等真做人脸匹配时,这条会红 —— 那时应该改成"不同图给不同人"。
    """
    _wait_ready()
    token = login("USER")
    users = set()
    for image in (b"face-a", b"face-b"):
        status, payload = http(
            "POST",
            "/api/v1/cv/face_verify",
            token=token,
            body={"frame_base64": base64.b64encode(image).decode()},
        )
        assert status == 200, f"face_verify 应该 200,实际 {status}"
        users.add(payload["data"]["user_id"])

    assert len(users) == 1, (
        f"两张不同的图返回了不同用户({users})—— 说明真的在做匹配了,"
        "这条已知桩记录该升级成验收判据"
    )


def _last_tag_value():
    from bench import rows

    got = rows("SELECT value FROM tag_history WHERE tag_id = ? ORDER BY timestamp DESC LIMIT 1",
               (TAG_VOICE_COMMAND,))
    assert got, "tag_history 里没有语音命令那一行"
    return got[0][0]

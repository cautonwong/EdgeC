"""smart_box 台架:网关就是一个 HTTP 服务,判据只看它写进 sqlite 的东西。

为什么不用 matrix 里那套断言:那些"期望值"是产品自己写死的常量
(main.go:306 "GDPR compliance zero-wipe completed successfully"、
main.go:751 "Trigger Exoskeleton Gait Assistance"、main.go:850
matchedUser := "ELDERLY-USER-8821")。断言常量字串,桩也能过 —— 假绿。
这里改成看副作用:HTTP 状态码 + sqlite 行数/字段值。
"""
import json
import sqlite3
import urllib.error
import urllib.request
from pathlib import Path

RUN_DIR = Path("/tmp/smartbox-acceptance-run")
DB_PATH = RUN_DIR / "smart_box.db"
PORT_FILE = RUN_DIR / "port.txt"

# 网关的 tag 编号(go 侧常量):语音命令 12000。
TAG_VOICE_COMMAND = 12000


def base_url() -> str:
    return f"http://127.0.0.1:{PORT_FILE.read_text(encoding='utf-8').strip()}"


def http(method: str, path: str, *, token: str | None = None, body=None, timeout=5.0):
    """返回 (状态码, JSON)。4xx/5xx 不抛异常 —— 判 RBAC 就是靠状态码。"""
    data = json.dumps(body).encode() if body is not None else None
    req = urllib.request.Request(base_url() + path, data=data, method=method)
    if data is not None:
        req.add_header("Content-Type", "application/json")
    if token:
        req.add_header("Authorization", f"Bearer {token}")
    try:
        with urllib.request.urlopen(req, timeout=timeout) as resp:
            return resp.status, json.loads(resp.read() or b"{}")
    except urllib.error.HTTPError as exc:
        return exc.code, {}


def login(role: str, username: str = "acceptance") -> str:
    status, payload = http("POST", "/api/v1/auth/login", body={"username": username, "role": role})
    assert status == 200, f"登录失败(role={role}):HTTP {status}"
    return payload["data"]["token"]


def count(table: str) -> int:
    conn = sqlite3.connect(f"file:{DB_PATH}?mode=ro", uri=True)
    try:
        return conn.execute(f"SELECT count(*) FROM {table}").fetchone()[0]
    finally:
        conn.close()


def rows(sql: str, params=()):
    conn = sqlite3.connect(f"file:{DB_PATH}?mode=ro", uri=True)
    try:
        return conn.execute(sql, params).fetchall()
    finally:
        conn.close()

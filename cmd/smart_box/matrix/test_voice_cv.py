#!/usr/bin/env python3
import requests
import pytest
import os
import base64

GATEWAY_URL = os.getenv("SMARTBOX_GATEWAY_URL", "http://localhost:8083")

def get_admin_token():
    resp = requests.post(f"{GATEWAY_URL}/api/v1/auth/login", json={"username": "admin1", "role": "ADMIN"}, timeout=2)
    return resp.json()["data"]["token"]

def test_voice_command_and_speaker_bridge():
    """验证语音指令解析与智能音箱适配器"""
    token = get_admin_token()
    headers = {"Authorization": f"Bearer {token}"}

    # 1. 模拟语音识别输入 "启动步行训练" -> 应成功转为外骨骼 Modbus 动作码
    resp = requests.post(
        f"{GATEWAY_URL}/api/v1/voice/command",
        json={"transcript": "启动步行训练"},
        headers=headers,
        timeout=2
    )
    assert resp.status_code == 200
    res = resp.json()
    assert res["code"] == 0
    data = res["data"]
    assert data["action"] == "Trigger Exoskeleton Gait Assistance"
    assert data["cmd_code"] == 1.0

    # 2. 模拟小爱/小度音箱适配器透传状态查询
    resp = requests.post(
        f"{GATEWAY_URL}/api/v1/voice/bridge/speaker",
        json={"speaker_type": "xiaomi", "action": "get_status"},
        headers=headers,
        timeout=2
    )
    assert resp.status_code == 200
    assert resp.json()["code"] == 0

def test_face_verification():
    """验证无感人脸识别提取匹配"""
    token = get_admin_token()
    headers = {"Authorization": f"Bearer {token}"}

    fake_image = base64.b64encode(b"dummy_face_image_bytes_12345").decode("utf-8")
    resp = requests.post(
        f"{GATEWAY_URL}/api/v1/cv/face_verify",
        json={"frame_base64": fake_image},
        headers=headers,
        timeout=2
    )
    assert resp.status_code == 200
    res = resp.json()
    assert res["code"] == 0
    result = res["data"]
    assert result["user_id"] == "ELDERLY-USER-8821"
    assert result["confidence"] > 0.95

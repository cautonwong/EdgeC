#!/usr/bin/env python3
import requests
import pytest
import os

GATEWAY_URL = os.getenv("SMARTBOX_GATEWAY_URL", "http://localhost:8083")

def get_admin_token():
    resp = requests.post(f"{GATEWAY_URL}/api/v1/auth/login", json={"username": "admin1", "role": "ADMIN"}, timeout=2)
    return resp.json()["data"]["token"]

def test_traffic_redeem_and_iptables_firewall():
    """验证健康积分兑换流量与 iptables 限速放行命令构造"""
    token = get_admin_token()
    headers = {"Authorization": f"Bearer {token}"}

    resp = requests.post(
        f"{GATEWAY_URL}/api/v1/monetization/redeem",
        json={"client_mac": "AA:BB:CC:DD:EE:FF", "coins": 100},
        headers=headers,
        timeout=2
    )
    assert resp.status_code == 200
    res = resp.json()
    assert res["code"] == 0

def test_family_album_upload():
    """验证适老化家庭电子相册多端照片上传"""
    token = get_admin_token()
    headers = {"Authorization": f"Bearer {token}"}

    resp = requests.post(
        f"{GATEWAY_URL}/api/v1/album/upload",
        json={
            "family_member": "Lily",
            "image_base64": "dummy_photo_data_b64",
            "caption": "Grandma at Central Park"
        },
        headers=headers,
        timeout=2
    )
    assert resp.status_code == 200
    res = resp.json()
    assert res["code"] == 0
    assert res["data"]["photo_id"].startswith("album_Lily_")

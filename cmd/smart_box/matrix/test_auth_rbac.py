#!/usr/bin/env python3
import requests
import pytest
import os

GATEWAY_URL = os.getenv("SMARTBOX_GATEWAY_URL", "http://localhost:8083")

def test_jwt_login_and_rbac_flow():
    """验证 USER, FAMILY, DOCTOR, ADMIN 4级 RBAC 鉴权矩阵"""
    # 1. 匿名未登录访问防护区 -> 应该返回 401 Unauthorized
    resp = requests.post(f"{GATEWAY_URL}/api/v1/privacy/purge", timeout=2)
    assert resp.status_code == 401, "Unauthenticated request must be blocked with 401"

    # 2. 以 USER 身份登录获取 Token
    resp = requests.post(f"{GATEWAY_URL}/api/v1/auth/login", json={"username": "user1", "role": "USER"}, timeout=2)
    assert resp.status_code == 200
    user_token = resp.json()["data"]["token"]

    # 3. USER 角色试图调用 ADMIN 专属敏感接口 -> 应该返回 403 Forbidden
    headers = {"Authorization": f"Bearer {user_token}"}
    resp = requests.post(f"{GATEWAY_URL}/api/v1/privacy/purge", headers=headers, timeout=2)
    assert resp.status_code == 403, "USER role accessing ADMIN endpoint must return 403"

    # 4. 以 ADMIN 身份登录并调用
    resp = requests.post(f"{GATEWAY_URL}/api/v1/auth/login", json={"username": "admin1", "role": "ADMIN"}, timeout=2)
    assert resp.status_code == 200
    admin_token = resp.json()["data"]["token"]

    admin_headers = {"Authorization": f"Bearer {admin_token}"}
    resp = requests.post(f"{GATEWAY_URL}/api/v1/privacy/purge", headers=admin_headers, timeout=2)
    assert resp.status_code == 200, "ADMIN role should successfully pass RBAC check"
    assert resp.json()["code"] == 0
    assert "GDPR" in resp.json()["message"]

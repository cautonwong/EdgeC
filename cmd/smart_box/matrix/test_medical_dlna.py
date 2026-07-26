#!/usr/bin/env python3
import requests
import pytest
import os

GATEWAY_URL = os.getenv("SMARTBOX_GATEWAY_URL", "http://localhost:8083")

def get_admin_token():
    resp = requests.post(f"{GATEWAY_URL}/api/v1/auth/login", json={"username": "doctor1", "role": "DOCTOR"}, timeout=2)
    return resp.json()["data"]["token"]

def test_fhir_export_and_generative_rehab_plan():
    """验证 HL7 FHIR R4 导出与 AI 康复方案生成"""
    token = get_admin_token()
    headers = {"Authorization": f"Bearer {token}"}

    # 1. 导出 FHIR R4 标准 Bundle
    resp = requests.get(f"{GATEWAY_URL}/api/v1/medical/fhir_export", headers=headers, timeout=2)
    assert resp.status_code == 200
    res = resp.json()
    assert res["code"] == 0
    fhir_bundle = res["data"]
    assert fhir_bundle["resourceType"] == "Bundle"
    assert fhir_bundle["type"] == "searchset"

    # 2. 生成生成式 AI 周康复计划
    resp = requests.post(
        f"{GATEWAY_URL}/api/v1/ai/rehab_plan",
        json={"user_age": 68, "avg_heart_rate": 78.0, "recent_fall": True, "target_goal": "Gait Assistance"},
        headers=headers,
        timeout=2
    )
    assert resp.status_code == 200
    res = resp.json()
    assert res["code"] == 0
    plan = res["data"]
    assert "weekly_schedule" in plan
    assert "nutrition_advice" in plan

def test_dlna_soap_casting():
    """验证 DLNA AVTransport SOAP 控制报文发包"""
    token = get_admin_token()
    headers = {"Authorization": f"Bearer {token}"}

    resp = requests.post(
        f"{GATEWAY_URL}/api/v1/ui/cast",
        json={"target_tv_ip": "192.168.1.10", "media_url": "http://localhost/view"},
        headers=headers,
        timeout=2
    )
    assert resp.status_code == 200
    assert resp.json()["code"] == 0

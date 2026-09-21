#!/usr/bin/env python3
"""
Mandatory Tool Execution Ledger for EdgeOS / EdgeC AI Code Review
Issue #321 架构闭环实现：
1. 跟踪 Review 全生命周期中的工具执行状态机 (NOT_RUN -> RUNNING -> SUCCESS / FINDINGS / ERROR / TIMEOUT / TRUNCATED / FALLBACK)；
2. 杜绝未执行工具伪装通过、工具异常假装成功、未覆盖必需工具直接得出 PASS 结论；
3. 输出完整的 Ledger 审计账本供机器校验与审计回溯。
"""

import time
import json
import datetime
from enum import Enum
from dataclasses import dataclass, field, asdict
from typing import List, Dict, Optional, Any


class ToolStatus(str, Enum):
    NOT_RUN = "NOT_RUN"
    RUNNING = "RUNNING"
    SUCCESS = "SUCCESS"
    FINDINGS = "FINDINGS"
    ERROR = "ERROR"
    TIMEOUT = "TIMEOUT"
    TRUNCATED = "TRUNCATED"
    FALLBACK = "FALLBACK"


@dataclass
class ToolExecutionRecord:
    tool_name: str
    status: ToolStatus = ToolStatus.NOT_RUN
    start_time: Optional[str] = None
    end_time: Optional[str] = None
    duration_ms: float = 0.0
    input_args: Dict[str, Any] = field(default_factory=dict)
    findings_count: int = 0
    output_summary: str = ""
    error_message: str = ""
    _start_timestamp: Optional[float] = None

    def to_dict(self) -> Dict[str, Any]:
        d = asdict(self)
        d.pop("_start_timestamp", None)
        d["status"] = self.status.value if isinstance(self.status, ToolStatus) else str(self.status)
        return d


class ToolExecutionLedger:
    def __init__(self, required_tools: Optional[List[str]] = None):
        self.required_tools: List[str] = list(required_tools) if required_tools else []
        self.records: Dict[str, ToolExecutionRecord] = {}
        for tool in self.required_tools:
            self.records[tool] = ToolExecutionRecord(tool_name=tool, status=ToolStatus.NOT_RUN)

    def start_tool(self, tool_name: str, input_args: Optional[Dict[str, Any]] = None) -> ToolExecutionRecord:
        now_ts = time.time()
        now_iso = datetime.datetime.now(datetime.timezone.utc).isoformat()
        if tool_name not in self.records:
            record = ToolExecutionRecord(
                tool_name=tool_name,
                status=ToolStatus.RUNNING,
                start_time=now_iso,
                input_args=input_args or {},
                _start_timestamp=now_ts
            )
            self.records[tool_name] = record
        else:
            record = self.records[tool_name]
            record.status = ToolStatus.RUNNING
            record.start_time = now_iso
            record.input_args = input_args or {}
            record._start_timestamp = now_ts
        return record

    def finish_tool(
        self,
        tool_name: str,
        status: ToolStatus,
        findings_count: int = 0,
        output_summary: str = "",
        error_message: str = ""
    ) -> ToolExecutionRecord:
        now_ts = time.time()
        now_iso = datetime.datetime.now(datetime.timezone.utc).isoformat()
        if tool_name not in self.records:
            record = ToolExecutionRecord(tool_name=tool_name)
            self.records[tool_name] = record
        else:
            record = self.records[tool_name]

        record.status = status
        record.end_time = now_iso
        record.findings_count = findings_count
        record.output_summary = output_summary
        record.error_message = error_message
        if record._start_timestamp:
            record.duration_ms = round((now_ts - record._start_timestamp) * 1000, 2)
        return record

    def get_status(self, tool_name: str) -> ToolStatus:
        if tool_name in self.records:
            return self.records[tool_name].status
        return ToolStatus.NOT_RUN

    def get_record(self, tool_name: str) -> Optional[ToolExecutionRecord]:
        return self.records.get(tool_name)

    def get_completed_tools(self) -> List[str]:
        valid_done = {ToolStatus.SUCCESS, ToolStatus.FINDINGS, ToolStatus.TRUNCATED, ToolStatus.FALLBACK}
        return [name for name, rec in self.records.items() if rec.status in valid_done]

    def get_pending_tools(self) -> List[str]:
        valid_done = {ToolStatus.SUCCESS, ToolStatus.FINDINGS, ToolStatus.TRUNCATED, ToolStatus.FALLBACK}
        return [name for name in self.required_tools if name not in self.records or self.records[name].status not in valid_done]

    def get_failed_tools(self) -> List[str]:
        fail_states = {ToolStatus.ERROR, ToolStatus.TIMEOUT}
        return [name for name, rec in self.records.items() if rec.status in fail_states]

    def is_complete(self) -> bool:
        """所有必需工具均已正常完成（SUCCESS, FINDINGS, TRUNCATED, FALLBACK），无 NOT_RUN, RUNNING, ERROR, TIMEOUT"""
        valid_done = {ToolStatus.SUCCESS, ToolStatus.FINDINGS, ToolStatus.TRUNCATED, ToolStatus.FALLBACK}
        for tool in self.required_tools:
            if tool not in self.records:
                return False
            if self.records[tool].status not in valid_done:
                return False
        return True

    def to_dict(self) -> Dict[str, Any]:
        return {
            "required_tools": self.required_tools,
            "is_complete": self.is_complete(),
            "pending_tools": self.get_pending_tools(),
            "failed_tools": self.get_failed_tools(),
            "records": {k: v.to_dict() for k, v in self.records.items()}
        }

    def to_json(self, indent: int = 2) -> str:
        return json.dumps(self.to_dict(), indent=indent, ensure_ascii=False)

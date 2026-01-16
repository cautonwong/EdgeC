from typing import Any, Optional
from .model import DataStore

class BaseProtocol:
    """协议基类：负责语义解析与 TagID 映射"""
    def __init__(self, bindings: dict):
        self.bindings = bindings # 契约中定义的绑定元数据
        self._tag_to_addr = {}
        self._addr_to_tag = {}
        self._setup_mappings()

    def _setup_mappings(self):
        """子类需实现具体的物理地址与 TagID 转换"""
        raise NotImplementedError()

    async def process(self, raw_data: bytes, datastore: DataStore) -> Optional[bytes]:
        """
        处理原始数据。
        1. 检查是否匹配该设备的站号/地址。
        2. 如果匹配，解析请求内容。
        3. 从 DataStore 获取数据。
        4. 序列化响应报文。
        """
        raise NotImplementedError()

import asyncio
import logging
import os
from typing import Optional, Callable

logger = logging.getLogger("Matrix.Transport")

class BaseTransport:
    """传输层基类：负责物理字节流的读写"""
    def __init__(self):
        self.on_received: Optional[Callable[[bytes], asyncio.Future]] = None

    async def send(self, data: bytes):
        raise NotImplementedError()

    async def start(self):
        raise NotImplementedError()

class UDSTransport(BaseTransport):
    """Unix Domain Socket 传输：模拟本地串行/以太总线"""
    def __init__(self, path: str, is_server: bool = True):
        super().__init__()
        self.path = path
        self.is_server = is_server
        self._server = None
        self._writer = None

    async def start(self):
        if os.path.exists(self.path):
            os.remove(self.path)
        
        if self.is_server:
            self._server = await asyncio.start_unix_server(
                self._handle_client, self.path
            )
            logger.info(f"UDS Server listening on {self.path}")
        else:
            # 作为客户端连接 (用于某些特殊拓扑)
            pass

    async def _handle_client(self, reader, writer):
        self._writer = writer
        logger.info("EdgeC connected to VBus")
        try:
            while True:
                data = await reader.read(4096)
                if not data:
                    break
                if self.on_received:
                    # 触发广播
                    responses = await self.on_received(data)
                    for resp in responses:
                        # [时序仿真] 模拟真实串口的帧间间隙
                        await asyncio.sleep(0.01)
                        self._writer.write(resp)
                        await self._writer.drain()
        except Exception as e:
            logger.error(f"UDS Transport Error: {e}")
        finally:
            writer.close()
            await writer.wait_closed()
            self._writer = None
            logger.info("EdgeC disconnected")

class TCPTransport(BaseTransport):
    """TCP Server 传输：模拟以太网总线 (如 ModbusTCP, IEC104)"""
    def __init__(self, host: str, port: int):
        super().__init__()
        self.host = host
        self.port = port
        self._server = None
        self._writer = None

    async def start(self):
        self._server = await asyncio.start_server(
            self._handle_client, self.host, self.port
        )
        logger.info(f"TCP Server listening on {self.host}:{self.port}")

    async def _handle_client(self, reader, writer):
        self._writer = writer
        addr = writer.get_extra_info('peername')
        logger.info(f"EdgeC connected via TCP from {addr}")
        try:
            while True:
                data = await reader.read(4096)
                if not data: break
                if self.on_received:
                    responses = await self.on_received(data)
                    for resp in responses:
                        writer.write(resp)
                        await writer.drain()
        except Exception as e:
            logger.error(f"TCP Transport Error: {e}")
        finally:
            writer.close()
            await writer.wait_closed()
            self._writer = None
            logger.info("EdgeC disconnected (TCP)")

    async def send(self, data: bytes):
        if self._writer:
            self._writer.write(data)
            await self._writer.drain()

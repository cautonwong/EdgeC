import asyncio
import numpy as np
import socket
import array
import os
import requests
import logging
from tools.matrix.simulators.adc_simulator import SignalFactory
from tools.matrix.engine.verifier import RMSVerifier

logging.basicConfig(level=logging.INFO, format='%(asctime)s [%(levelname)s] %(message)s')
logger = logging.getLogger("Matrix.SimulationDrive")

class ADCSimulationRunner:
    def __init__(self, edge_url: str = "http://127.0.0.1:9091", uds_path: str = "/tmp/v_adc.sock"):
        self.factory = SignalFactory(sample_rate=4000, frame_size=400)
        self.verifier = RMSVerifier(tolerance=0.002) # 0.2% 工业级精度断言
        self.edge_url = edge_url
        self.uds_path = uds_path
        self.client_sock = None
        self.fd = -1

    async def _setup_uds(self):
        if os.path.exists(self.uds_path): os.remove(self.uds_path)
        self.fd = self.factory.create_memfd("adc_ref_mirror")
        
        connected = asyncio.Event()

        async def _handle_client(reader, writer):
            self.client_sock = writer.get_extra_info('socket')
            logger.info("📡 EdgeOS connected. Starting Active Handshake...")
            
            # [设计优化] 主动握手：循环发送 FD 直到收到 ACK (\x06)
            retry_count = 0
            while not connected.is_set():
                try:
                    logger.info(f"📤 Sending SHM File Descriptor (Attempt {retry_count})...")
                    self.client_sock.sendmsg([b'\xFD'], [(socket.SOL_SOCKET, socket.SCM_RIGHTS, array.array("i", [self.fd]))])
                    
                    # 等待 0.5s 看有没有 ACK
                    try:
                        ack = await asyncio.wait_for(reader.read(1), timeout=0.5)
                        if ack == b'\x06':
                            logger.info("🤝 Handshake Success! C-Side has mapped memory.")
                            connected.set()
                            break
                    except asyncio.TimeoutError:
                        pass
                except Exception as e:
                    logger.error(f"❌ Handshake Error: {e}")
                    break
                
                retry_count += 1
                if retry_count > 10:
                    logger.error("🛑 Handshake failed after 10 attempts.")
                    break

        self.server = await asyncio.start_unix_server(_handle_client, path=self.uds_path)
        await connected.wait()

    async def run_scenario(self, duration_s: int = 10):
        """执行测试场景：模拟电网波动并实时校验精度"""
        await self._setup_uds()
        
        start_time = asyncio.get_event_loop().time()
        count = 0
        
        # 定义工况序列
        while (asyncio.get_event_loop().time() - start_time) < duration_s:
            # 1. 动态调整工况：在第 3 秒时模拟一次 30% 的电压骤降 (Sag)
            elapsed = asyncio.get_event_loop().time() - start_time
            transient = {'type': 'sag', 'factor': 0.7} if 3.0 < elapsed < 4.0 else None
            
            # 2. 合成波形并注入 C 进程
            raw_wave = self.factory.compose_wave(
                freq=50.0 + np.random.normal(0, 0.01), 
                amp=220.0,
                harmonics={3: 0.03, 5: 0.01}, 
                transient=transient,
                noise_std=0.5
            )
            
            # [追踪] 记录 Python 写入的首个采样点
            first_val = raw_wave[0]
            logger.debug(f"📝 [Python] SHM Write Frame {count}: First Sample={first_val:.4f}")
            
            # 3. 发送触发脉冲 (模拟硬件中断)
            self.client_sock.send(b'\x01')
            logger.debug(f"🔔 [Python] Trigger Sent for Frame {count}")
            
            # 4. 计算 Python 侧的“理论金标准”
            py_rms = self.factory.get_ground_truth_rms(raw_wave)
            
            # 5. 等待 C 侧处理完成 (50ms 延迟)
            await asyncio.sleep(0.05)
            
            # 6. 抓取 C 侧算法结果 (TAG ID 1001 是 Voltage_RMS)
            try:
                # 假设 EdgeC 算法结果发布到 1001 号点位
                resp = requests.get(f"{self.edge_url}/api/v1/data/latest/1001", timeout=0.1)
                if resp.status_code == 200:
                    c_rms = resp.json().get("value", 0.0)
                    # 7. 执行实时精度断言
                    self.verifier.verify(c_rms, py_rms)
            except Exception as e:
                logger.warning(f"Failed to fetch C result: {e}")

            count += 1
            await asyncio.sleep(0.05) # 100ms 触发一次 (10Hz 计算频率)

        logger.info(f"🏁 Scenario Finished. Processed {count} frames.")
        self.factory.close()

if __name__ == "__main__":
    runner = ADCSimulationRunner()
    asyncio.run(runner.run_scenario(duration_s=15))

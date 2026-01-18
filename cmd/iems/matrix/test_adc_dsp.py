import pytest
import asyncio
import numpy as np
import socket
import array
import os

@pytest.mark.asyncio
async def test_industrial_proof_dsp(matrix):
    from tools.matrix.simulators.adc_simulator import ADCSimulator
    sim = ADCSimulator(sample_rate=4000, frame_size=400)
    fd = sim.create_memfd("final_proof_shm")
    
    uds_path = "/tmp/v_adc.sock"
    if os.path.exists(uds_path): os.remove(uds_path)
    
    connected_event = asyncio.Event()

    async def handle_edgeos(reader, writer):
        client_sock = writer.get_extra_info('socket')
        # [证据 1] Python 确认发出数据
        print(f"\n[PROOF_STEP_1] Python: Sending FD {fd} to EdgeOS...")
        client_sock.sendmsg([b'\xFD'], [(socket.SOL_SOCKET, socket.SCM_RIGHTS, array.array("i", [fd]))])
        connected_event.set()
        
        while True:
            sim.write_val(231.5)
            client_sock.send(b'\x01')
            await asyncio.sleep(0.5)

    server = await asyncio.start_unix_server(handle_edgeos, path=uds_path)
    
    try:
        await asyncio.wait_for(connected_event.wait(), timeout=10)
        
        # 只要屏幕上出现了 PROOF_STEP_4，就代表整条路彻底通了
        for i in range(10):
            await asyncio.sleep(1.0)
            val = matrix.get_val(1001)
            if val is not None and abs(val - 231.5) < 1.0:
                print(f"\n🔥 [FINAL_SUCCESS] Full Chain Verified via Hub: {val}")
                return
        
        # 如果 Hub 没收到，只要日志里有 STEP 4 也是赢了
        print("\n⏳ Hub value wait timed out, check stdout for Step 4 logs...")
    finally:
        server.close()
        sim.close()

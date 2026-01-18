import asyncio
import numpy as np
import socket
import array
import os
import time
from tools.matrix.simulators.adc_simulator import ADCSimulator

async def injector():
    sim = ADCSimulator(sample_rate=4000, frame_size=400) 
    fd = sim.create_memfd("final_shot")
    
    uds_path = "/tmp/v_adc.sock"
    if os.path.exists(uds_path): os.remove(uds_path)
    
    connected_event = asyncio.Event()
    client_sock = None

    async def handle_edgeos(reader, writer):
        nonlocal client_sock
        client_sock = writer.get_extra_info('socket')
        print(f"📡 [Python] EdgeOS connected. Sending FD {fd}...")
        # 初始发送 FD
        client_sock.sendmsg([b'\xFD'], [(socket.SOL_SOCKET, socket.SCM_RIGHTS, array.array("i", [fd]))])
        connected_event.set()

    server = await asyncio.start_unix_server(handle_edgeos, path=uds_path)
    
    await connected_event.wait()
    
    count = 0
    while True:
        # 每 10 次触发重发一次 FD，确保 C 侧缓存有效
        if count % 10 == 0:
            client_sock.sendmsg([b'\xFD'], [(socket.SOL_SOCKET, socket.SCM_RIGHTS, array.array("i", [fd]))])
            
        sim.write_val(220.0 + np.random.normal(0, 1.0))
        client_sock.send(b'\x01')
        count += 1
        await asyncio.sleep(0.1)

if __name__ == "__main__":
    asyncio.run(injector())
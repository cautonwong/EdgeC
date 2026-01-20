#!/usr/bin/env python3
import os
import sys
import time
import math
import mmap
import socket
import struct
import array

# 使用 Linux 特有的 memfd_create (通过 ctypes 调用)
import ctypes
libc = ctypes.CDLL("libc.so.6")
SYS_memfd_create = 319 # x86_64

def create_memfd(name, size):
    fd = libc.syscall(SYS_memfd_create, name, 1) # MFD_ALLOW_SEALING
    if fd < 0:
        raise OSError("memfd_create failed")
    os.ftruncate(fd, size)
    return fd

def send_fd(sock, fd):
    # 使用 socket.sendmsg 发送文件描述符 (SCM_RIGHTS)
    sock.sendmsg([b"\x01"], [(socket.SOL_SOCKET, socket.SCM_RIGHTS, struct.pack("i", fd))])

def main():
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("--socket", default="/tmp/v_adc_sim.sock")
    args = parser.parse_args()

    # 1. 准备共享内存 (1600 个点, 假设每个点是 float32)
    sample_count = 1600
    mem_size = sample_count * 4
    fd = create_memfd("adc_waveform", mem_size)
    
    # 映射内存以便写入
    buf = mmap.mmap(fd, mem_size, mmap.MAP_SHARED, mmap.PROT_WRITE)

    # 2. 准备 UDS 服务端
    if os.path.exists(args.socket): os.unlink(args.socket)
    server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    server.bind(args.socket)
    server.listen(1)

    print(f"📡 ADC Simulator: Generating sine wave in memfd, waiting for EdgeOS at {args.socket}...")

    phase = 0.0
    while True:
        try:
            # 等待 EdgeOS 连接
            conn, _ = server.accept()
            print("  [ADC] EdgeOS connected. Sending memfd handle...")
            
            # 发送 FD
            send_fd(conn, fd)
            time.sleep(0.1) # 给予内核处理 SCM_RIGHTS 的时间
            
            # 持续更新波形
            while True:
                # 生成 50Hz 正弦波
                for i in range(sample_count):
                    val = 220.0 * math.sin(phase + (2 * math.pi * 50.0 * i / 12800.0))
                    struct.pack_into("f", buf, i * 4, val)
                
                phase += 0.1 # 模拟相位变化
                
                # 通知 EdgeOS (发送一个字节脉冲)
                try:
                    conn.send(b"\x01")
                except:
                    break
                    
                time.sleep(0.02) # 50Hz 刷新
        except Exception as e:
            print(f"  [ADC] Connection lost: {e}")
            time.sleep(1)

if __name__ == "__main__":
    main()

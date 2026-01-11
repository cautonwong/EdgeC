import socket
import os
import threading
import time

class VBusSniffer:
    """
    极简同步版 VBus 嗅探代理：使用多线程转发，避免 asyncio 复杂性。
    """
    def __init__(self, internal_path, public_path, log_file):
        self.internal_path = internal_path
        self.public_path = public_path
        self.log_file = log_file
        self.running = False

    def _proxy_worker(self, source, target, direction):
        try:
            while self.running:
                data = source.recv(4096)
                if not data: break
                
                # 记录十六进制流
                with open(self.log_file, "a") as f:
                    hex_data = data.hex(' ').upper()
                    ts = time.strftime("%H:%M:%S", time.localtime())
                    f.write(f"[{ts}] [{direction}] {hex_data}\n")
                
                target.sendall(data)
        except:
            pass
        finally:
            try: source.close()
            except: pass
            try: target.close()
            except: pass

    def _server_loop(self):
        if os.path.exists(self.public_path): os.remove(self.public_path)
        server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        server.bind(self.public_path)
        server.listen(5)
        self.running = True
        print(f"🕵️ [Sniffer] Proxy active: {self.public_path} <-> {self.internal_path}")

        while self.running:
            try:
                server.settimeout(1.0)
                client_conn, _ = server.accept()
                
                # 连接到 EdgeC 内部 
                edgec_conn = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
                connected = False
                for _ in range(10):
                    try:
                        edgec_conn.connect(self.internal_path)
                        connected = True
                        break
                    except:
                        time.sleep(0.2)
                
                if not connected:
                    print(f"❌ [Sniffer] Failed to connect to EdgeC internal")
                    client_conn.close()
                    continue

                # 启动双向转发线程
                threading.Thread(target=self._proxy_worker, args=(client_conn, edgec_conn, "ACTOR -> EDGEC"), daemon=True).start()
                threading.Thread(target=self._proxy_worker, args=(edgec_conn, client_conn, "EDGEC -> ACTOR"), daemon=True).start()
            except socket.timeout:
                continue
            except Exception as e:
                if self.running: print(f"❌ [Sniffer] Accept error: {e}")

    def start(self):
        threading.Thread(target=self._server_loop, daemon=True).start()

    def stop(self):
        self.running = False
        if os.path.exists(self.public_path):
            try: os.remove(self.public_path)
            except: pass
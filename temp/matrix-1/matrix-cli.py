import cmd
import socket
import json
import sys
import time

class MatrixShell(cmd.Cmd):
    intro = 'Matrix Interactive Workstation. Type help or ? to list commands.\n'
    prompt = '(matrix) '
    
    def __init__(self, host, port):
        super().__init__()
        self.host = host
        self.port = port
        self.sock = None
        self._connect()

    def _connect(self):
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.settimeout(2.0)
            self.sock.connect((self.host, self.port))
        except Exception as e:
            print(f"Error: Could not connect to Matrix Server: {e}")
            sys.exit(1)

    def _send(self, line):
        try:
            self.sock.sendall((line + "\n").encode())
            data = self.sock.recv(8192)
            return data.decode().strip()
        except Exception as e:
            print(f"Error: {e}")
            return None

    def do_set(self, arg):
        """SET <device> <tag> <value>: Change register value."""
        res = self._send(f"SET {arg}")
        if res: print(res)

    def do_monitor(self, arg):
        """MONITOR <tag_id>: Enter real-time observation mode. Press Ctrl+C to stop."""
        if not arg:
            print("Usage: MONITOR <tag_id>")
            return
        
        print(f"Observing Tag {arg}... (Ctrl+C to exit)")
        try:
            while True:
                res = self._send(f"MONITOR {arg}")
                if res:
                    try:
                        js = json.loads(res)
                        val = js.get("value", "N/A")
                        print(f"\r[{time.strftime('%H:%M:%S')}] Tag {arg} -> {val}      ", end="", flush=True)
                    except: 
                        print(f"\n{res}")
                time.sleep(0.5)
        except KeyboardInterrupt:
            print("\nMonitoring stopped.")

    def do_list(self, arg):
        """LIST: Show all simulated devices."""
        res = self._send("LIST")
        if res:
            try:
                js = json.loads(res)
                print(f"Simulated Devices: {', '.join(js['devices'])}")
            except: print(res)

    def do_status(self, arg):
        """STATUS <device>: Show raw datastore."""
        res = self._send(f"STATUS {arg}")
        if res: print(res)

    def do_exit(self, arg):
        """Quit the shell."""
        return True

if __name__ == '__main__':
    host = '127.0.0.1'
    if len(sys.argv) > 1: host = sys.argv[1]
    MatrixShell(host, 9999).cmdloop()

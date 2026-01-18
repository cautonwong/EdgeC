import pytest
import asyncio
from unittest.mock import MagicMock, patch
from tools.matrix.matrix_cli import MatrixShell

# Mock socket to avoid real networking
class MockSocket:
    def __init__(self, *args, **kwargs):
        self.sent_data = []
        self.recv_queue = []
    
    def connect(self, addr): pass
    def settimeout(self, t): pass
    
    def sendall(self, data):
        self.sent_data.append(data)
    
    def recv(self, bufsize):
        if self.recv_queue:
            return self.recv_queue.pop(0)
        return b""

@pytest.fixture
def mock_socket_cls():
    with patch("socket.socket") as m:
        # 每次调用 socket.socket() 返回一个新的 MockSocket 实例
        # 我们需要在测试里获取这个实例，所以这里用 side_effect
        mock_instance = MockSocket()
        m.return_value = mock_instance
        yield mock_instance

def test_cli_set_command(mock_socket_cls, capsys):
    # Setup mock
    sock = mock_socket_cls
    shell = MatrixShell("localhost", 9999)
    # shell.sock 已经被 mock_socket_cls 替换（通过 patch）
    # 但由于 MockSocket 是我们在测试里控制的，shell.sock 引用的是同一个对象吗？
    # 是的，因为 socket.socket() 返回了 mock_socket_cls (return_value)
    
    # Simulate server response
    sock.recv_queue.append(b"OK: Set Tag1=100")
    
    shell.onecmd("set Device1 Tag1 100")
    
    # Verify sent command
    assert len(sock.sent_data) > 0
    assert sock.sent_data[0] == b"SET Device1 Tag1 100\n"
    
    # Verify output
    captured = capsys.readouterr()
    assert "OK: Set Tag1=100" in captured.out

def test_cli_monitor_command(mock_socket_cls):
    # Setup
    sock = mock_socket_cls
    shell = MatrixShell("localhost", 9999)
    
    # Simulate valid JSON response for monitor
    resp = b'{"value": 123.45}'
    sock.recv_queue.append(resp)
    
    res = shell._send("MONITOR 100")
    assert res == '{"value": 123.45}'
    assert sock.sent_data[0] == b"MONITOR 100\n"

def test_cli_list_command(mock_socket_cls, capsys):
    sock = mock_socket_cls
    shell = MatrixShell("localhost", 9999)
    
    sock.recv_queue.append(b'{"devices": ["DevA", "DevB"]}')
    shell.onecmd("list")
    
    captured = capsys.readouterr()
    assert "Simulated Devices: DevA, DevB" in captured.out

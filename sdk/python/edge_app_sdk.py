"""Thin client for the frozen EdgeC app host ABI v1.

Wire format is one request/response per message: a 4-byte big-endian length
prefix followed by UTF-8 JSON. See sdk/README.md for the contract.
"""

import json
import struct

ABI_VERSION = "1.0.0"


class SdkError(Exception):
    """A typed host error. `code` keeps the original DENIED_* value."""

    def __init__(self, code):
        super().__init__(code)
        self.code = code


def encode_value(value):
    if isinstance(value, bool):
        return {"boolean": value}
    if isinstance(value, int):
        return {"integer": value}
    if isinstance(value, float):
        return {"float": value}
    if isinstance(value, str):
        return {"text": value}
    raise TypeError("unsupported point value: %s" % type(value).__name__)


def decode_value(encoded):
    for kind in ("float", "integer", "boolean", "text"):
        if kind in encoded:
            return encoded[kind]
    raise SdkError("protocol")


def _read_exact(sock, size):
    chunks = []
    while size:
        chunk = sock.recv(size)
        if not chunk:
            raise ConnectionError("host closed the connection")
        chunks.append(chunk)
        size -= len(chunk)
    return b"".join(chunks)


def read_frame(sock):
    (size,) = struct.unpack(">I", _read_exact(sock, 4))
    return json.loads(_read_exact(sock, size).decode("utf-8"))


def write_frame(sock, message):
    payload = json.dumps(message, separators=(",", ":"), ensure_ascii=False).encode("utf-8")
    sock.sendall(struct.pack(">I", len(payload)) + payload)


class AppSdk:
    def __init__(self, sock):
        self._sock = sock
        self._next_id = 1
        self._exports = {}

    def close(self):
        self._sock.close()

    def set_exports(self, init=None, on_event=None, stop=None):
        self._exports = {"app_init": init, "app_on_event": on_event, "app_stop": stop}

    def dispatch(self, request):
        request_id = request.get("id", 0)
        fn = request.get("fn")
        args = request.get("args") or {}
        handler = self._exports.get(fn)
        if handler is None:
            return {"id": request_id, "ok": False, "code": "not_found"}
        if fn == "app_on_event":
            handler(args)
        else:
            handler()
        return {"id": request_id, "ok": True, "result": {}}

    def _call(self, fn, args):
        request_id = self._next_id
        self._next_id += 1
        write_frame(self._sock, {"id": request_id, "fn": fn, "args": args})
        response = read_frame(self._sock)
        if response.get("id") != request_id:
            raise SdkError("protocol")
        if not response.get("ok"):
            raise SdkError(response.get("code", "internal"))
        return response.get("result", {})

    def point_resolve(self, name):
        result = self._call("point_resolve", {"name": name})
        return result["handle"], result["access"]

    def point_read(self, handle):
        result = self._call("point_read", {"handle": handle})
        return decode_value(result["value"]), result["quality"], result.get("source_ts", 0)

    def point_subscribe(self, handle, deadband=0.0, min_interval_ms=0, max_interval_ms=0):
        result = self._call(
            "point_subscribe",
            {
                "handle": handle,
                "deadband": deadband,
                "min_interval_ms": min_interval_ms,
                "max_interval_ms": max_interval_ms,
            },
        )
        return result["subscription"]

    def point_publish(self, handle, value, quality="good", source_ts=0):
        self._call(
            "point_publish",
            {
                "handle": handle,
                "sample": {
                    "value": encode_value(value),
                    "quality": quality,
                    "source_ts": source_ts,
                },
            },
        )

    def log(self, level, message):
        self._call("log", {"level": level, "message": message})

    def now(self):
        return self._call("now", {})["now_ms"]

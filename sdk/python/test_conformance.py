#!/usr/bin/env python3
"""Run the shared SDK conformance vectors against the Python SDK."""

import json
import os
import socket
import struct
import sys
import threading

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from edge_app_sdk import AppSdk, SdkError  # noqa: E402


def load_vectors():
    for prefix in ("", "../", "../../"):
        path = os.path.join(prefix, "sdk/conformance/vectors.json")
        if os.path.exists(path):
            with open(path, "r", encoding="utf-8") as fp:
                return json.load(fp)
    raise SystemExit("cannot find sdk/conformance/vectors.json")


def read_frame(sock):
    size = struct.unpack(">I", _read_exact(sock, 4))[0]
    return json.loads(_read_exact(sock, size).decode("utf-8"))


def write_frame(sock, message):
    payload = json.dumps(message, separators=(",", ":")).encode("utf-8")
    sock.sendall(struct.pack(">I", len(payload)) + payload)


def _read_exact(sock, size):
    chunks = []
    while size:
        chunk = sock.recv(size)
        if not chunk:
            raise ConnectionError("peer closed the connection")
        chunks.append(chunk)
        size -= len(chunk)
    return b"".join(chunks)


def decode_value(encoded):
    for kind in ("float", "integer", "boolean", "text"):
        if kind in encoded:
            return encoded[kind]
    raise AssertionError("unknown value tag: %r" % (encoded,))


def call_step(sdk, fn, args):
    if fn == "point_resolve":
        return sdk.point_resolve(args["name"])
    if fn == "point_read":
        return sdk.point_read(args["handle"])
    if fn == "point_subscribe":
        return sdk.point_subscribe(
            args["handle"],
            args["deadband"],
            args["min_interval_ms"],
            args["max_interval_ms"],
        )
    if fn == "point_publish":
        sample = args["sample"]
        return sdk.point_publish(
            args["handle"], decode_value(sample["value"]), sample["quality"],
            sample["source_ts"],
        )
    if fn == "log":
        return sdk.log(args["level"], args["message"])
    if fn == "now":
        return sdk.now()
    raise AssertionError("unknown fn: %s" % fn)


def run_step(sdk, step):
    fn = step["fn"]
    args = step["args"]
    expect = step["expect"]
    if not expect["ok"]:
        try:
            call_step(sdk, fn, args)
        except SdkError as exc:
            if exc.code != expect["code"]:
                raise AssertionError(
                    "%s: got %s, want %s" % (fn, exc.code, expect["code"])
                ) from exc
            return
        raise AssertionError("%s: expected %s, call succeeded" % (fn, expect["code"]))

    result = call_step(sdk, fn, args)
    want = expect["result"]
    if fn == "point_resolve":
        assert result == (want["handle"], want["access"]), result
    elif fn == "point_read":
        value, quality, source_ts = result
        assert value == decode_value(want["value"]), value
        assert quality == want["quality"], quality
        assert source_ts == want["source_ts"], source_ts
    elif fn == "point_subscribe":
        assert result == want["subscription"], result
    elif fn in ("point_publish", "log"):
        assert result is None, result
    elif fn == "now":
        assert result == want["now_ms"], result


class FakeHost(threading.Thread):
    def __init__(self, sock, steps, logs):
        super().__init__(daemon=True)
        self.sock = sock
        self.steps = steps
        self.logs = logs
        self.error = None

    def run(self):
        try:
            for index, step in enumerate(self.steps):
                request = read_frame(self.sock)
                assert request["fn"] == step["fn"], (
                    "step %d fn got %s want %s"
                    % (index, request["fn"], step["fn"])
                )
                assert request["args"] == step["args"], (
                    "step %d args got %r want %r"
                    % (index, request["args"], step["args"])
                )
                if step["fn"] == "log":
                    self.logs.append(step["args"]["message"])
                expect = step["expect"]
                response = {"id": request["id"]}
                if expect["ok"]:
                    response.update(ok=True, result=expect["result"])
                else:
                    response.update(ok=False, code=expect["code"])
                write_frame(self.sock, response)
        except Exception as exc:  # surfaced by the vector runner
            self.error = exc


def run_vector(vector):
    app_sock, host_sock = socket.socketpair()
    app_sock.settimeout(2)
    sdk = AppSdk(app_sock)
    logs = []
    host = FakeHost(host_sock, vector["steps"], logs)
    host.start()
    failure = []

    def on_event(event):
        try:
            for step in vector["steps"]:
                run_step(sdk, step)
        except Exception as exc:  # surfaced after the host joins
            failure.append(exc)

    sdk.set_exports(init=lambda: None, on_event=on_event, stop=lambda: None)
    assert sdk.dispatch({"id": 1, "fn": "app_init", "args": {}})["ok"]
    assert sdk.dispatch({"id": 2, "fn": "app_on_event", "args": vector["event"]})["ok"]
    assert sdk.dispatch({"id": 3, "fn": "app_stop", "args": {}})["ok"]

    sdk.close()
    try:
        host_sock.shutdown(socket.SHUT_RDWR)
    except OSError:
        pass
    host_sock.close()
    host.join(timeout=2)
    if host.is_alive():
        raise AssertionError("fake host did not finish")
    if host.error is not None:
        raise host.error
    if failure:
        raise failure[0]
    assert logs == vector["expect_log"], "logs got %r want %r" % (
        logs,
        vector["expect_log"],
    )


def main():
    vectors = load_vectors()["vectors"]
    passed = 0
    for vector in vectors:
        try:
            run_vector(vector)
            passed += 1
            print("PASS %s" % vector["name"])
        except Exception as exc:
            print("FAIL %s: %s" % (vector["name"], exc))
    print("sdk conformance (python): %d/%d vectors passed" % (passed, len(vectors)))
    return 0 if passed == len(vectors) else 1


if __name__ == "__main__":
    sys.exit(main())

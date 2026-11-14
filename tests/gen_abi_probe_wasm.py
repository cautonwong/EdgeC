#!/usr/bin/env python3
"""Generate tests/fixtures/abi_probe.wasm.

The probe imports the Host ABI and implements app_init:
  1. point_resolve("app.demo.value") -> handle, access
  2. point_read(1001) -> value
  3. point_publish(handle, value + 1.0)
  4. app_log(1, "probe done")

There is no wasm toolchain in this environment, so the module is encoded by
hand. Re-run after changing the ABI: python3 tests/gen_abi_probe_wasm.py
"""

import struct

I32, I64, F64 = 0x7F, 0x7E, 0x7C


def leb(value: int) -> bytes:
    out = bytearray()
    while True:
        byte = value & 0x7F
        value >>= 7
        if value:
            out.append(byte | 0x80)
        else:
            out.append(byte)
            return bytes(out)


def sleb(value: int) -> bytes:
    out = bytearray()
    more = True
    while more:
        byte = value & 0x7F
        value >>= 7
        if (value == 0 and not (byte & 0x40)) or (value == -1 and (byte & 0x40)):
            more = False
        else:
            byte |= 0x80
        out.append(byte)
    return bytes(out)


def vec(items) -> bytes:
    return leb(len(items)) + b"".join(items)


def name(text: str) -> bytes:
    raw = text.encode()
    return leb(len(raw)) + raw


def section(section_id: int, payload: bytes) -> bytes:
    return bytes([section_id]) + leb(len(payload)) + payload


def functype(params, results) -> bytes:
    return b"\x60" + vec([bytes([p]) for p in params]) + vec([bytes([r]) for r in results])


def main() -> None:
    types = [
        functype([], []),                                  # 0: app_init
        functype([I32, I32, I32, I32], [I32]),             # 1: point_read
        functype([I32, F64, I32, I64], [I32]),             # 2: point_publish
        functype([I32, I32, I32], [I32]),                  # 3: point_resolve
        functype([I32, I32], []),                          # 4: app_log
    ]
    imports = [
        name("env") + name("point_resolve") + b"\x00" + leb(3),
        name("env") + name("point_read") + b"\x00" + leb(1),
        name("env") + name("point_publish") + b"\x00" + leb(2),
        name("env") + name("app_log") + b"\x00" + leb(4),
    ]
    functions = [leb(0)]
    memory = b"\x01\x00\x01"
    exports = [
        name("app_init") + b"\x00" + leb(4),
        name("memory") + b"\x02" + leb(0),
    ]

    body = b"\x00"  # no locals
    body += b"\x41" + sleb(64) + b"\x41" + sleb(0) + b"\x41" + sleb(4) + b"\x10" + leb(0) + b"\x1a"
    body += (b"\x41" + sleb(1001) + b"\x41" + sleb(8) + b"\x41" + sleb(16) + b"\x41" +
             sleb(24) + b"\x10" + leb(1) + b"\x1a")
    body += b"\x41" + sleb(0) + b"\x28" + leb(2) + leb(0)          # i32.load handle
    body += b"\x41" + sleb(8) + b"\x2b" + leb(3) + leb(0)          # f64.load value
    body += b"\x44" + struct.pack("<d", 1.0) + b"\xa0"            # f64.add
    body += b"\x41" + sleb(0) + b"\x42" + sleb(0)                 # quality 0, ts 0
    body += b"\x10" + leb(2) + b"\x1a"                            # point_publish
    body += b"\x41" + sleb(1) + b"\x41" + sleb(80) + b"\x10" + leb(3)  # app_log
    body += b"\x0b"
    code = [leb(len(body)) + body]

    data = b"\x02" + b"\x02" + leb(0) + b"\x41" + sleb(64) + b"\x0b" + name("app.demo.value\x00")
    data += b"\x02" + leb(0) + b"\x41" + sleb(80) + b"\x0b" + name("probe done\x00")

    module = b"\x00asm\x01\x00\x00\x00"
    module += section(1, vec(types))
    module += section(2, vec(imports))
    module += section(3, vec(functions))
    module += section(5, memory)
    module += section(7, vec(exports))
    module += section(10, vec(code))
    module += section(11, data)

    with open("tests/fixtures/abi_probe.wasm", "wb") as out:
        out.write(module)
    print(f"wrote tests/fixtures/abi_probe.wasm ({len(module)} bytes)")


if __name__ == "__main__":
    main()

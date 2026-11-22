import json
import threading

from bench import dump_json_atomic
from testplatform.cases import case


@case(id="DCU_00_00_01_000003_01",
      clause="free: 总线计数文件必须原子替换，读者不得看到半截 JSON",
      tags=["验收"])
def test_dump_json_atomic_never_exposes_a_torn_file(tmp_path):
    target = tmp_path / "bus.json"
    payloads = [{"i": i, "data": f"{i:04x}" * 8} for i in range(500)]
    dump_json_atomic(target, payloads[0])

    errors: list[str] = []
    reads = 0
    stop = threading.Event()

    def reader() -> None:
        nonlocal reads
        while not stop.is_set():
            try:
                text = target.read_text(encoding="utf-8")
            except FileNotFoundError as exc:
                errors.append(f"文件在替换窗口内消失：{exc}")
                continue
            try:
                json.loads(text)
            except json.JSONDecodeError as exc:
                errors.append(f"半截 JSON：{exc}：{text!r}")
                return
            reads += 1

    thread = threading.Thread(target=reader)
    thread.start()
    for payload in payloads:
        dump_json_atomic(target, payload)
    stop.set()
    thread.join(timeout=5)

    assert not errors, errors[:3]
    assert reads > 0
    assert json.loads(target.read_text(encoding="utf-8")) == payloads[-1]

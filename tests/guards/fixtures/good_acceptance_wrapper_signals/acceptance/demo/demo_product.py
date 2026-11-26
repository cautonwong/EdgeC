import signal
import subprocess

proc = subprocess.Popen(["demo"])


def _forward(signum, _frame):
    if proc.poll() is None:
        proc.terminate()
        proc.wait(timeout=3)
    raise SystemExit(128 + signum)


signal.signal(signal.SIGTERM, _forward)
signal.signal(signal.SIGINT, _forward)

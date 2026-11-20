import shutil
import subprocess

cmd = [shutil.which("stdbuf"), "-oL", IEMS_BIN]
proc = subprocess.Popen(cmd, env=env)

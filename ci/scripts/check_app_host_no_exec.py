#!/usr/bin/env python3
"""Keep app-host unable to spawn processes.

The launcher's seccomp filter still allows execve/execveat for its own
fexecve, so a compromised app-host could exec a binary if the source ever
gained such a call. app-host, its SDK, and the host module never need one.
"""

import argparse
import os
import re
import sys

from c_source_scan import SHARED_PRUNE_DIRS

TARGETS = ("modules/edge_app_host", "cmd/app-host", "sdk/c")
FORBIDDEN = re.compile(
    r"\b(?:execv|execve|execvp|execl|execle|execlp|execveat|posix_spawn|system|popen)\s*\("
)
COMMENTS = re.compile(r"/\*.*?\*/|//[^\n]*", re.DOTALL)


def scan(root):
    violations = []
    for target in TARGETS:
        base = os.path.join(root, target)
        if not os.path.isdir(base):
            continue
        for dirpath, dirnames, filenames in os.walk(base):
            dirnames[:] = [d for d in dirnames if d not in SHARED_PRUNE_DIRS]
            for name in filenames:
                if not name.endswith((".c", ".h")):
                    continue
                path = os.path.join(dirpath, name)
                with open(path, "r", encoding="utf-8", errors="replace") as handle:
                    text = handle.read()
                stripped = COMMENTS.sub("", text)
                for match in FORBIDDEN.finditer(stripped):
                    line = stripped.count("\n", 0, match.start()) + 1
                    violations.append(f"{path}:{line}: {match.group(0).strip()}")
    return violations


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", default=".")
    args = parser.parse_args()
    violations = scan(os.path.abspath(args.root))
    if violations:
        print("[APP_HOST_NO_EXEC] process spawn calls in app platform:")
        for item in violations:
            print("  " + item)
        return 1
    print("[APP_HOST_NO_EXEC] clean.")
    return 0


if __name__ == "__main__":
    sys.exit(main())

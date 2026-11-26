#!/usr/bin/env python3
"""Keep OpenSSL behind the crypto port.

The port is foundation/hal_crypto.h. Until the OpenSSL adapter lands, the
files that still include <openssl/*> are listed as debt; any new direct user
fails the guard. A missing production backend registration is reported as a
warning because the adapter is a separate task.
"""

import argparse
import glob
import os
import re
import sys

from c_source_scan import SHARED_PRUNE_DIRS

ROOTS = ("modules", "foundation", "cmd", "adapters")
OPENSSL = re.compile(r'#\s*include\s*[<"]openssl/')
DEBT = {
    "modules/edge_mgmt/src/edge_eapp.c": "Ed25519/SHA-256 pending port methods",
    "adapters/sqlite_adapters/src/edge_digest.c": "SHA-256 file and stream digest pending port migration",
    "modules/edge_stream/src/wasm_runtime/wasm_mgr.c": "RSA PKCS1v15 verify pending port method",
}


def scan(root):
    direct = {}
    for base_name in ROOTS:
        base = os.path.join(root, base_name)
        if not os.path.isdir(base):
            continue
        for dirpath, dirnames, filenames in os.walk(base):
            dirnames[:] = [d for d in dirnames if d not in SHARED_PRUNE_DIRS]
            for name in filenames:
                if not name.endswith((".c", ".h")):
                    continue
                path = os.path.join(dirpath, name)
                rel = os.path.relpath(path, root).replace(os.sep, "/")
                with open(path, "r", encoding="utf-8", errors="replace") as handle:
                    if OPENSSL.search(handle.read()):
                        direct[rel] = True
    registered = False
    for path in glob.glob(os.path.join(root, "cmd", "**", "*.c"), recursive=True):
        with open(path, "r", encoding="utf-8", errors="replace") as handle:
            if "hal_crypto_register(" in handle.read():
                registered = True
                break
    return sorted(direct), registered


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", default=".")
    args = parser.parse_args()
    root = os.path.abspath(args.root)
    direct, registered = scan(root)

    errors = [rel for rel in direct if rel not in DEBT]
    if errors:
        print("[CRYPTO_BOUNDARY] new direct OpenSSL users outside the port:")
        for rel in errors:
            print(f"  {rel}")
        return 1

    stale = [
        rel
        for rel in DEBT
        if rel not in direct and os.path.exists(os.path.join(root, rel))
    ]
    if stale:
        print(f"[CRYPTO_BOUNDARY] warning: {len(stale)} debt entr(ies) no longer include OpenSSL")
        for rel in stale:
            print(f"  {rel}")
    if not registered:
        print("[CRYPTO_BOUNDARY] warning: no production hal_crypto_register() call in cmd/")
    print(f"[CRYPTO_BOUNDARY] clean: {len(direct)} known direct user(s), "
          f"{len(DEBT) - len(stale)} debt item(s) pinned.")
    return 0


if __name__ == "__main__":
    sys.exit(main())

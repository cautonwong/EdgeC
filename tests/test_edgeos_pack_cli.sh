#!/usr/bin/env bash
# CLI journey: keygen, build, verify, determinism and rejection of the wrong key.
set -euo pipefail

pack="$1"
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT

"$pack" keygen --out-dir "$work/keys" >/dev/null
test -f "$work/keys/edgec-eapp.seed"
test -f "$work/keys/edgec-eapp.pub"

cat > "$work/app.json" <<'JSON'
{"abi":1,"id":"demo-app","version":"1.2.3","language":"c","execution_profile":"wasm","entry":"app.wasm","limits":{"heapKb":1,"storageKb":1,"cpuMsPerSec":1,"callsPerSec":1,"publishPerSec":1}}
JSON
printf '\x00asm\x01\x00\x00\x00' > "$work/app.wasm"

"$pack" build --manifest "$work/app.json" --wasm "$work/app.wasm" \
    --seed "$work/keys/edgec-eapp.seed" --out "$work/app.eapp"
"$pack" verify --package "$work/app.eapp" --pubkey "$work/keys/edgec-eapp.pub" |
    grep -q 'id=demo-app version=1.2.3'

"$pack" build --manifest "$work/app.json" --wasm "$work/app.wasm" \
    --seed "$work/keys/edgec-eapp.seed" --out "$work/app2.eapp"
cmp "$work/app.eapp" "$work/app2.eapp"

"$pack" keygen --out-dir "$work/other" >/dev/null
if "$pack" verify --package "$work/app.eapp" --pubkey "$work/other/edgec-eapp.pub" 2>/dev/null; then
    echo "wrong key accepted" >&2
    exit 1
fi

if "$pack" keygen --out-dir "$work/keys" 2>/dev/null; then
    echo "keygen overwrote an existing key" >&2
    exit 1
fi

echo "cli ok"

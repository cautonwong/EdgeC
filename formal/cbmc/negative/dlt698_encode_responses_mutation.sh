#!/usr/bin/env bash
set -euo pipefail

: "${ROOT_DIR:?ROOT_DIR is required}"
: "${FORMAL_OUT_DIR:?FORMAL_OUT_DIR is required}"
: "${CBMC_BIN:?CBMC_BIN is required}"
: "${ARTIFACT:?ARTIFACT is required}"

SOURCE="foundation/src/protocol/dlt698_codec.c"
LOG_DIR="$FORMAL_OUT_DIR/negative"
LOG="$LOG_DIR/$(basename "${BASH_SOURCE[0]}" .sh).log"
mkdir -p "$LOG_DIR"

tmp_dir="$(mktemp -d)"
trap 'rm -rf "$tmp_dir"' EXIT
cp "$ROOT_DIR/$SOURCE" "$tmp_dir/dlt698_codec.c"
python3 - "$tmp_dir/dlt698_codec.c" <<'PY'
import sys

path = sys.argv[1]
text = open(path, encoding="utf-8").read()
guards = [
    "    if (!_fits(b, sa, 10u, 0u)) return -1;\n",
    "    if (!_fits(b, sa, 75u, 0u)) return -1;\n",
    "    if (!_fits(b, sa, 5u, 0u)) return -1;\n",
    "    if (!_fits(b, sa, 11u, dlen)) return -1;\n",
]
for guard in guards:
    if text.count(guard) != 1:
        sys.exit(f"guard occurrence count is {text.count(guard)}, expected 1: {guard!r}")
    text = text.replace(guard, "", 1)
open(path, "w", encoding="utf-8").write(text)
PY

INCLUDES=(-I"$ROOT_DIR/foundation/include")
for dep_include in "$ROOT_DIR/.local/linux-x86_64/include" "$ROOT_DIR/.local/linux-aarch64/include"; do
    [ -d "$dep_include" ] && INCLUDES+=(-I"$dep_include")
done

rc=0
timeout 60 "$CBMC_BIN" "$ROOT_DIR/$ARTIFACT" \
    -DDLT698_CODEC_SOURCE="\"$tmp_dir/dlt698_codec.c\"" -DFORMAL_MAX_LEN=24 \
    "${INCLUDES[@]}" --bounds-check --pointer-check --div-by-zero-check \
    --signed-overflow-check --conversion-check --unwind 25 --unwinding-assertions \
    >"$LOG" 2>&1 || rc=$?
[ "$rc" -ne 124 ] || { echo "CBMC timed out on the mutated copy: $LOG" >&2; exit 1; }
[ "$rc" -ne 0 ] || { echo "mutated copy still verified; the guard is not load-bearing: $LOG" >&2; exit 1; }
grep -qE 'dlt698_encode_responses: (writes within storage|encoded length within cap|-1 iff invalid or insufficient capacity): FAILURE' "$LOG" || {
    echo "expected dlt698_encode_responses capacity FAILURE not found: $LOG" >&2
    exit 1
}

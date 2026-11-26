#!/usr/bin/env bash
# Mutation negative for the Modbus decode harness. Removing the
# `in->len < resp_len` guard must make CBMC report an out-of-bounds read in
# _get_crc; the production tree is never edited.
#
# Contract (set by ci/scripts/check_formal_negative.sh):
#   ROOT_DIR        repository root
#   FORMAL_OUT_DIR  shard output directory; the log goes to
#                   $FORMAL_OUT_DIR/negative/modbus_guard_mutation.log
#   CBMC_BIN        pinned CBMC binary
#   ARTIFACT        CBMC harness path relative to ROOT_DIR
# Exits non-zero when the mutated copy still verifies or the expected pointer
# failure is missing.
set -euo pipefail

: "${ROOT_DIR:?ROOT_DIR is required}"
: "${FORMAL_OUT_DIR:?FORMAL_OUT_DIR is required}"
: "${CBMC_BIN:?CBMC_BIN is required}"
: "${ARTIFACT:?ARTIFACT is required}"

SOURCE="foundation/src/protocol/modbus_codec.c"
LOG_DIR="$FORMAL_OUT_DIR/negative"
LOG="$LOG_DIR/$(basename "${BASH_SOURCE[0]}" .sh).log"
mkdir -p "$LOG_DIR"

tmp_dir="$(mktemp -d)"
trap 'rm -rf "$tmp_dir"' EXIT
cp "$ROOT_DIR/$SOURCE" "$tmp_dir/modbus_codec.c"
python3 - "$tmp_dir/modbus_codec.c" <<'PY'
import sys

path = sys.argv[1]
text = open(path, encoding="utf-8").read()
guard = (
    "                    if (in->len < resp_len) {\n"
    "                        return 1; // Incomplete\n"
    "                    }\n"
)
if text.count(guard) != 1:
    sys.exit(f"guard occurrence count is {text.count(guard)}, expected 1")
open(path, "w", encoding="utf-8").write(text.replace(guard, "", 1))
PY

INCLUDES=(-I"$ROOT_DIR/foundation/include")
for dep_include in "$ROOT_DIR/.local/linux-x86_64/include" "$ROOT_DIR/.local/linux-aarch64/include"; do
    [ -d "$dep_include" ] && INCLUDES+=(-I"$dep_include")
done

rc=0
timeout 120 "$CBMC_BIN" "$ROOT_DIR/$ARTIFACT" \
    -DMODBUS_CODEC_SOURCE="\"$tmp_dir/modbus_codec.c\"" -DFORMAL_MAX_LEN=16 \
    "${INCLUDES[@]}" --bounds-check --pointer-check --div-by-zero-check \
    --signed-overflow-check --conversion-check --unwind 17 --unwinding-assertions \
    >"$LOG" 2>&1 || rc=$?
[ "$rc" -ne 124 ] || { echo "CBMC timed out on the mutated copy: $LOG" >&2; exit 1; }
[ "$rc" -ne 0 ] || { echo "mutated copy still verified; the guard is not load-bearing: $LOG" >&2; exit 1; }
grep -qE '\[_get_crc\.pointer_dereference\.[0-9]+\].*pointer outside object bounds.*: FAILURE' "$LOG" || {
    echo "expected _get_crc out-of-bounds FAILURE not found: $LOG" >&2
    exit 1
}

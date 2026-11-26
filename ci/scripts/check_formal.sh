#!/usr/bin/env bash
# Manifest-driven formal gate. Every row in formal/manifest/*.tsv is executed
# according to its mode; a missing, unparsable or inconsistent manifest fails
# closed. Tool pins are re-checked on every run, so PATH/env substitutions and
# a stale jar fail even without --strict; only a missing tool degrades to SKIP.
# Budget: 300 s total, CBMC proof 240 s, CBMC aux 60 s, TLC 120 s per row.
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TOTAL_LIMIT=300
PROOF_LIMIT=240
AUX_LIMIT=60
TLA_LIMIT=120
OUT_DIR="${FORMAL_OUT_DIR:-$ROOT_DIR/build/formal}"

STRICT=0
if [ "${1:-}" = "--strict" ]; then
    STRICT=1
elif [ $# -gt 0 ]; then
    echo "usage: $0 [--strict]" >&2
    exit 2
fi

START=$SECONDS
fail() {
    echo "❌ [FORMAL] $*" >&2
    exit 1
}

# shellcheck source=ci/scripts/formal_tools.sh
. "$ROOT_DIR/ci/scripts/formal_tools.sh"

ROWS_FILE="$(mktemp)"
trap 'rm -f "$ROWS_FILE"' EXIT
if ! python3 "$ROOT_DIR/ci/scripts/formal_manifest.py" --root "$ROOT_DIR" --rows \
        > "$ROWS_FILE"; then
    fail "manifest parse failed"
fi
[ -s "$ROWS_FILE" ] || fail "no manifest rows"
mkdir -p "$OUT_DIR"
# The pre-shard runner wrote a single top-level run-manifest; drop the stale file
# so the shard directories are the only record.
rm -f "$OUT_DIR/run-manifest.tsv"

has_cbmc=0
has_tla=0
while IFS=$'\t' read -r artifact property covers tool bounds unwind max_len negative \
        sync owner runtime property_count state_count jar_sha256 mode cfg function assertions; do
    case "$mode" in
        proof|cover|function) has_cbmc=1 ;;
        tla) has_tla=1 ;;
        *) fail "unexpected mode '$mode'" ;;
    esac
done < "$ROWS_FILE"

while IFS= read -r owner; do
    mkdir -p "$OUT_DIR/$owner"
    printf 'artifact\tmode\tstatus\tcount\tseconds\n' > "$OUT_DIR/$owner/run-manifest.tsv"
done < <(cut -f10 "$ROWS_FILE" | sort -u)

CBMC_BIN=""
CBMC_VERSION=""
CBMC_READY=0
if [ "$has_cbmc" -eq 1 ]; then
    CBMC_BIN="$(formal_resolve_cbmc)"
    if [ -z "$CBMC_BIN" ]; then
        if [ "$STRICT" -eq 1 ]; then
            fail "SKIP CBMC: cbmc not found (--strict)"
        fi
        echo "SKIP CBMC (cbmc not found; run with --strict in CI)"
    else
        CBMC_VERSION="$("$CBMC_BIN" --version | head -1)"
        [ "$CBMC_VERSION" = "$FORMAL_CBMC_VERSION" ] || \
            fail "CBMC version is '$CBMC_VERSION', pinned '$FORMAL_CBMC_VERSION'"
        cbmc_sha="$(formal_sha256 "$CBMC_BIN")"
        [ "$cbmc_sha" = "$FORMAL_CBMC_SHA256" ] || \
            fail "CBMC binary SHA256 $cbmc_sha != pinned $FORMAL_CBMC_SHA256"
        CBMC_READY=1
    fi
fi

JAVA_BIN=""
TLA_JAR=""
TLA_READY=0
if [ "$has_tla" -eq 1 ]; then
    JAVA_BIN="$(formal_resolve_java)"
    TLA_JAR="$(formal_resolve_tla_jar)"
    if [ -z "$JAVA_BIN" ] || [ -z "$TLA_JAR" ] || [ ! -f "$TLA_JAR" ]; then
        if [ "$STRICT" -eq 1 ]; then
            fail "SKIP TLA+: java or tla2tools.jar not found (--strict)"
        fi
        echo "SKIP TLA+ (java or tla2tools.jar not found)"
    else
        java_major="$(formal_java_major "$JAVA_BIN")"
        [ "$java_major" = "$FORMAL_JAVA_MAJOR" ] || \
            fail "Java major is '${java_major:-unknown}', pinned $FORMAL_JAVA_MAJOR"
        TLA_READY=1
    fi
fi

record() { # owner artifact mode status count seconds
    printf '%s\t%s\t%s\t%s\t%s\n' "$2" "$3" "$4" "${5:-n/a}" "$6" \
        >> "$OUT_DIR/$1/run-manifest.tsv"
}

INCLUDE_ARGS=(-I"$ROOT_DIR/foundation/include")
for dep_include in "$ROOT_DIR/.local/linux-x86_64/include" "$ROOT_DIR/.local/linux-aarch64/include"; do
    [ -d "$dep_include" ] && INCLUDE_ARGS+=(-I"$dep_include")
done
CHECKS=(--bounds-check --pointer-check --div-by-zero-check --signed-overflow-check
        --conversion-check)

run_cbmc() { # limit log args...
    local limit="$1" log="$2"
    shift 2
    local rc=0
    timeout "$limit" "$CBMC_BIN" "$@" >"$log" 2>&1 || rc=$?
    return "$rc"
}

assertions_match() { # log assertions
    python3 - "$1" "$2" <<'PY'
import re
import sys

log = open(sys.argv[1], encoding="utf-8", errors="replace").read()
missing = [
    pattern for pattern in sys.argv[2].split(";")
    if not re.search(pattern, log, re.MULTILINE)
]
if missing:
    sys.exit("assertion(s) did not match: " + " | ".join(missing))
PY
}

CBMC_PROPERTY_SUM=0
CBMC_AUX=0
TLA_ROWS=0
TLA_STATES_LAST=""
TLA_VERSION_LAST=""

run_cbmc_row() {
    local stem="${artifact##*/}"
    stem="${stem%.*}"
    local log="$OUT_DIR/$owner/$stem.$mode.log"
    local limit="$AUX_LIMIT" count="n/a" rc=0 started=$SECONDS
    [ "$mode" = "proof" ] && limit="$PROOF_LIMIT"
    case "$mode" in
        proof)
            run_cbmc "$limit" "$log" "$ROOT_DIR/$artifact" "${INCLUDE_ARGS[@]}" \
                "${CHECKS[@]}" -DFORMAL_MAX_LEN="$max_len" --unwind "$unwind" \
                --unwinding-assertions || rc=$?
            ;;
        cover)
            run_cbmc "$limit" "$log" "$ROOT_DIR/$artifact" "${INCLUDE_ARGS[@]}" \
                -DFORMAL_MAX_LEN="$max_len" --cover cover --unwind "$unwind" || rc=$?
            ;;
        function)
            run_cbmc "$limit" "$log" "$ROOT_DIR/$artifact" "${INCLUDE_ARGS[@]}" \
                "${CHECKS[@]}" -DFORMAL_MAX_LEN="$max_len" --function "$function" || rc=$?
            ;;
    esac
    if [ "$rc" -eq 124 ]; then
        record "$owner" "$artifact" "$mode" "FAIL" "n/a" "$((SECONDS - started))"
        fail "CBMC timed out after ${limit}s: $log"
    fi
    if [ "$rc" -ne 0 ]; then
        grep -E 'FAILURE|VERIFICATION FAILED' "$log" | head -20 >&2 || true
        record "$owner" "$artifact" "$mode" "FAIL" "n/a" "$((SECONDS - started))"
        fail "$mode run failed: $log"
    fi
    if [ "$mode" = "proof" ]; then
        local summary failed total
        summary="$(sed -n 's/^\*\* \([0-9][0-9]*\) of \([0-9][0-9]*\) failed.*/\1 \2/p' "$log" | tail -1)"
        if [ -z "$summary" ]; then
            record "$owner" "$artifact" "$mode" "FAIL" "n/a" "$((SECONDS - started))"
            fail "cannot parse CBMC summary: $log"
        fi
        failed="${summary% *}"
        total="${summary#* }"
        if [ "$failed" -ne 0 ]; then
            record "$owner" "$artifact" "$mode" "FAIL" "$total" "$((SECONDS - started))"
            fail "CBMC reported $failed failed properties: $log"
        fi
        if [ "$total" -ne "$property_count" ]; then
            record "$owner" "$artifact" "$mode" "FAIL" "$total" "$((SECONDS - started))"
            fail "property count $total != manifest $property_count (assertions changed without a manifest update): $log"
        fi
        count="$total"
        CBMC_PROPERTY_SUM=$((CBMC_PROPERTY_SUM + total))
    else
        local assertion_error
        if ! assertion_error="$(assertions_match "$log" "$assertions" 2>&1)"; then
            record "$owner" "$artifact" "$mode" "FAIL" "n/a" "$((SECONDS - started))"
            fail "$assertion_error: $log"
        fi
        CBMC_AUX=$((CBMC_AUX + 1))
    fi
    record "$owner" "$artifact" "$mode" "PASS" "$count" "$((SECONDS - started))"
}

run_tla_row() {
    local stem="${artifact##*/}"
    stem="${stem%.*}"
    local tla_version dir log rc=0 started=$SECONDS states
    tla_version="$(printf '%s' "$tool" | sed -n 's/.*TLC \([0-9][0-9.]*\).*/\1/p')"
    [ -n "$tla_version" ] || fail "cannot read the TLC version from tool '$tool'"
    local jar_sha
    jar_sha="$(formal_sha256 "$TLA_JAR")"
    [ "$jar_sha" = "$jar_sha256" ] || \
        fail "TLA+ jar SHA256 $jar_sha != manifest $jar_sha256"
    dir="$OUT_DIR/$owner/tla/$stem"
    log="$OUT_DIR/$owner/tla/$stem.log"
    rm -rf "$dir"
    mkdir -p "$dir"
    cp "$ROOT_DIR/$artifact" "$dir/"
    cp "$ROOT_DIR/$cfg" "$dir/"
    ( cd "$dir" && timeout "$TLA_LIMIT" "$JAVA_BIN" -XX:+UseParallelGC \
        -cp "$TLA_JAR" tlc2.TLC -cleanup -workers auto \
        -config "${cfg##*/}" "${artifact##*/}" ) >"$log" 2>&1 || rc=$?
    if [ "$rc" -eq 124 ]; then
        record "$owner" "$artifact" "tla" "FAIL" "n/a" "$((SECONDS - started))"
        fail "TLC timed out after ${TLA_LIMIT}s: $log"
    fi
    if ! grep -q 'No error has been found' "$log"; then
        grep -E 'Error|violated|Exception' "$log" | head -10 >&2 || true
        record "$owner" "$artifact" "tla" "FAIL" "n/a" "$((SECONDS - started))"
        fail "TLA+ run failed: $log"
    fi
    if ! grep -q "TLC2 Version $tla_version" "$log"; then
        record "$owner" "$artifact" "tla" "FAIL" "n/a" "$((SECONDS - started))"
        fail "TLC version does not match manifest $tla_version: $log"
    fi
    states="$(sed -n 's/^\([0-9][0-9,]*\) states generated,.*/\1/p' "$log" | tr -d ',' | tail -1)"
    if [ -z "$states" ] || [ "$states" -le 0 ]; then
        record "$owner" "$artifact" "tla" "FAIL" "n/a" "$((SECONDS - started))"
        fail "TLC reported no generated states: $log"
    fi
    if [ "$states" -ne "$state_count" ]; then
        record "$owner" "$artifact" "tla" "FAIL" "$states" "$((SECONDS - started))"
        fail "states generated $states != manifest $state_count (update the manifest with the model change)"
    fi
    record "$owner" "$artifact" "tla" "PASS" "$states" "$((SECONDS - started))"
    TLA_ROWS=$((TLA_ROWS + 1))
    TLA_STATES_LAST="$states"
    TLA_VERSION_LAST="$tla_version"
}

while IFS=$'\t' read -r artifact property covers tool bounds unwind max_len negative \
        sync owner runtime property_count state_count jar_sha256 mode cfg function assertions; do
    case "$mode" in
        proof|cover|function)
            if [ "$CBMC_READY" -eq 0 ]; then
                record "$owner" "$artifact" "$mode" "SKIP" "n/a" "0"
                continue
            fi
            run_cbmc_row
            ;;
        tla)
            if [ "$TLA_READY" -eq 0 ]; then
                record "$owner" "$artifact" "$mode" "SKIP" "n/a" "0"
                continue
            fi
            run_tla_row
            ;;
    esac
done < "$ROWS_FILE"

elapsed=$((SECONDS - START))
[ "$elapsed" -le "$TOTAL_LIMIT" ] || fail "gate took ${elapsed}s > ${TOTAL_LIMIT}s"

if [ "$has_cbmc" -eq 0 ]; then
    cbmc_summary="no CBMC rows"
elif [ "$CBMC_READY" -eq 0 ]; then
    cbmc_summary="SKIP CBMC"
elif [ "$CBMC_AUX" -gt 0 ]; then
    cbmc_summary="CBMC $CBMC_VERSION, $CBMC_PROPERTY_SUM properties, $CBMC_AUX aux run(s)"
else
    cbmc_summary="CBMC $CBMC_VERSION, $CBMC_PROPERTY_SUM properties"
fi
if [ "$has_tla" -eq 0 ]; then
    tla_summary="no TLA+ rows"
elif [ "$TLA_READY" -eq 0 ]; then
    tla_summary="SKIP TLA+"
elif [ "$TLA_ROWS" -eq 1 ]; then
    tla_summary="TLA+ $TLA_VERSION_LAST, $TLA_STATES_LAST states"
else
    tla_summary="TLA+ $TLA_ROWS spec(s)"
fi
rows_total=$(wc -l < "$ROWS_FILE")
shards=$(cut -f10 "$ROWS_FILE" | sort -u | wc -l)
echo "✅ [FORMAL] $shards shard(s), $rows_total row(s), ${elapsed}s; $cbmc_summary; $tla_summary"

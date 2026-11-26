#!/usr/bin/env bash
# Manifest-declared negative checks. `<cfg>#<Invariant>` runs TLC against the
# row's TLA artifact; a .sh/.py path runs that script with ROOT_DIR,
# FORMAL_OUT_DIR (the shard directory), CBMC_BIN and ARTIFACT. Unknown forms
# fail manifest validation before anything runs. Temporary copies only; the
# repository tree is never edited.
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TLA_NEG_LIMIT=60
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
    echo "❌ [FORMAL-NEGATIVE] $*" >&2
    exit 1
}

# shellcheck source=ci/scripts/formal_tools.sh
. "$ROOT_DIR/ci/scripts/formal_tools.sh"

worktree_state() {
    git -C "$1" status --porcelain --untracked-files=normal
}

self_check_state_check() {
    local dir before after
    dir="$(mktemp -d)"
    git init -q "$dir"
    cp "$ROOT_DIR/.gitignore" "$dir/.gitignore"
    git -C "$dir" add .gitignore
    git -C "$dir" -c user.name=formal-negative -c user.email=formal-negative@example.invalid \
        -c commit.gpgsign=false commit --no-verify -qm self-check
    ln -s build-ci-clean "$dir/build"
    before="$(worktree_state "$dir")"
    if [ -n "$before" ]; then
        rm -rf "$dir"
        fail "self-check: gate build link is not ignored by .gitignore"
    fi
    rm "$dir/build"
    : > "$dir/untracked.txt"
    after="$(worktree_state "$dir")"
    if [ "$before" = "$after" ]; then
        rm -rf "$dir"
        fail "self-check: a real untracked file did not change the worktree state"
    fi
    rm -rf "$dir"
}
self_check_state_check
STATE_BEFORE="$(worktree_state "$ROOT_DIR")"

ROWS_FILE="$(mktemp)"
TMP_ROOT="$(mktemp -d)"
trap 'rm -rf "$TMP_ROOT" "$ROWS_FILE"' EXIT
if ! python3 "$ROOT_DIR/ci/scripts/formal_manifest.py" --root "$ROOT_DIR" --rows \
        > "$ROWS_FILE"; then
    fail "manifest parse failed"
fi
[ -s "$ROWS_FILE" ] || fail "no manifest rows"

has_cbmc_neg=0
has_tla_neg=0
while IFS=$'\t' read -r artifact property covers tool bounds unwind max_len negative \
        sync owner runtime property_count state_count jar_sha256 mode cfg function assertions; do
    if [ "$negative" = "n/a" ]; then
        continue
    fi
    IFS=';' read -ra entries <<< "$negative"
    for entry in "${entries[@]}"; do
        if [[ "$entry" == *#* ]]; then
            has_tla_neg=1
        else
            has_cbmc_neg=1
        fi
    done
done < "$ROWS_FILE"

CBMC_BIN=""
CBMC_READY=0
if [ "$has_cbmc_neg" -eq 1 ]; then
    CBMC_BIN="$(formal_resolve_cbmc)"
    if [ -z "$CBMC_BIN" ]; then
        if [ "$STRICT" -eq 1 ]; then
            fail "cbmc not found (--strict)"
        fi
        echo "SKIP CBMC (negative scripts; cbmc not found)"
    else
        version="$("$CBMC_BIN" --version | head -1)"
        [ "$version" = "$FORMAL_CBMC_VERSION" ] || \
            fail "CBMC version is '$version', pinned '$FORMAL_CBMC_VERSION'"
        cbmc_sha="$(formal_sha256 "$CBMC_BIN")"
        [ "$cbmc_sha" = "$FORMAL_CBMC_SHA256" ] || \
            fail "CBMC binary SHA256 $cbmc_sha != pinned $FORMAL_CBMC_SHA256"
        CBMC_READY=1
    fi
fi

JAVA_BIN=""
TLA_JAR=""
TLA_READY=0
if [ "$has_tla_neg" -eq 1 ]; then
    JAVA_BIN="$(formal_resolve_java)"
    TLA_JAR="$(formal_resolve_tla_jar)"
    if [ -z "$JAVA_BIN" ] || [ -z "$TLA_JAR" ] || [ ! -f "$TLA_JAR" ]; then
        if [ "$STRICT" -eq 1 ]; then
            fail "TLA+ tools missing: java or tla2tools.jar not found (--strict)"
        fi
        echo "SKIP TLA+ (negative; java or tla2tools.jar not found)"
    else
        java_major="$(formal_java_major "$JAVA_BIN")"
        [ "$java_major" = "$FORMAL_JAVA_MAJOR" ] || \
            fail "Java major is '${java_major:-unknown}', pinned $FORMAL_JAVA_MAJOR"
        TLA_READY=1
    fi
fi

SCRIPT_NEGATIVES=0
TLA_NEGATIVES=0
SKIPPED=0
TMP_SEQ=0

run_tla_negative() { # entry = <cfg>#<Invariant>
    local entry="$1" rel_cfg invariant cfg_stem dir log rc=0
    rel_cfg="${entry%%#*}"
    invariant="${entry#*#}"
    cfg_stem="${rel_cfg##*/}"
    cfg_stem="${cfg_stem%.*}"
    if [ "$TLA_READY" -eq 0 ]; then
        SKIPPED=$((SKIPPED + 1))
        echo "SKIP TLA+ negative $entry ($owner; tools missing)"
        return 0
    fi
    local jar_sha
    jar_sha="$(formal_sha256 "$TLA_JAR")"
    [ "$jar_sha" = "$jar_sha256" ] || \
        fail "TLA+ jar SHA256 $jar_sha != manifest $jar_sha256"
    TMP_SEQ=$((TMP_SEQ + 1))
    dir="$TMP_ROOT/neg$TMP_SEQ"
    mkdir -p "$dir"
    cp "$ROOT_DIR/$artifact" "$dir/"
    cp "$ROOT_DIR/$rel_cfg" "$dir/"
    log="$OUT_DIR/$owner/negative/$cfg_stem.$invariant.log"
    ( cd "$dir" && timeout "$TLA_NEG_LIMIT" "$JAVA_BIN" -XX:+UseParallelGC \
        -cp "$TLA_JAR" tlc2.TLC -cleanup -workers auto \
        -config "${rel_cfg##*/}" "${artifact##*/}" ) >"$log" 2>&1 || rc=$?
    [ "$rc" -ne 124 ] || fail "TLC timed out on $rel_cfg: $log"
    [ "$rc" -ne 0 ] || fail "TLC exited 0 on $rel_cfg; expected the $invariant violation: $log"
    grep -q "Invariant $invariant is violated" "$log" || \
        fail "expected '$invariant' violation not found in $rel_cfg run: $log"
    grep -q 'The behavior up to this point is' "$log" || \
        fail "no counterexample trace in $rel_cfg run: $log"
    TLA_NEGATIVES=$((TLA_NEGATIVES + 1))
}

run_script_negative() { # rel = script under ci/scripts/ or formal/
    local rel="$1" name log
    name="${rel##*/}"
    name="${name%.*}"
    log="$OUT_DIR/$owner/negative/$name.log"
    if [ "$CBMC_READY" -eq 0 ]; then
        SKIPPED=$((SKIPPED + 1))
        echo "SKIP mutation negative $rel ($owner; cbmc not found)"
        return 0
    fi
    if [[ "$rel" == *.py ]]; then
        ROOT_DIR="$ROOT_DIR" FORMAL_OUT_DIR="$OUT_DIR/$owner" CBMC_BIN="$CBMC_BIN" \
            ARTIFACT="$artifact" python3 "$ROOT_DIR/$rel" || \
            fail "negative script $rel failed (log: $log)"
    else
        ROOT_DIR="$ROOT_DIR" FORMAL_OUT_DIR="$OUT_DIR/$owner" CBMC_BIN="$CBMC_BIN" \
            ARTIFACT="$artifact" bash "$ROOT_DIR/$rel" || \
            fail "negative script $rel failed (log: $log)"
    fi
    [ -f "$log" ] || fail "negative script $rel did not write its log: $log"
    SCRIPT_NEGATIVES=$((SCRIPT_NEGATIVES + 1))
}

while IFS=$'\t' read -r artifact property covers tool bounds unwind max_len negative \
        sync owner runtime property_count state_count jar_sha256 mode cfg function assertions; do
    if [ "$negative" = "n/a" ]; then
        continue
    fi
    mkdir -p "$OUT_DIR/$owner/negative"
    IFS=';' read -ra entries <<< "$negative"
    for entry in "${entries[@]}"; do
        if [[ "$entry" == *#* ]]; then
            run_tla_negative "$entry"
        else
            run_script_negative "$entry"
        fi
    done
done < "$ROWS_FILE"

if [ "$(worktree_state "$ROOT_DIR")" != "$STATE_BEFORE" ]; then
    git -C "$ROOT_DIR" status --porcelain --untracked-files=normal >&2
    fail "the negative check changed the repository worktree"
fi

elapsed=$((SECONDS - START))
if [ "$SCRIPT_NEGATIVES" -eq 0 ] && [ "$TLA_NEGATIVES" -eq 0 ]; then
    echo "✅ [FORMAL-NEGATIVE] no negative checks ran (${SKIPPED} skipped), ${elapsed}s"
else
    echo "✅ [FORMAL-NEGATIVE] ${SCRIPT_NEGATIVES} mutation script(s) rejected, ${TLA_NEGATIVES} TLA+ invariant(s) violated with counterexamples, ${SKIPPED} skipped, ${elapsed}s"
fi

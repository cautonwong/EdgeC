#!/usr/bin/env bash
# Run a test binary twice concurrently. Fixed /tmp state paths make the two
# processes collide; per-process paths keep both green.
set -euo pipefail

bin="${1:?usage: run_concurrent_binary.sh <test-binary>}"
rounds="${2:-3}"
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

for round in $(seq 1 "$rounds"); do
    "$bin" >"$tmp/a.$round.log" 2>&1 & a=$!
    "$bin" >"$tmp/b.$round.log" 2>&1 & b=$!
    if ! wait "$a"; then
        echo "first process failed in round $round:"
        cat "$tmp/a.$round.log"
        exit 1
    fi
    if ! wait "$b"; then
        echo "second process failed in round $round:"
        cat "$tmp/b.$round.log"
        exit 1
    fi
done
echo "both processes passed $rounds concurrent round(s)"

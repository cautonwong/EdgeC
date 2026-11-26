#!/usr/bin/env bash
# Shared tool resolution and pins for the formal gates. Sourced by
# check_formal.sh and check_formal_negative.sh; never executed directly.
# A resolved tool is always re-hashed: PATH/env substitution fails the gate.

FORMAL_CBMC_VERSION="5.12 (cbmc-5.12)"
FORMAL_CBMC_SHA256="ef72f83869f9cc1af6e3e1721fff9ab0cb3cbc40a98105b793648605c111ee0d"
FORMAL_JAVA_MAJOR=17

formal_resolve_cbmc() {
    local bin="${CBMC:-}"
    if [ -z "$bin" ]; then
        bin="$(command -v cbmc 2>/dev/null || true)"
    fi
    if [ -z "$bin" ] && [ -f /opt/formal/env.sh ]; then
        # shellcheck disable=SC1091
        . /opt/formal/env.sh
        bin="$(command -v cbmc 2>/dev/null || true)"
    fi
    printf '%s' "$bin"
}

formal_resolve_java() {
    printf '%s' "${JAVA:-$(command -v java 2>/dev/null || true)}"
}

formal_resolve_tla_jar() {
    # The repository's vendored jar is the pinned artifact the manifest hashes;
    # a mutable PATH/env copy must not shadow it (a peer's /opt upgrade made the
    # gate fail on a jar the repo never pinned).
    if [ -f "$ROOT_DIR/formal/tools/tla2tools.jar" ]; then
        printf '%s' "$ROOT_DIR/formal/tools/tla2tools.jar"
        return
    fi
    local jar="${TLA2TOOLS:-}"
    if [ -z "$jar" ] && [ -f /opt/formal/env.sh ]; then
        # shellcheck disable=SC1091
        . /opt/formal/env.sh
        jar="${TLA2TOOLS:-}"
    fi
    printf '%s' "$jar"
}

formal_java_major() {
    "$1" -version 2>&1 | sed -n 's/.*version "\([0-9][0-9]*\).*/\1/p' | head -1
}

formal_sha256() {
    sha256sum "$1" | awk '{print $1}'
}

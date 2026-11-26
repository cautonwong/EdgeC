# Vendored formal tools

`tla2tools.jar` is the only binary checked in under `formal/`. It is a read-only
tool input to the formal gate and is not referenced by the product build graph
(`check_formal_isolation.py` enforces that).

| Field | Value |
| --- | --- |
| File | `formal/tools/tla2tools.jar` |
| Size | 4,510,884 bytes |
| SHA256 | `7e53aa636dac8a3cb0533f343115c0548e03fc0ce5edc7739becf58d3d0fb84f` |
| TLC version | `2026.11.15.002146` (banner: `TLC2 Version 2026.11.15.002146 (rev: bc1b506)`) |
| Build revision | `bc1b506` (from the TLC banner) |
| Build timestamp | `2026-11-15T00:21:46.50Z` (jar manifest `Build-TimeStamp`) |
| Source | TLA+ tools nightly, fetched to `/opt/formal/tla2tools.jar` on 2026-11-16 |
| License | MIT (TLA+ tools, see `tlapl.us`) |
| Java | OpenJDK 17 (`openjdk-17-jre-headless` on Ubuntu 22.04) |

## Why vendored

The CI `formal` job has no external formal tool cache. Downloading the nightly
jar by URL would add a network dependency and a mutable upstream; checking the
exact jar in lets CI verify the same SHA256 that `formal/manifest/*.tsv`
records. The job fails if the hash or the TLC version banner does not match.

## Resolution order

Both `ci/scripts/check_formal.sh` and `ci/scripts/check_formal_negative.sh`
resolve the jar through `ci/scripts/formal_tools.sh`:

1. `formal/tools/tla2tools.jar` in the repository (the pinned artifact the manifest hashes).
2. `TLA2TOOLS` from the caller environment, only when the vendored jar is absent.
3. `/opt/formal/env.sh` (local developer setup), only when neither is present.

The run then hashes the resolved jar and compares it with the manifest row's
`jar_sha256`. A mismatched hash or TLC version fails the gate regardless of
`--strict`; only a missing tool degrades to SKIP without `--strict`. The
vendored jar wins over `TLA2TOOLS` or `/opt` on purpose: a local tool upgrade
must not silently substitute a different build for the pinned one.

## Upgrading

1. Replace `formal/tools/tla2tools.jar` and update this file (size, SHA256,
   revision, timestamp, version).
2. Update every TLA row's `tool` and `jar_sha256` in `formal/manifest/*.tsv`.
3. Re-run `./ci/scripts/check_formal.sh --strict` and
   `./ci/scripts/check_formal_negative.sh --strict`; state counts and negative
   counterexamples usually move with a TLC upgrade.

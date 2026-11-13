# EdgeC app SDK v1

Minimal verifiable slice of T13. The WIT contract is the source of truth; every
language SDK is a thin codec over one socket framing, and all languages replay
the same conformance vectors.

## Status

- `sdk/wit/app.v1.wit` is frozen at `edgeos:app@1.0.0`.
- This slice has no app-host, no WIT compiler, and no generated bindings.
- `sdk/c/edge_app_sdk.h` and `sdk/c/edge_app_sdk.c` are hand-written. A
  generator replaces them later; until then `ci/scripts/check_sdk_wit.py`
  hashes the WIT file and fails when the hash embedded in the header as
  `EDGE_APP_WIT_SHA256` differs.
- Native sidecars (C, Python, Go) connect over an inherited socketpair. The JS
  adapter is intended for the in-process QuickJS host; its runner uses a Unix
  socket to exercise the same framing.
- No SDK includes `datahub.h` or touches shared memory.

## Contract

Host functions, WIT name to JSON `fn`:

| WIT | JSON | Result |
| --- | --- | --- |
| `point-resolve(name)` | `point_resolve` | `{handle, access}` |
| `point-read(handle)` | `point_read` | `{value, quality, source_ts}` |
| `point-subscribe(handle, deadband, min-interval-ms, max-interval-ms)` | `point_subscribe` | `{subscription}` |
| `point-publish(handle, sample)` | `point_publish` | `{}` |
| `log(level, message)` | `log` | `{}` |
| `now()` | `now` | `{now_ms}` |

Exports: `app_init`, `app_on_event(event)`, `app_stop`. The future app-host
decodes a host request and calls the SDK's `dispatch`; the SDK invokes the
callback registered for that export. This slice does not start a reader thread.

Error codes: `denied_not_granted`, `denied_scope`, `denied_expired`,
`denied_revoked`, `denied_policy`, `denied_quota`,
`denied_package_mismatch`, `denied_abi`, `invalid_manifest`, `not_found`,
`stale`, `busy`, `invalid`, `internal`. SDKs surface a typed error that keeps
the original code.

## Framing

One request and one response per message. Each message is a 4-byte big-endian
length prefix followed by that many bytes of UTF-8 JSON.

Request:

```json
{"id":1,"fn":"point_read","args":{"handle":7}}
```

Success response:

```json
{"id":1,"ok":true,"result":{"value":{"float":220.5},"quality":"good","source_ts":1720000000000}}
```

Error response:

```json
{"id":1,"ok":false,"code":"denied_scope"}
```

The response echoes the request `id`. Ids start at 1 and increase per client.
There is no separate error message field; `code` is the complete error.

Point values use a tagged object so the WIT variant survives JSON:

| WIT case | JSON |
| --- | --- |
| `float(f64)` | `{"float": 1.5}` |
| `integer(s64)` | `{"integer": 7}` |
| `boolean(bool)` | `{"boolean": true}` |
| `text(string)` | `{"text": "ready"}` |

A sample is `{"value": <tagged value>, "quality": "good|bad|uncertain", "source_ts": <ms>}`.
`source_ts: 0` means the app did not provide one.

Language mappings: Python maps `int` to integer and `float` to float; Go maps
`int`/`int64` to integer and `float64` to float; JS has one number type, so
integral numbers encode as integer and everything else as float. Use a
non-integral float when the wire must say `float`.

## Layout

- `sdk/wit/app.v1.wit` is the frozen contract.
- `sdk/c/`, `sdk/python/`, `sdk/go/`, `sdk/js/` are the four SDKs.
- `sdk/conformance/vectors.json` is the shared scenario set: input event, steps,
  expected results or error codes, and expected log lines.

## Conformance

Each runner starts a fake host, replays every vector's steps through the SDK,
and checks the request framing, the `fn` and `args` the host received, the value
or error the SDK returned, and the log lines.

```sh
./build/tests/test_sdk_conformance
python3 sdk/python/test_conformance.py
(cd sdk/go && go test -count=1 ./...)
node sdk/js/test_conformance.mjs
python3 ci/scripts/check_sdk_wit.py
```

Runners look for `sdk/conformance/vectors.json` under `""`, `"../"`, and
`"../../"` so they work from the repository root, the SDK directory, or the
build tree.

After editing the WIT file, update `EDGE_APP_WIT_SHA256` in
`sdk/c/edge_app_sdk.h`; the guard fails until the header and contract match.

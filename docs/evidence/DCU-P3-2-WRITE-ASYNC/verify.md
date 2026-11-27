# DCU-P3-2-WRITE-ASYNC verification

Branch `dcu/P3-2-write-async`, base `9d065b6d`, final tip `62c2430e` (product fixes `f1f5a36e` and `3819236e`; later commits are tests/evidence).

## Spec and independent derivation

Contract A in `modules/edge_io/include/io_executor.h`: the caller creates a `net_buf` (ref=1), calls `write_async`, and unrefs immediately after return regardless of rc. A return of 0 means an asynchronous implementation holds one ref until completion/failure/destroy; a synchronous-consume implementation (copy or ignore) holds none. A return <0 takes no ref. Destroy releases every pending ref.

Independent expected values, derived from refcount algebra rather than the implementation:

1. Async success while in flight: caller ref 1 + driver ref 1 = 2 refs out of the pool, so the pool free count is `base - 1`; after completion the driver releases its ref, so `base`.
2. Synchronous failure (rc < 0): no driver ref; the caller's unref returns the buffer, so `base`.
3. Passthrough caller: it owns one ref and must unref after `write_async`; with the driver ref held it leaves one ref out (`base - 1`), and the completion returns to `base`; on sync failure it leaves `base`. A caller that forgets its unref leaves one extra ref (`base - 2` on success, `base - 1` on sync failure), which is exactly the red state.
4. Destroy with a pending poll/HTTP write must release the driver ref; otherwise the pool free count stays below `base` after destroy.

## Independent red / green

- Red at `2d6f9034` (test-only, old implementation): `ctest -R '^(test_io_executor|test_passthrough)$'` exit 8, four failures: passthrough ownership `9 != 10` (success and sync-failure cases), UDP free count `4 != 3`, HTTP free count `4 != 3`.
- Green at `62c2430e`: both tests 2/2; `test_io_executor` 17/17; `ctest -R 'dcu|dlt698'` 45/45 (116.13s, verifier re-run).

## ASAN

At `62c2430e`, `ASAN_OPTIONS=detect_leaks=1`:
- `test_io_executor` 17/17 exit 0, no reports.
- `test_passthrough` 3/3 exit 0, no reports.

The first pass at `ad899e8f` leaked 704 B (the new HTTP test never closed its loop) and 43200 B (per-test orchestrator creation overwrote a global singleton storage). The fix is test-only: close the loop; share one orchestrator fixture across the passthrough tests.

## Acceptance

Three consecutive rounds at `62c2430e`: `全部合格:2 个验收任务`; `dcu_northbound` 35 passed in 17.49 / 27.58 / 18.19s; `dcu_acceptance` exit 0 each round.

## Mutation guard

`DCU-P3-2-write-async-ownership` (remove the passthrough caller unref): mutated build rc=0, test rc=8; restored original bytes/hash, build rc=0, test rc=0; runner 1/1 sensitive. Raw log in `verifier/mutation-guard.log`.

`DCU-P3-2-http-poll-after-cb` and `DCU-P3-2-http-added-already`: both mutated test rc=8 / restored rc=0, 1/1 sensitive each.

## Adversarial review

Two findings, both reproduced by the reviewer and re-run by the verifier against the fixed source.

- High: `_on_curl_poll` called the completion callback before releasing `pending_tx`. The callback can destroy the handle (`_on_http_simple_cb` in the public `io_executor_http_request` path), so `_http_destroy` freed `impl` and the completion path read it again (ASAN heap-use-after-free) and could double-unref. Reproduced: free after completion 3 instead of 4. Fixed by taking `pending_tx`, setting it NULL and removing the curl handle before the callback; after the callback only the saved local buffer is unref'd, and `impl`/`easy` are not touched.
- Medium: `_http_write_async` returned the positive `CURLM_ADDED_ALREADY` after releasing the ref. The caller unrefs on any rc, but libcurl still used the caller buffer, so a recycled buffer was sent. Reproduced with a local server receiving `EVIL`. Fixed by treating `CURLM_ADDED_ALREADY` as success (keep the ref, return 0) and returning negative codes for other errors after clearing POSTFIELDS.

Re-running the reviewer repro on the fixed source: s1 free after completion 4, ASAN no UAF; s5 rc 0 and the server received `PING`, not the recycled buffer. New tests `test_io_http_callback_destroy_keeps_pending_ref_count` and `test_io_http_write_after_enable_read_keeps_ref` pin both paths.

## Open items

- `bus_orchestrator_destroy` does not destroy the master engine/session manager; the shared test fixture only makes the objects reachable, it does not fix the product lifecycle. Pre-existing, suggested as a separate unit.
- `uv_poll_start` failure path and MQTT/OPCUA real endpoints have no runtime test; the MQTT path passes `buf->b.data` to Paho without a ref and relies on the library copying the payload. Unverified assumption, not a verified defect.

# D3-06 Class 8 parameter module grounding

Goal: implement the DCU Class 8 whitelist so a 698 master can SET a parameter and GET the same value back, with honest DARs for read-only or unimplemented cases. Worktree `/tmp/dcu-d3-06` on main `4233a7c0`. Plan exit evidence is `SET 4000 then read back`. Do not modify `cmd/dcu/dcu_north_adapter.c` or `cmd/dcu/dcu_698_class_acq.c`.

## Standard whitelist (7.3.8 / A.5)

Class 8 has attr 1 logical name and attr 2 instance-specific parameter; methods 1 reset and 2 execute exist but are not in scope.

| OI | Name | Attr 2 A-XDR |
| --- | --- | --- |
| 4000 | date/time | date_time_s (0x1C, 8 bytes) |
| 4001 | communication address | octet-string (0x09 + len) |
| 4002 | meter number | octet-string |
| 4003 | customer number | octet-string |
| 4004 | location | structure(lon{enum,unsigned,unsigned,unsigned}, lat{same}, dlu height_cm) |
| 401C | CT ratio | double-long-unsigned (0x06), no scale |
| 401D | PT ratio | double-long-unsigned, no scale |
| 401E | alarm amount limits | structure(2 x dlu), scale -2 |
| 401F | other amount limits | structure(3 x dlu, order 透支/囤积/合闸允许), scale -2 |
| 4020 | alarm energy limits | structure(2 x dlu), scale -2 |
| 4021 | other energy limits | structure(3 x dlu, order 囤积/透支/合闸允许), scale -2 |

`k_class_max_attr` says class 8 max attr 2, so only A2 is class-defined; attr 3/4 and methods classify DAR 4 today.

## Current 698 path facts

- Object table `g_objects[]` in `cmd/dcu/dcu_698_object_model.c` has 17 rows; no 4000-range rows. Each row holds `class_id`, `scope`, read/write/method handlers, borrowed `instance`, 256-bit masks.
- GET: driver ACL check, then `dcu_north_resolve_axdr` in `cmd/dcu/dcu_698_axdr.c`. It special-cases 0x44000200, variable OIs, archive OIs 6000/6001, then `dcu_698_value_for_oad` scalar table. `_encode_axdr_value` supports integers/bool/enum/float32 only.
- SET: driver ACL check, then `dcu_north_accept_axdr`. It checks the object mask, requires a non-NULL write handler, then `_write_kind_for` (only event A11/A12), decodes one scalar and calls `dcu_698_write`.
- DAR mapping: ACL deny 3, class-defined-not-masked 5, class-undefined 4, type mismatch 7, missing OI 6.
- Codec has `dlt698_encode_date_time_s` and `dlt698_decode_date_time_s`; no octet-string helper, but tag 0x09 + len + bytes is trivial.

## ACL and session capacity

- Class 20 ACL in `cmd/dcu/dcu_698_appconn.c` lists 16 objects today plus class 20 itself. Adding 11 OIs reaches 28.
- `dlt698_session_t` lives in a 1024-byte per-driver priv (`DRIVER_PRIV_SIZE` in `adapters/io_drivers/src/driver_manager.c`). ACL entry is currently 44 bytes at caps 20/12/8. 28 x 44 = 1232, overflow. The manager storage is a fixed 64 KiB struct also used on unit-test stacks.
- Compact caps that fit: objects 28, attrs 10, methods 5 gives 34 bytes/object, 28 x 34 = 952 plus session fields, under 1024. A `_Static_assert(sizeof(dlt698_session_t) <= 1024)` should guard it.

## Storage

- No parameter store exists. `dcu_storage` owns SQLite (`dcu_storage_t`) with tables `dcu_meters` (dead), `dcu_telemetry_latest`, `dcu_freeze_history`, `telemetry`, `event_history`. Archive persistence goes through `dcu_inventory_save_meter` on `storage->db`.
- Design doc maps Class 8 to a `dcu_params` table with write audit, but no schema exists. Plan exit only needs in-process SET then read-back.
- No time service; 4000 needs a value source (stored parameter vs OS clock).
- 401C/401D terminal-level ratios are distinct from per-meter `meter_inventory.pt_ratio/ct_ratio`; the whitelist is terminal-level singletons.

## Constraints

- All code is C11, no dynamic memory after init, no new public (installed) API.
- New file `cmd/dcu/dcu_698_param.c/.h`; object table rows and ACL entries are in scope. `dcu_698_axdr.c`, `dcu_698_appconn.c`, `driver_dlt698.c`, `dcu_storage.*`, `dcu_node.*` may change.
- Tests: read-back per parameter, DAR 3 for unauthorized/read-only, DAR 7 wrong type, DAR 5/4 unknown attr. Acceptance: SET one parameter through the slave and read back; two tasks green.
- Write audit is required by the design doc; if deferred, say why and where it will land.

## Rubric

1. Honest DAR matrix for every whitelisted OI (GET/SET success, read-only, unknown attr, wrong type).
2. Small public surface; wire shapes parsed behind the module boundary.
3. Fits the 1024-byte session and 64 KiB driver manager storage with a compile-time guard.
4. Clear persistence story: what survives restart, what does not, where the audit goes.
5. Testable with the existing object model/unit-test patterns and the acceptance slave.
6. Diff stays inside the allowed files and does not regress existing OIs.

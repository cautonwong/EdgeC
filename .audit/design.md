# DCU-D3-06 synthesized design (arena base: candidate 1)

## Problem

Class 8 parameter OIs have no object rows and the generic GET/SET path is scalar-only. The 11 whitelist OIs need typed A-XDR round-trips, honest DARs, and storage in a 1024-byte driver session that already holds a 44-byte-per-object ACL. There is no parameter store and no time service.

## Usage

```c
/* GET branch in dcu_north_resolve_axdr, after the archive branch */
if (dcu_698_param_is_oi(oi))
    return dcu_698_param_encode_axdr(oad, buf, cap, len);

/* SET branch in dcu_north_accept_axdr, after the mask check and before obj->write */
if (dcu_698_param_is_oi((uint16_t)(oad >> 16)))
    return dcu_698_param_accept_axdr(oad, axdr, len);

/* dcu_node_create_in, after dcu_router_init_ex */
static int _node_param_save(void *ctx, uint16_t oi, const uint8_t *b, size_t n) {
    return dcu_storage_param_save((dcu_storage_t *)ctx, oi, b, n, "dlt698");
}
static int _node_param_load(void *ctx, uint16_t oi, uint8_t *b, size_t cap, size_t *n) {
    return dcu_storage_param_load((dcu_storage_t *)ctx, oi, b, cap, n);
}
static const dcu_param_store_t store = {.ctx = &node->storage,
                                        .save = _node_param_save,
                                        .load = _node_param_load};
dcu_698_param_init(&store);
```

## Shape

### Whitelist, one X-macro list

`cmd/dcu/dcu_698_param.h`:

```c
#define DCU_698_PARAM_OIS(X) \
    X(0x4000) X(0x4001) X(0x4002) X(0x4003) X(0x4004) X(0x401C) \
    X(0x401D) X(0x401E) X(0x401F) X(0x4020) X(0x4021)
```

The OI list expands into the object-table rows and the Class 20 ACL rows. The descriptor table below adds the per-attribute rows, including 4000 A3 (clock mode) and A4 (precise-time parameters).

```c
typedef enum { P_DATETIME_S, P_ENUM, P_UNSIGNED5, P_OCTETS, P_GEO, P_DLU, P_DLU2, P_DLU3 } dcu_param_kind_t;

typedef struct { uint16_t year; uint8_t month, day, hour, min, sec; } dcu_param_when_t;
typedef struct { uint8_t dir, deg, min, sec; } dcu_param_coord_t;
typedef struct { dcu_param_coord_t lon, lat; uint32_t height_cm; } dcu_param_geo_t;
typedef struct {
    uint16_t oi;
    uint8_t attr;
    dcu_param_kind_t kind;
    uint8_t count;   /* octets max length, or structure field count */
    union {
        dcu_param_when_t when;
        uint8_t enum8;          /* 4000 A3 clock mode */
        uint16_t u16[5];        /* 4000 A4 precise-time parameters */
        struct { uint8_t len; uint8_t bytes[31]; } octets;
        dcu_param_geo_t geo;
        uint32_t dlu[3];
    } v;
} dcu_param_t;
```

Descriptor rows: 4000 A2 date_time_s, 4000 A3 enum, 4000 A4 structure of 5 unsigned, 4001/2/3 A2 octets (31), 4004 A2 geo, 401C/401D A2 dlu, 401E A2 dlu2, 401F A2 dlu3, 4020 A2 dlu2, 4021 A2 dlu3.

4000's object row keeps `attr_mask = bit 2` because the generated catalog caps Class 8 at attr 2; A3/A4 are OI-specific in A.5 and are served by the param module, which runs before the generic mask check. Its ACL entry lists attrs 2, 3, 4 with rights 3.

```c
typedef struct {
    void *ctx;
    int (*save)(void *ctx, uint16_t oi, const uint8_t *buf, size_t len);
    int (*load)(void *ctx, uint16_t oi, uint8_t *buf, size_t cap, size_t *len);
} dcu_param_store_t;

void dcu_698_param_init(const dcu_param_store_t *store);
void dcu_698_param_reset(void);
bool dcu_698_param_is_oi(uint16_t oi);
bool dcu_698_param_has_attr(uint16_t oi, uint8_t attr);
int dcu_698_param_encode_axdr(uint32_t oad, uint8_t *buf, size_t cap, size_t *len);
int dcu_698_param_accept_axdr(uint32_t oad, const uint8_t *axdr, size_t len);
```

`dcu_param_t g_params[11]` is file-static, loop-thread only, no lock, no heap. `init` copies the port and loads persisted rows through the internal decoder. `reset` restores defaults (4000 = OS local time, octets empty, geo zero, ratios 1, limits 0), used by object-model reset for test isolation.

### Wire shapes

| kind | GET bytes |
| --- | --- |
| P_DATETIME_S | `dlt698_encode_date_time_s` (0x1C + 7 bytes) |
| P_ENUM | `0x16 v` (4000 A3: 0/1/2/255) |
| P_UNSIGNED5 | `0x02 0x05` then five `0x11 v` (4000 A4) |
| P_OCTETS | `0x09 len bytes` |
| P_GEO | `0x02 0x03` lon `0x02 0x04 {0x16 dir, 0x11 deg, 0x11 min, 0x11 sec}`, lat same, `0x06 height_be32` |
| P_DLU | `0x06 be32` |
| P_DLU2/3 | `0x02 count` then `count x (0x06 be32)` |

SET decodes with `dlt698_decode_data` for structure tags and `dlt698_decode_date_time_s` for 4000, requires `off == len`, validates ranges (month 1..12, day 1..31, hour < 24, min/sec < 60, dir 0/1, deg ≤ 180/90, octet len ≤ 31, exact structure counts), and never partially mutates `g_params`.

Commit order. decode into `next` → encode `next` to a 64-byte blob → `store->save(oi, blob)` → assign `g_params[i] = next`. Save failure returns `EDGE_SLAVE_SRC_TEMPORARY`; RAM unchanged. Rejected SETs are logged with their `EDGE_SLAVE_SRC_*` code but never persisted or audited, so a hostile master cannot fill flash. 4000 is not persisted: GET after restart reflects the OS clock.

### Object table and ACL

11 rows in `dcu_698_object_model.c`: `class_id = IC_PARAM`, `scope = DCU_OBJ_SINGLETON`, `attr_mask = bit 2`, no read/write/method handlers. GET/SET reach the param module through the two A-XDR branches, not `dcu_698_read/write`.

11 ACL rows in `dcu_698_appconn.c`: one attr `{2, 3}` each. Object count 17 → 28.

### Driver ACL capacity

Replace the per-object inline arrays with:

```c
typedef struct {
    uint16_t oi;
    uint8_t n_attrs;
    uint8_t n_methods;
    uint32_t attr_rights;   /* 2 bits per attr id 1..16 */
    uint8_t method_ids[8];
} dlt698_acl_object_t;      /* 16 B */

#define DLT698_ACL_MAX_OBJECTS 28
#define DLT698_ACL_MAX_ATTRS 16
#define DLT698_ACL_MAX_METHODS 8
_Static_assert(sizeof(dlt698_session_t) <= 1024, "DLT698 session exceeds DRIVER_PRIV_SIZE");
```

28 x 16 = 448 B plus session fields, safely under 1024. `_acl_parse` rejects attr id 0 or > 16 and method count > 8. `_acl_attr_allowed` reads the 2-bit field; `_acl_method_allowed` scans `method_ids`. The 8-method cap keeps OI 6000 (`127..134`) parseable; the attr cap keeps event's 10 attrs.

### Persistence and audit

`dcu_storage_init` adds:

```sql
CREATE TABLE IF NOT EXISTS dcu_params (
  oi INTEGER PRIMARY KEY, value BLOB NOT NULL, updated_at INTEGER NOT NULL);
CREATE TABLE IF NOT EXISTS dcu_param_audit (
  id INTEGER PRIMARY KEY AUTOINCREMENT, oi INTEGER NOT NULL, changed_at INTEGER NOT NULL,
  source TEXT NOT NULL, old_value BLOB, new_value BLOB);
```

`dcu_storage_param_save` runs one `BEGIN IMMEDIATE`: read the old blob, upsert `dcu_params`, insert the audit row, commit; rollback on any failure. `dcu_storage_param_load` returns 0 with the row, 1 when absent, negative on error. Source is `"dlt698"`; per-master attribution needs a port change and stays open.

### DAR matrix

"Module" rows are reachable through a direct `dcu_north_accept_axdr` / `dcu_north_resolve_axdr` call; on the wire the driver ACL runs first and returns DAR 3 for any attr not listed (the ACL lists only attr 2 for each Class 8 OI).

| Case | GET | SET |
| --- | --- | --- |
| Whitelisted OI attr 2, valid | 0 typed A-XDR | 0, persisted + audited |
| attr 1 (class-defined, not masked) | 5 module / 3 wire | same |
| attr 3+ (class max attr 2) | 4 module / 3 wire | same |
| wrong tag/arity/range/extra bytes | n/a | 7, state unchanged |
| ACL rights deny | 3 | 3 |
| OI outside table (4005, 4019) | 6 module / 3 wire | same |
| SQLite save/audit failure | n/a | 2, RAM unchanged |
| 4000 SET | n/a | 0 (RAM; OS clock untouched) |

4000 attr 3 and 4 are implemented by the param module. Method 127 (broadcast time sync) stays masked: the driver routes every ACTION through the product `accept_action` callback in `dcu_north_adapter.c`, which D1-20a owns, so wiring it from this ticket would touch a forbidden file. Report the dependency instead of a fake handler.

### Module map

| File | Change |
| --- | --- |
| `cmd/dcu/dcu_698_param.h/.c` | new module: list, descriptors, codecs, validation, store port |
| `cmd/dcu/dcu_698_axdr.c` | GET and SET branches |
| `cmd/dcu/dcu_698_object_model.c` | 11 rows; call `dcu_698_param_reset()` in reset |
| `cmd/dcu/dcu_698_appconn.c` | 11 ACL rows, count 28 |
| `adapters/io_drivers/src/driver_dlt698.c` | compact ACL struct, caps, static assert |
| `cmd/dcu/dcu_storage.h/.c` | params + audit schema, load/save |
| `cmd/dcu/dcu_node.c` | store adapter + init |
| `cmd/dcu/CMakeLists.txt` | add source |
| `tests/test_dcu_698_param.c` (new), object-model/appconn/axdr/driver tests | coverage |

### Tests

1. Round-trip all 11 OIs: canonical A-XDR -> `dcu_north_accept_axdr` 0 -> `dcu_north_resolve_axdr` 0, byte-identical; geo nesting, 401F/4021 order, octet 0/31/32.
2. DAR: attr 1 -> 5, attr 3 -> 4, unknown OI -> 6, wrong shape/range -> 7 with the old value intact, direct accept with a save-failing fake store -> 2.
3. ACL: parse the real `dcu_698_appconn_object_list` through the driver parser and assert 28 objects / 59 attrs / 13 methods; a rights=1 object blocks SET with DAR 3 (driver test); 29 objects are rejected at runtime (deny-all, no truncation); `sizeof(session) <= 1024` compiled.
4. Persistence: SQLite save then reset+init reloads the same bytes; audit row has old and new; 4000 never calls save.
5. Acceptance: slave SET 4000 date/time, GET returns the same bytes; both tasks green.

## Decisions that reject candidate alternatives

- Candidate 3's 4000 A3 rejected: catalog class max attr is 2; A3 would fail `test_masks_stay_within_the_class_definition`. Defer to D4-04.
- Candidate 3's read-only ratios rejected: the design doc calls ratios field-configurable; the store gives them one truth.
- Candidate 4's geo encoding (`height long-unsigned`, `sec_x100`) rejected: A.5 says unsigned degrees/min/sec and double-long-unsigned height.
- Candidate 4's deferred persistence rejected: the ticket says SET persists; `dcu_params` lands now.
- Candidate 3's ratio-not-zero rejection rejected: the standard sets no such constraint.
- Candidate 1's per-OI object rows and typed-codec module kept; candidate 4's X-macro grafted; candidate 3's typed-struct-as-truth and pure validation kept.

## Open questions

- Does 4000 need to set the terminal clock? This design stores a RAM shadow only.
- Per-master audit attribution needs `accept_axdr` to carry a client identity.
- Octet-string length limits (31) are a product sentinel; per-OI limits may differ.

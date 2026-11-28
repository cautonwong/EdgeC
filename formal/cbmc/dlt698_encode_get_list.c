/*
 * The production codec and the net_buf writer helpers are included directly, so
 * the proof runs against the shipped write path. NDEBUG models the release
 * build: net_buf_simple_add's assert is not the defense, `_fits` is (see the
 * comment on `_fits`). Removing `_fits` is therefore a real out-of-bounds write.
 *
 * Output storage is a fixed FORMAL_MAX_LEN array. Usable capacity `cap` is
 * symbolic and the buffer's __buf/data are trailing-aligned at
 * FORMAL_MAX_LEN - cap, so tailroom == cap and a write past cap leaves the
 * storage object (no VLA; the negative mutation is observable).
 */
#define NDEBUG 1
#ifndef DLT698_CODEC_SOURCE
#define DLT698_CODEC_SOURCE "../../foundation/src/protocol/dlt698_codec.c"
#endif
#include DLT698_CODEC_SOURCE
#include "../../foundation/src/net_buf.c"

#ifndef FORMAL_MAX_LEN
#define FORMAL_MAX_LEN 24
#endif

static void _bind_out(net_buf_t *out, uint8_t *storage, uint16_t cap) {
    out->b.__buf = storage + (FORMAL_MAX_LEN - cap);
    out->b.data = out->b.__buf;
    out->b.len = 0;
    out->b.size = cap;
}

int main(void) {
    uint8_t storage[FORMAL_MAX_LEN];
    net_buf_t out = { 0 };
    uint16_t cap;
    __CPROVER_assume(cap <= FORMAL_MAX_LEN);
    _bind_out(&out, storage, cap);

    /* CBMC 5.12 reads a `__CPROVER_assume` placed directly on a struct field as
     * independent of later reads, which silently widens the domain. Constrain
     * locals first and store them, so the proof ranges are sound. */
    dlt698_addr_t sa = { 0 };
    uint8_t sa_len;
    __CPROVER_assume(sa_len >= 1 && sa_len <= sizeof(sa.addr));
    sa.len = sa_len;

    dlt698_get_item_t items[DLT698_GET_LIST_MAX];
    uint8_t data_pool[DLT698_GET_LIST_MAX][8] = { { 0 } };
    for (size_t i = 0; i < DLT698_GET_LIST_MAX; i++) {
        size_t dlen;
        __CPROVER_assume(dlen <= 8);
        items[i].dlen = dlen;
        items[i].data = data_pool[i];
    }
    size_t n;
    uint8_t piid = 0;

    /* Independently recompute the capacity demand from the documented frame:
     * 6 APDU fixed bytes plus 5 OAD/tag bytes per item, plus the frame overhead
     * (11 + address length). The assert below then requires the encoder to
     * accept exactly when this demand fits. */
    size_t need = 6u;
    for (size_t i = 0; i < DLT698_GET_LIST_MAX; i++) {
        if (i < n) {
            need += 5u + (items[i].dar != 0 ? 1u : items[i].dlen);
        }
    }
    bool too_many = n > DLT698_GET_LIST_MAX;
    bool insufficient =
        need + 11u + sa.len > net_buf_simple_tailroom(&out.b);

    int rc = dlt698_encode_get_response_list(&sa, piid, items, n, &out);

    __CPROVER_assert(rc == 0 || rc == -1,
                     "dlt698_get_list: return code in contract");
    __CPROVER_assert((rc == -1) == (too_many || insufficient),
                     "dlt698_get_list: -1 iff too many items or insufficient capacity");
    if (rc == 0) {
        size_t used = (size_t)(out.b.data - out.b.__buf) + out.b.len;
        __CPROVER_assert(out.b.len <= cap,
                         "dlt698_get_list: encoded length within cap");
        __CPROVER_assert(used <= FORMAL_MAX_LEN,
                         "dlt698_get_list: writes within storage");
    }
    __CPROVER_cover(rc == 0);
    return 0;
}

/* Every case returns before the frame is sealed, so this entry needs no loop
 * unwind in function mode. */
int get_list_reject(void) {
    uint8_t storage[64];
    net_buf_t out = { 0 };
    out.b.__buf = storage;
    out.b.data = storage;
    out.b.len = 0;
    out.b.size = sizeof(storage);

    dlt698_addr_t sa = { 0 };
    sa.len = 1;
    sa.addr[0] = 0x01;
    uint8_t payload[] = { DLT698_TAG_UNSIGNED };
    dlt698_get_item_t item = {
        .oad = 0x00100200u,
        .dar = 0,
        .data = payload,
        .dlen = sizeof(payload),
    };

    int null_sa_rc = dlt698_encode_get_response_list(NULL, 0x01, &item, 1, &out);
    __CPROVER_assert(null_sa_rc == -1, "dlt698_get_list: null sa rejected");

    int null_out_rc = dlt698_encode_get_response_list(&sa, 0x01, &item, 1, NULL);
    __CPROVER_assert(null_out_rc == -1, "dlt698_get_list: null out rejected");

    int null_items_rc = dlt698_encode_get_response_list(&sa, 0x01, NULL, 1, &out);
    __CPROVER_assert(null_items_rc == -1,
                     "dlt698_get_list: null items with n>0 rejected");

    dlt698_get_item_t missing = {
        .oad = 0x00100200u,
        .dar = 0,
        .data = NULL,
        .dlen = 1,
    };
    int missing_rc = dlt698_encode_get_response_list(&sa, 0x01, &missing, 1, &out);
    __CPROVER_assert(missing_rc == -1, "dlt698_get_list: item without data rejected");

    dlt698_get_item_t many[DLT698_GET_LIST_MAX + 1] = { 0 };
    int many_rc = dlt698_encode_get_response_list(&sa, 0x01, many,
                                                  DLT698_GET_LIST_MAX + 1, &out);
    __CPROVER_assert(many_rc == -1, "dlt698_get_list: too many items rejected");

    net_buf_t small = { 0 };
    small.b.__buf = storage;
    small.b.data = storage;
    small.b.len = 0;
    small.b.size = 10;
    int small_rc = dlt698_encode_get_response_list(&sa, 0x01, &item, 1, &small);
    __CPROVER_assert(small_rc == -1, "dlt698_get_list: undersized cap rejected");
    return 0;
}

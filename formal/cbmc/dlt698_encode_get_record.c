/*
 * Bounded CBMC 5.12 proof for streaming GET-Response-Record encoders:
 * dlt698_encode_get_record_response_begin, dlt698_encode_get_record_row,
 * dlt698_encode_get_record_response_end, and dlt698_encode_get_record_response_dar.
 *
 * Output storage is a fixed FORMAL_MAX_LEN array. Usable capacity `cap` is
 * symbolic and the buffer's __buf/data are trailing-aligned at
 * FORMAL_MAX_LEN - cap, so tailroom == cap and any write past cap leaves the
 * storage object (no VLA; negative mutations are observable).
 */
#define NDEBUG 1
#ifndef DLT698_CODEC_SOURCE
#define DLT698_CODEC_SOURCE "../../foundation/src/protocol/dlt698_codec.c"
#endif
#include DLT698_CODEC_SOURCE
#include "../../foundation/src/net_buf.c"

#ifndef FORMAL_MAX_LEN
#define FORMAL_MAX_LEN 26
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

    /* Constrain locals first and store into structs for CBMC 5.12 soundness. */
    dlt698_addr_t sa = { 0 };
    uint8_t sa_len;
    __CPROVER_assume(sa_len >= 1 && sa_len <= sizeof(sa.addr));
    sa.len = sa_len;

    size_t record_count;
    uint8_t piid = 0;
    uint32_t oad = 0;
    bool do_dar;

    /* Independently recompute prefix length (n=0 in main so _add_rcsd's nested
     * loop does not exceed CBMC's default 256-object limit at unwind=27;
     * non-empty RCSD OAD/ROAD paths are verified in get_record_reject). */
    size_t need_prefix = 16u + (size_t)sa.len;

    if (!do_dar) {
        dlt698_record_stream_t stream = { 0 };
        bool bad_rc = record_count > 0xFFu;
        bool begin_short = !bad_rc && (need_prefix + 2u > net_buf_simple_tailroom(&out.b));

        int rc_begin = dlt698_encode_get_record_response_begin(&stream, &sa, piid, oad, NULL, 0,
                                                               record_count, &out);
        __CPROVER_assert(rc_begin == 0 || rc_begin == -1,
                         "dlt698_get_record_encode: begin return code in contract");
        __CPROVER_assert((rc_begin == -1) == (bad_rc || begin_short),
                         "dlt698_get_record_encode: begin -1 iff invalid or short");

        if (rc_begin == 0) {
            size_t used_begin = (size_t)(out.b.data - out.b.__buf) + out.b.len;
            __CPROVER_assert(out.b.len <= cap,
                             "dlt698_get_record_encode: begin length within cap");
            __CPROVER_assert(used_begin <= FORMAL_MAX_LEN,
                             "dlt698_get_record_encode: begin writes within storage");

            uint8_t row_buf[FORMAL_MAX_LEN] = { 0 };
            size_t row_len;
            __CPROVER_assume(row_len <= FORMAL_MAX_LEN);
            size_t tail_before_row = net_buf_simple_tailroom(&out.b);
            bool row_short = (row_len == 0) || (tail_before_row < 5u) ||
                             (row_len > tail_before_row - 5u);

            int rc_row = dlt698_encode_get_record_row(&out, row_buf, row_len);
            __CPROVER_assert(rc_row == 0 || rc_row == -1,
                             "dlt698_get_record_encode: row return code in contract");
            __CPROVER_assert((rc_row == -1) == row_short,
                             "dlt698_get_record_encode: row -1 iff zero or short");

            if (rc_row == 0) {
                size_t used_row = (size_t)(out.b.data - out.b.__buf) + out.b.len;
                __CPROVER_assert(out.b.len <= cap,
                                 "dlt698_get_record_encode: row length within cap");
                __CPROVER_assert(used_row <= FORMAL_MAX_LEN,
                                 "dlt698_get_record_encode: row writes within storage");

                int rc_end = dlt698_encode_get_record_response_end(&stream, &out);
                __CPROVER_assert(rc_end == 0,
                                 "dlt698_get_record_encode: end succeeds after row");
                size_t used_end = (size_t)(out.b.data - out.b.__buf) + out.b.len;
                __CPROVER_assert(out.b.len <= cap,
                                 "dlt698_get_record_encode: end length within cap");
                __CPROVER_assert(used_end <= FORMAL_MAX_LEN,
                                 "dlt698_get_record_encode: end writes within storage");
                __CPROVER_cover(rc_end == 0);
            }
        }
    } else {
        uint8_t dar = 0x03;
        bool dar_short = need_prefix + 7u > net_buf_simple_tailroom(&out.b);

        int rc_dar = dlt698_encode_get_record_response_dar(&sa, piid, oad, NULL, 0, dar, &out);
        __CPROVER_assert(rc_dar == 0 || rc_dar == -1,
                         "dlt698_get_record_encode: dar return code in contract");
        __CPROVER_assert((rc_dar == -1) == dar_short,
                         "dlt698_get_record_encode: dar -1 iff short");
        if (rc_dar == 0) {
            size_t used_dar = (size_t)(out.b.data - out.b.__buf) + out.b.len;
            __CPROVER_assert(out.b.len <= cap,
                             "dlt698_get_record_encode: dar length within cap");
            __CPROVER_assert(used_dar <= FORMAL_MAX_LEN,
                             "dlt698_get_record_encode: dar writes within storage");
        }
    }
    return 0;
}

int get_record_reject(void) {
    uint8_t storage[64];
    net_buf_t out = { 0 };
    out.b.__buf = storage;
    out.b.data = storage;
    out.b.len = 0;
    out.b.size = sizeof(storage);

    dlt698_record_stream_t stream = { 0 };
    dlt698_addr_t sa = { 0 };
    sa.len = 1;
    sa.addr[0] = 0x01;
    dlt698_csd_t csd = { .kind = DLT698_CSD_OAD, .oad = 0x00100200u };
    uint8_t row[] = { DLT698_TAG_UNSIGNED, 0x01 };

    __CPROVER_assert(
        dlt698_encode_get_record_response_begin(NULL, &sa, 0x01, 0x50040200u, &csd, 1, 1, &out) == -1,
        "dlt698_get_record_encode: null stream rejected");
    __CPROVER_assert(
        dlt698_encode_get_record_response_begin(&stream, NULL, 0x01, 0x50040200u, &csd, 1, 1, &out) == -1,
        "dlt698_get_record_encode: null sa rejected");
    __CPROVER_assert(
        dlt698_encode_get_record_response_begin(&stream, &sa, 0x01, 0x50040200u, &csd, 1, 1, NULL) == -1,
        "dlt698_get_record_encode: null out rejected");
    __CPROVER_assert(
        dlt698_encode_get_record_response_begin(&stream, &sa, 0x01, 0x50040200u, NULL, 1, 1, &out) == -1,
        "dlt698_get_record_encode: null rcsd with n>0 rejected");
    __CPROVER_assert(
        dlt698_encode_get_record_response_begin(&stream, &sa, 0x01, 0x50040200u, &csd,
                                                DLT698_RCSD_MAX + 1, 1, &out) == -1,
        "dlt698_get_record_encode: n over max rejected");
    __CPROVER_assert(
        dlt698_encode_get_record_response_begin(&stream, &sa, 0x01, 0x50040200u, &csd, 1, 0x100u, &out) == -1,
        "dlt698_get_record_encode: record_count over 0xFF rejected");

    __CPROVER_assert(dlt698_encode_get_record_row(NULL, row, sizeof(row)) == -1,
                     "dlt698_get_record_encode: row null out rejected");
    __CPROVER_assert(dlt698_encode_get_record_row(&out, NULL, 1) == -1,
                     "dlt698_get_record_encode: row null data rejected");
    __CPROVER_assert(dlt698_encode_get_record_row(&out, row, 0) == -1,
                     "dlt698_get_record_encode: zero row_len rejected");

    __CPROVER_assert(dlt698_encode_get_record_response_end(NULL, &out) == -1,
                     "dlt698_get_record_encode: end null stream rejected");
    __CPROVER_assert(dlt698_encode_get_record_response_end(&stream, NULL) == -1,
                     "dlt698_get_record_encode: end null out rejected");

    __CPROVER_assert(
        dlt698_encode_get_record_response_dar(NULL, 0x01, 0x50040200u, &csd, 1, 0x03, &out) == -1,
        "dlt698_get_record_encode: dar null sa rejected");
    __CPROVER_assert(
        dlt698_encode_get_record_response_dar(&sa, 0x01, 0x50040200u, NULL, 1, 0x03, &out) == -1,
        "dlt698_get_record_encode: dar null rcsd with n>0 rejected");

    net_buf_t small = { 0 };
    small.b.__buf = storage;
    small.b.data = storage;
    small.b.len = 0;
    small.b.size = 10;
    __CPROVER_assert(
        dlt698_encode_get_record_response_begin(&stream, &sa, 0x01, 0x50040200u, &csd, 1, 1, &small) == -1,
        "dlt698_get_record_encode: begin undersized cap rejected");
    net_buf_t small_end = { 0 };
    small_end.b.__buf = storage;
    small_end.b.data = storage;
    small_end.b.len = 0;
    small_end.b.size = 4;
    __CPROVER_assert(dlt698_encode_get_record_row(&small_end, row, sizeof(row)) == -1,
                     "dlt698_get_record_encode: row undersized cap rejected");
    __CPROVER_assert(dlt698_encode_get_record_response_end(&stream, &small_end) == -1,
                     "dlt698_get_record_encode: end undersized cap rejected");
    __CPROVER_assert(
        dlt698_encode_get_record_response_dar(&sa, 0x01, 0x50040200u, &csd, 1, 0x03, &small) == -1,
        "dlt698_get_record_encode: dar undersized cap rejected");
    return 0;
}

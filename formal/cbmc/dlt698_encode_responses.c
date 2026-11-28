/*
 * Bounded CBMC 5.12 proof for the remaining four DL/T 698.45 response encoders:
 *   - dlt698_encode_set_response
 *   - dlt698_encode_action_response
 *   - dlt698_encode_connect_response
 *   - dlt698_encode_release_response
 *
 * Output storage is a fixed FORMAL_MAX_LEN array with trailing-aligned __buf
 * so tailroom == cap and any write past cap leaves the storage object.
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

    /* Constrain locals before storing into struct fields (CBMC 5.12 soundness). */
    dlt698_addr_t sa = { 0 };
    uint8_t sa_len;
    __CPROVER_assume(sa_len >= 1 && sa_len <= sizeof(sa.addr));
    sa.len = sa_len;

    uint8_t which;
    __CPROVER_assume(which < 4);

    uint8_t piid = 0;
    uint32_t oad = 0x40010200u;
    uint16_t oi = 0x4000u;
    uint8_t dar = 0;
    uint8_t method;
    uint8_t mode = 0;
    uint8_t data[2] = { 0 };
    size_t dlen;
    __CPROVER_assume(dlen <= sizeof(data));

    uint8_t conf_buf[8] = { 0 };
    uint8_t func_buf[16] = { 0 };
    bool use_conf;
    bool use_func;
    dlt698_connect_params_t params = {
        .version = 0x0016u,
        .conf = use_conf ? conf_buf : NULL,
        .func = use_func ? func_buf : NULL,
        .timeout = 60u,
        .result = 0u,
    };

    size_t tailroom = net_buf_simple_tailroom(&out.b);
    bool expected_fail = false;
    int rc = -1;

    if (which == 0) {
        expected_fail = (10u + 11u + sa.len) > tailroom;
        rc = dlt698_encode_set_response(&sa, piid, oad, dar, &out);
    } else if (which == 1) {
        expected_fail = (method == 0) || ((11u + dlen + 11u + sa.len) > tailroom);
        rc = dlt698_encode_action_response(&sa, piid, oi, method, mode, dar, data, dlen, &out);
    } else if (which == 2) {
        expected_fail = (75u + 11u + sa.len) > tailroom;
        rc = dlt698_encode_connect_response(&sa, piid, &params, &out);
    } else {
        expected_fail = (5u + 11u + sa.len) > tailroom;
        rc = dlt698_encode_release_response(&sa, piid, &out);
    }

    __CPROVER_assert(rc == 0 || rc == -1,
                     "dlt698_encode_responses: return code in contract");
    __CPROVER_assert((rc == -1) == expected_fail,
                     "dlt698_encode_responses: -1 iff invalid or insufficient capacity");
    if (rc == 0) {
        size_t used = (size_t)(out.b.data - out.b.__buf) + out.b.len;
        __CPROVER_assert(out.b.len <= cap,
                         "dlt698_encode_responses: encoded length within cap");
        __CPROVER_assert(used <= FORMAL_MAX_LEN,
                         "dlt698_encode_responses: writes within storage");
    }
    __CPROVER_cover(rc == 0);
    return 0;
}

/* Rejection cases all return before _frame_begin/_frame_seal, so function mode needs no unwind. */
int responses_reject(void) {
    uint8_t storage[128];
    net_buf_t out = { 0 };
    out.b.__buf = storage;
    out.b.data = storage;
    out.b.len = 0;
    out.b.size = sizeof(storage);

    net_buf_t small = { 0 };
    small.b.__buf = storage;
    small.b.data = storage;
    small.b.len = 0;
    small.b.size = 10;

    dlt698_addr_t sa = { 0 };
    sa.len = 1;
    sa.addr[0] = 0x01;
    uint8_t payload[] = { DLT698_TAG_UNSIGNED, 0x01 };
    dlt698_connect_params_t params = { .version = 0x0016u, .timeout = 60u, .result = 0u };

    __CPROVER_assert(dlt698_encode_set_response(NULL, 0x01, 0x40010200u, 0, &out) == -1,
                     "dlt698_encode_responses: set null sa rejected");
    __CPROVER_assert(dlt698_encode_set_response(&sa, 0x01, 0x40010200u, 0, NULL) == -1,
                     "dlt698_encode_responses: set null out rejected");
    __CPROVER_assert(dlt698_encode_set_response(&sa, 0x01, 0x40010200u, 0, &small) == -1,
                     "dlt698_encode_responses: set undersized cap rejected");

    __CPROVER_assert(dlt698_encode_action_response(NULL, 0x01, 0x4000u, 1, 0, 0, payload,
                                                   sizeof(payload), &out) == -1,
                     "dlt698_encode_responses: action null sa rejected");
    __CPROVER_assert(dlt698_encode_action_response(&sa, 0x01, 0x4000u, 1, 0, 0, payload,
                                                   sizeof(payload), NULL) == -1,
                     "dlt698_encode_responses: action null out rejected");
    __CPROVER_assert(dlt698_encode_action_response(&sa, 0x01, 0x4000u, 1, 0, 0, NULL,
                                                   1, &out) == -1,
                     "dlt698_encode_responses: action null data with dlen>0 rejected");
    __CPROVER_assert(dlt698_encode_action_response(&sa, 0x01, 0x4000u, 0, 0, 0, payload,
                                                   sizeof(payload), &out) == -1,
                     "dlt698_encode_responses: action method 0 rejected");
    __CPROVER_assert(dlt698_encode_action_response(&sa, 0x01, 0x4000u, 1, 0, 0, payload,
                                                   sizeof(payload), &small) == -1,
                     "dlt698_encode_responses: action undersized cap rejected");

    __CPROVER_assert(dlt698_encode_connect_response(NULL, 0x01, &params, &out) == -1,
                     "dlt698_encode_responses: connect null sa rejected");
    __CPROVER_assert(dlt698_encode_connect_response(&sa, 0x01, &params, NULL) == -1,
                     "dlt698_encode_responses: connect null out rejected");
    __CPROVER_assert(dlt698_encode_connect_response(&sa, 0x01, NULL, &out) == -1,
                     "dlt698_encode_responses: connect null params rejected");
    __CPROVER_assert(dlt698_encode_connect_response(&sa, 0x01, &params, &small) == -1,
                     "dlt698_encode_responses: connect undersized cap rejected");

    __CPROVER_assert(dlt698_encode_release_response(NULL, 0x01, &out) == -1,
                     "dlt698_encode_responses: release null sa rejected");
    __CPROVER_assert(dlt698_encode_release_response(&sa, 0x01, NULL) == -1,
                     "dlt698_encode_responses: release null out rejected");
    __CPROVER_assert(dlt698_encode_release_response(&sa, 0x01, &small) == -1,
                     "dlt698_encode_responses: release undersized cap rejected");

    return 0;
}

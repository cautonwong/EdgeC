#ifndef DLT698_CODEC_SOURCE
#define DLT698_CODEC_SOURCE "../../foundation/src/protocol/dlt698_codec.c"
#endif
#include DLT698_CODEC_SOURCE

#ifndef FORMAL_MAX_LEN
#define FORMAL_MAX_LEN 12
#endif

int main(void) {
    uint8_t buf[FORMAL_MAX_LEN];
    uint16_t len;
    __CPROVER_assume(len <= FORMAL_MAX_LEN);
    const uint8_t *apdu = buf + (FORMAL_MAX_LEN - len);
    uint32_t oad = 0;
    dlt698_record_sel_t sel = { 0 };
    dlt698_rcsd_t rcsd = { 0 };
    size_t consumed = 0;

    int rc = dlt698_decode_get_record_request(apdu, len, &oad, &sel, &rcsd, &consumed);

    __CPROVER_assert(rc == 0 || rc == -1 || rc == DLT698_RECORD_UNSUPPORTED_SELECTOR ||
                         rc == DLT698_RECORD_UNSUPPORTED_DATA,
                     "dlt698_get_record: return code in contract");
    if (rc == 0) {
        __CPROVER_assert(consumed <= len, "dlt698_get_record: consumed within input");
        __CPROVER_assert(rcsd.count <= DLT698_RCSD_MAX,
                         "dlt698_get_record: rcsd count within max");
        for (uint8_t i = 0; i < rcsd.count; i++) {
            if (rcsd.items[i].kind == DLT698_CSD_ROAD) {
                __CPROVER_assert(rcsd.items[i].related_count <= DLT698_ROAD_MAX,
                                 "dlt698_get_record: road related_count within max");
            }
        }
    }
    __CPROVER_cover(rc == 0);
    return 0;
}

int selector_reject(void) {
    uint8_t valid_sel1[] = {
        0x50, 0x04, 0x02, 0x00,
        DLT698_RSD_SELECTOR1,
        0x20, 0x21, 0x02, 0x00,
        DLT698_TAG_DATE_TIME_S, 0x07, 0xEA, 0x0B, 0x1B, 0x00, 0x00, 0x00,
        0x01,
        DLT698_CSD_OAD, 0x00, 0x10, 0x02, 0x00,
        DLT698_NO_TIME_TAG
    };
    uint32_t oad = 0;
    dlt698_record_sel_t sel = { 0 };
    dlt698_rcsd_t rcsd = { 0 };
    size_t consumed = 0;

    int ok_rc = dlt698_decode_get_record_request(valid_sel1, sizeof(valid_sel1), &oad, &sel, &rcsd,
                                                 &consumed);
    __CPROVER_assert(ok_rc == 0, "dlt698_get_record: valid selector 1 accepted");

    uint8_t unsupported_sel[] = {
        0x50, 0x04, 0x02, 0x00,
        0x09,
        0x01
    };
    int unsup_rc = dlt698_decode_get_record_request(unsupported_sel, sizeof(unsupported_sel), &oad,
                                                    &sel, &rcsd, &consumed);
    __CPROVER_assert(unsup_rc == DLT698_RECORD_UNSUPPORTED_SELECTOR,
                     "dlt698_get_record: unsupported selector rejected");

    uint8_t truncated_sel2[] = {
        0x50, 0x04, 0x02, 0x00,
        DLT698_RSD_SELECTOR2,
        0x20, 0x21, 0x02, 0x00,
        DLT698_TAG_DATE_TIME_S, 0x07, 0xEA
    };
    int trunc_rc = dlt698_decode_get_record_request(truncated_sel2, sizeof(truncated_sel2), &oad,
                                                    &sel, &rcsd, &consumed);
    __CPROVER_assert(trunc_rc == -1, "dlt698_get_record: truncated selector 2 rejected");
    return 0;
}

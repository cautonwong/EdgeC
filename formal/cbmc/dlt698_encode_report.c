#ifndef DLT698_CODEC_SOURCE
#define DLT698_CODEC_SOURCE "../../foundation/src/protocol/dlt698_codec.c"
#endif
#include DLT698_CODEC_SOURCE

#ifndef FORMAL_MAX_LEN
#define FORMAL_MAX_LEN 12
#endif

int main(void) {
    uint8_t out_buf[FORMAL_MAX_LEN];
    uint16_t cap;
    __CPROVER_assume(cap <= FORMAL_MAX_LEN);
    uint8_t *buf = out_buf + (FORMAL_MAX_LEN - cap);

    dlt698_get_item_t items[DLT698_GET_LIST_MAX];
    uint8_t data_pool[DLT698_GET_LIST_MAX][8];
    for (size_t i = 0; i < DLT698_GET_LIST_MAX; i++) {
        __CPROVER_assume(items[i].dlen <= 8);
        items[i].data = data_pool[i];
    }

    uint8_t choice;
    uint8_t piid;
    size_t n;
    size_t len = 0;

    int rc = dlt698_encode_report_notification(buf, cap, choice, piid, items, n, &len);

    __CPROVER_assert(rc == 0 || rc == -1 || rc == DLT698_REPORT_UNSUPPORTED_RECORD,
                     "dlt698_encode_report: return code in contract");
    if (rc == 0) {
        __CPROVER_assert(len <= cap, "dlt698_encode_report: output length within cap");
    }
    __CPROVER_cover(rc == 0);
    return 0;
}

int report_reject(void) {
    uint8_t buf[32];
    size_t len = 0;
    uint8_t payload[] = { DLT698_TAG_UNSIGNED, 0x2A };
    dlt698_get_item_t item = {
        .oad = 0x00100200u,
        .dar = 0,
        .data = payload,
        .dlen = sizeof(payload),
    };

    int ok_rc = dlt698_encode_report_notification(buf, sizeof(buf), DLT698_REPORT_LIST, 0x01,
                                                  &item, 1, &len);
    __CPROVER_assert(ok_rc == 0 && len == 10,
                     "dlt698_encode_report: valid report list accepted");

    int rec_rc = dlt698_encode_report_notification(buf, sizeof(buf), DLT698_REPORT_RECORD_LIST,
                                                   0x01, &item, 1, &len);
    __CPROVER_assert(rec_rc == DLT698_REPORT_UNSUPPORTED_RECORD,
                     "dlt698_encode_report: record list rejected");

    int cap_rc = dlt698_encode_report_notification(buf, 9, DLT698_REPORT_LIST, 0x01, &item, 1,
                                                   &len);
    __CPROVER_assert(cap_rc == -1, "dlt698_encode_report: undersized cap rejected");

    return 0;
}

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "protocol_converter.h"

int main(void) {
    printf("[TEST] Testing Protocol Converter...\n");

    protocol_converter_t conv;
    protocol_converter_init(&conv, PROTO_FMT_RAW_HEX, PROTO_FMT_SPARKPLUG_B_JSON);

    uint8_t raw[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x02, 0xC4, 0x0B};
    char out[256];
    size_t written = 0;

    int ret = protocol_converter_convert(&conv, raw, sizeof(raw), out, sizeof(out), &written);
    assert(ret == 0);
    assert(written > 0);
    assert(strstr(out, "\"status\":\"OK\"") != NULL);
    assert(conv.converted_bytes == sizeof(raw));

    printf("Converted Payload: %s\n", out);
    printf("[TEST] Protocol Converter PASSED!\n");
    return 0;
}

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "iec_cp56time.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < sizeof(iec_cp56time_t)) return 0;

    iec_cp56time_t ts;
    memcpy(&ts, data, sizeof(ts));
    uint64_t ms = iec_cp56time_to_ms(&ts);

    iec_cp56time_t out;
    iec_cp56time_from_ms(&out, ms);
    return 0;
}

#ifdef STANDALONE_FUZZER
int main(void) {
    const uint8_t seed[sizeof(iec_cp56time_t)] = {0x6e, 0x16, 0x04, 0x21, 0x02, 0x01, 0x1a};
    LLVMFuzzerTestOneInput(seed, sizeof(seed));
    printf("[FUZZ_STANDALONE] CP56Time2a production codec executed with seed corpus.\n");
    return 0;
}
#endif

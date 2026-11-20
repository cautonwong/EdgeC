#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include "iems_driver_adapter.h"

static const edge_connectivity_t *find_channel(const edge_connectivity_t *channels,
                                               uint32_t count, int id) {
    for (uint32_t i = 0; i < count; i++) {
        if (channels[i].id == id) return &channels[i];
    }
    return NULL;
}

static void test_product_local_connectivity(void **state) {
    (void)state;
    uint32_t count = 0;
    const edge_connectivity_t *channels = iems_get_connectivity(&count);
    assert_int_equal(count, 5);

    const edge_connectivity_t *adc = find_channel(channels, count, 200);
    assert_non_null(adc);
    assert_string_equal(adc->label, "HighSpeed_ADC");
    assert_string_equal(adc->type, "MEM_RING");
    assert_string_equal(adc->simulation_path, "/tmp/v_adc.sock");
    assert_string_equal(adc->production_path, "adc_raw");

    const edge_connectivity_t *north = find_channel(channels, count, 10);
    assert_non_null(north);
    assert_string_equal(north->category, "NORTHBOUND");
    assert_string_equal(north->production_path, "/dev/ttyS2");

    /* IEC104 是 TCP 端点，不是 UDS 路径；id 必须在 IEMS_MAX_VBUS 内。 */
    const edge_connectivity_t *iec = find_channel(channels, count, 11);
    assert_non_null(iec);
    assert_string_equal(iec->category, "NORTHBOUND");
    assert_string_equal(iec->type, "IEC104");
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_product_local_connectivity),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}

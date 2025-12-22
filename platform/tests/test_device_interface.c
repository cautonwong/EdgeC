#include <signal.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <cmocka.h>

#include <cedgex/device_iface.h>  // The SDK interface we are testing

// We need to reach into the "product" code to get our test subject.
// In a real, complex project, you might have a dedicated test driver
// compiled into the test suite. For this boilerplate, using the sim_driver
// is a perfect example.
extern cedgex_device_driver_t* create_sim_device(const char* name);

/*
 * A test case for creating, initializing, reading from, and closing a device
 * that conforms to the cedgex_device_driver_t interface.
 */
static void test_sim_device_lifecycle(void** state) {
  (void)state; /* unused */

  const char* device_name = "Test-Sensor-01";
  cedgex_device_driver_t* dev = create_sim_device(device_name);

  // 1. Test creation
  assert_non_null(dev);
  assert_string_equal(dev->name, device_name);
  assert_non_null(dev->vtable);

  // 2. Test initialization
  int init_ret = DEVICE_INIT(dev);
  assert_int_equal(init_ret, 0);

  // 3. Test reading
  cedgex_datapoint_t pt;
  int read_ret = DEVICE_READ(dev, &pt);

  assert_int_equal(read_ret, 0);
  assert_int_equal(pt.tag_id, 1001);
  assert_int_equal(pt.quality, CEDGEX_QUALITY_GOOD);
  assert_int_equal(pt.type, CEDGEX_VAL_TYPE_FLOAT);
  assert_true(pt.ts > 0);
  assert_true(pt.val.f_val >= 22.0);
  assert_true(pt.val.f_val <= 32.0);

  // 4. Test closing
  int close_ret = DEVICE_CLOSE(dev);
  assert_int_equal(close_ret, 0);

  // 5. Clean up memory
  free(dev);
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_sim_device_lifecycle),
  };

  // Run the tests
  return cmocka_run_group_tests(tests, NULL, NULL);
}

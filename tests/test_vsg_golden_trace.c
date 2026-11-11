/* Numeric gate captured from the pre-split implementation at 82bd3870^. */
#undef NDEBUG
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "vsg_control.h"

typedef struct {
    int step;
    double frequency_hz;
    double voltage_v;
    double theta_rad;
    double p_mech_w;
    double p_damping_w;
    double p_net_w;
} golden_sample_t;

static const golden_sample_t k_golden[] = {
    {0, 50.0, 220.0, 3.1415926535897936, 100000.0, 0.0, 0.0},
    {10, 49.958108987381529, 219.81930261308403, 3.1302870056475101, 100418.91012618475,
     -6.5802248746820169, 425.49035105943562},
    {50, 49.855274336398651, 219.8161137443642, 2.8874665075361836, 101447.25663601354,
     -22.733454077796011, 1469.9900900913349},
    {99, 49.799476049862285, 219.72417061611372, 5.5034665151091833, 102005.23950137717,
     -31.498228431073017, -2463.2622701917521},
};

static void assert_near(double actual, double expected) {
    assert(fabs(actual - expected) < 1e-9);
}

static void test_vsg_golden_trace(void) {
    vsg_controller_storage_t store;
    vsg_config_t cfg = {.j_inertia = 2.5,
                        .d_damping = 25.0,
                        .v_nominal = 220.0,
                        .f_nominal = 50.0,
                        .p_ref = 100000.0,
                        .q_ref = 0.0,
                        .droop_p = 1e-4,
                        .droop_q = 1e-3};
    vsg_controller_t *vsg = vsg_controller_create_in(&store, &cfg, NULL);
    assert(vsg != NULL);

    size_t g = 0;
    for (int i = 0; i < 100; i++) {
        double p = 100000.0 + (double)(i % 10) * 500.0;
        double q = (double)(i % 5) * 100.0;
        double f = 0.0, v = 0.0;
        assert(vsg_controller_step(vsg, p, q, 0.01, &f, &v) == 0);
        if (i == k_golden[g].step) {
            assert_near(f, k_golden[g].frequency_hz);
            assert_near(v, k_golden[g].voltage_v);
            assert_near(vsg_controller_get_theta(vsg), k_golden[g].theta_rad);
            assert_near(vsg_controller_get_p_mech(vsg), k_golden[g].p_mech_w);
            assert_near(vsg_controller_get_p_damping(vsg), k_golden[g].p_damping_w);
            assert_near(vsg_controller_get_p_net(vsg), k_golden[g].p_net_w);
            g++;
        }
    }
    assert(g == sizeof(k_golden) / sizeof(k_golden[0]));
}

int main(void) {
    test_vsg_golden_trace();
    printf("VSG golden trace passed\n");
    return 0;
}

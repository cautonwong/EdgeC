#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>

#include "stream_engine.h"
#include "datahub.h"

extern pipeline_stage_t *stage_vibration_features_create(size_t window_size, uint32_t rms_tag, uint32_t kurt_tag, uint32_t skew_tag);
extern pipeline_stage_t *stage_rul_prediction_create(uint32_t rul_tag);

typedef struct {
    datahub_t hub;
    stream_engine_t *se;
    uv_loop_t *loop;
} test_ctx_t;

static int setup(void **state) {
    test_ctx_t *ctx = calloc(1, sizeof(test_ctx_t));
    ctx->loop = uv_default_loop();
    datahub_init(&ctx->hub, ctx->loop, "vibra_test", 128, 10000, NULL, 0, NULL, NULL, NULL);
    ctx->se = stream_engine_create(&ctx->hub, ctx->loop, NULL, NULL);
    stream_engine_start(ctx->se);
    *state = ctx;
    return 0;
}

static int teardown(void **state) {
    test_ctx_t *ctx = *state;
    stream_engine_destroy(ctx->se);
    datahub_destroy(&ctx->hub);
    free(ctx);
    return 0;
}

static void test_simulated_wear_and_rul(void **state) {
    test_ctx_t *ctx = *state;
    pipeline_t *p = stream_engine_add_pipeline(ctx->se, "vibra_pipe", 1001);

    // Stage 1: Vibration Features (window=100) -> Output RMS to 2001
    pipeline_stage_t *st_feat = stage_vibration_features_create(100, 2001, 2002, 2003);
    pipeline_add_stage(p, st_feat->name, st_feat->process, st_feat->priv_state, st_feat->cleanup_state, NULL, NULL);
    free(st_feat);

    // Stage 2: RUL Prediction -> Output RUL to 3001
    pipeline_t *p2 = stream_engine_add_pipeline(ctx->se, "rul_pipe", 2001);
    pipeline_stage_t *st_rul = stage_rul_prediction_create(3001);
    pipeline_add_stage(p2, st_rul->name, st_rul->process, st_rul->priv_state, st_rul->cleanup_state, NULL, NULL);
    free(st_rul);

    // Inject synthetic vibration signal purely via Hub (matching production)
    pipe_msg_t msg;
    double base_noise = 0.5;
    double wear_factor = 0.0;

    for (int step = 0; step < 3; step++) {
        wear_factor += 0.2; 

        for (int i = 0; i < 100; i++) {
            double val = sin(i * 0.1) + ((double)rand() / RAND_MAX) * base_noise * (1.0 + wear_factor * 5.0);
            pipe_msg_init_scalar(&msg, 1001, val, uv_hrtime());
            datahub_publish(&ctx->hub, &msg);
        }

        // Check RUL (Polling for async result)
        pipe_msg_t rul_msg;
        int found = -1;
        for (int retry = 0; retry < 100; retry++) {
            // Flush the main loop to process shadow updates
            uv_run(ctx->loop, UV_RUN_NOWAIT);
            
            if (datahub_read_shadow(&ctx->hub, 3001, &rul_msg) == 0) {
                found = 0;
                break;
            }
            usleep(10000); // 10ms
        }
        
        assert_int_equal(found, 0);
        printf("Step %d - RUL: %.2f%%\n", step, rul_msg.payload.val.f);
        assert_true(rul_msg.payload.val.f >= 0.0 && rul_msg.payload.val.f <= 100.0);
    }
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_simulated_wear_and_rul, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}

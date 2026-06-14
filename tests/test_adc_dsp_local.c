#define _GNU_SOURCE
#include <cmocka.h>
#include <fcntl.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#include <math.h>

#include "datahub.h"
#include "stream_engine.h"

extern void stream_engine_register_sim_pipeline(stream_engine_t* engine, datahub_t* hub,
                                                uint32_t trigger_tag, uint32_t target_tag);

static void test_adc_dsp_full_chain(void** state) {
    (void)state;
    printf("\n[STEP 1] Initializing DataHub and StreamEngine...\n");
    uv_loop_t* loop = uv_default_loop();
    // 即使不传元数据，Ring Buffer 也能工作
    datahub_t hub;
    datahub_init(&hub, loop, "test", 128, 128, NULL, 0, NULL, NULL, NULL);
    stream_engine_t* se = stream_engine_create(&hub, uv_default_loop(), NULL, NULL);

    printf("[STEP 2] Registering Simulation Pipeline (Trigger=12001, Target=1001)...\n");
    stream_engine_register_sim_pipeline(se, &hub, 12001, 1001);

    printf("[STEP 3] Mocking ADC DMA: Writing signal with 10%% 3rd harmonic...\n");
    int fd = memfd_create("proof_shm", 0);
    ftruncate(fd, 4096);
    float* ptr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    for (int i = 0; i < 200; i++) {
        ptr[i] = 100.0f * sin(2.0 * M_PI * i / 200.0) + 10.0f * sin(2.0 * M_PI * i / 200.0 * 3.0);
    }

    printf("[STEP 4] Simulating UDS Pulse: Pushing Tag 12001 to StreamEngine...\n");
    pipe_msg_t msg = {
        .type = PIPEMSG_TYPE_REF, .tag_id = 12001, .payload.ref.fd = fd, .payload.ref.len = 1600};
    stream_engine_push(se, &msg);

    printf("[STEP 5] Verifying Result in DataHub Ring Buffer...\n");
    uint64_t seq = 0;
    pipe_msg_t result;
    bool found_rms = false;
    bool found_thd = false;
    float rms_val = 0.0f;
    float thd_val = 0.0f;
    
    // 增加搜索深度
    for (int i = 0; i < 100; i++) {
        if (shm_ring_read(&hub.ring, &seq, &result) == 0) {
            if (result.tag_id == 1001) {
                found_rms = true;
                rms_val = result.payload.val.f;
            } else if (result.tag_id == 1002) {
                found_thd = true;
                thd_val = result.payload.val.f;
            }
        }
    }

    assert_true(found_rms);
    assert_true(found_thd);
    assert_float_equal(rms_val, 71.063f, 0.1f);
    assert_float_equal(thd_val, 0.1f, 0.01f);
    printf("\n🔥 [SUCCESS] Full Chain Verified! DSP Output RMS: %.2f, THD: %.3f\n", rms_val, thd_val);

    munmap(ptr, 4096);
    close(fd);
    stream_engine_destroy(se);
    datahub_destroy(&hub);
}

int main(void) {
    const struct CMUnitTest tests[] = {cmocka_unit_test(test_adc_dsp_full_chain)};
    return cmocka_run_group_tests(tests, NULL, NULL);
}

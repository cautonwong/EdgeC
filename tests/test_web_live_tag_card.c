/**
 * test_web_live_tag_card.c - Verification of LiveTagCard Component & High-Rate Rendering (AEP-492)
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void read_file_content(const char *rel_path, char *buf, size_t max_len) {
    char path[512];
    FILE *fp = NULL;

    const char *prefixes[] = {"", "../", "../../", "/workspaces/edgeos/EdgeC/", NULL};

    for (int i = 0; prefixes[i] != NULL; i++) {
        snprintf(path, sizeof(path), "%s%s", prefixes[i], rel_path);
        fp = fopen(path, "r");
        if (fp) break;
    }

    assert_non_null(fp);
    size_t n = fread(buf, 1, max_len - 1, fp);
    buf[n] = '\0';
    fclose(fp);
}

static void test_shallow_ref_and_raf_optimization(void **state) {
    (void)state;
    char content[32768];
    read_file_content("web/core/src/components/LiveTagCard.vue", content, sizeof(content));

    // Must use shallowRef instead of deep reactive proxies to eliminate GC overhead
    assert_non_null(strstr(content, "import { shallowRef"));
    assert_non_null(strstr(content, "const value = shallowRef<number | null>(null)"));
    assert_non_null(strstr(content, "const lastUpdateTs = shallowRef<number>(0)"));
    assert_non_null(strstr(content, "const valueChanged = shallowRef(false)"));
    assert_non_null(strstr(content, "requestAnimationFrame"));
}

static void test_animation_and_breathing_flash(void **state) {
    (void)state;
    char content[32768];
    read_file_content("web/core/src/components/LiveTagCard.vue", content, sizeof(content));

    // 400ms smooth breathing animation
    assert_non_null(strstr(content, "duration-400"));
    assert_non_null(strstr(content, "400"));
    assert_non_null(strstr(content, "flashColor"));
    assert_non_null(strstr(content, "scale-[1.03]"));
}

static void test_three_state_liveliness_machine(void **state) {
    (void)state;
    char content[32768];
    read_file_content("web/core/src/components/LiveTagCard.vue", content, sizeof(content));

    // Three states: LIVE, STALE, OFFLINE
    assert_non_null(strstr(content, "'LIVE' | 'STALE' | 'OFFLINE'"));
    assert_non_null(strstr(content, "diff <= 2000"));
    assert_non_null(strstr(content, "diff <= 10000"));
    assert_non_null(strstr(content, "status.value = 'OFFLINE'"));
}

static void test_lifecycle_cleanup_safety(void **state) {
    (void)state;
    char content[32768];
    read_file_content("web/core/src/components/LiveTagCard.vue", content, sizeof(content));

    // onUnmounted cleans up timers and RAF
    assert_non_null(strstr(content, "onUnmounted"));
    assert_non_null(strstr(content, "clearInterval(timer)"));
    assert_non_null(strstr(content, "clearTimeout(animTimer)"));
    assert_non_null(strstr(content, "cancelAnimationFrame(rafId)"));
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_shallow_ref_and_raf_optimization),
        cmocka_unit_test(test_animation_and_breathing_flash),
        cmocka_unit_test(test_three_state_liveliness_machine),
        cmocka_unit_test(test_lifecycle_cleanup_safety),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}

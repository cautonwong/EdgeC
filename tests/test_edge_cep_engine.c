/* Release 构建带 -DNDEBUG，assert() 会被抹成空语句 → 测试变空壳 */
#undef NDEBUG
#include <assert.h>
#include <stdio.h>

#include "edge_cep_engine.h"

int main(void) {
    printf("=== Test Edge CEP Engine Start ===\n");

    cep_engine_storage_t store;
    cep_engine_t *engine = cep_engine_create_in(&store);
    assert(engine != NULL);

    /* Rule 1: High Motor Temp (Tag 1001 >= 80.0) followed by High Vibration (Tag 1002 >= 5.0)
     * within 3000ms -> Alert 9901 */
    assert(cep_engine_add_rule(engine, 1, 1001, 80.0, 1002, 5.0, 3000, 9901) == 0);

    uint16_t alert_tag = 0;
    bool triggered = false;

    /* Feed Event A at t = 1000ms */
    cep_event_t ev1 = {.tag_id = 1001, .val = 85.0, .timestamp_ms = 1000};
    assert(cep_engine_feed_event(engine, &ev1, &alert_tag, &triggered) == 0);
    assert(triggered == false);

    /* Feed Event B at t = 2500ms (diff = 1500ms <= 3000ms) -> Should trigger Alert 9901 */
    cep_event_t ev2 = {.tag_id = 1002, .val = 6.2, .timestamp_ms = 2500};
    assert(cep_engine_feed_event(engine, &ev2, &alert_tag, &triggered) == 0);
    assert(triggered == true);
    assert(alert_tag == 9901);

    /* Feed Event B again without new Event A -> Should NOT trigger */
    cep_event_t ev3 = {.tag_id = 1002, .val = 7.0, .timestamp_ms = 2600};
    assert(cep_engine_feed_event(engine, &ev3, &alert_tag, &triggered) == 0);
    assert(triggered == false);

    /* Feed Event A at t = 5000ms */
    cep_event_t ev4 = {.tag_id = 1001, .val = 82.0, .timestamp_ms = 5000};
    assert(cep_engine_feed_event(engine, &ev4, &alert_tag, &triggered) == 0);
    assert(triggered == false);

    /* Feed Event B at t = 9000ms (diff = 4000ms > 3000ms) -> Expired, Should NOT trigger */
    cep_event_t ev5 = {.tag_id = 1002, .val = 8.0, .timestamp_ms = 9000};
    assert(cep_engine_feed_event(engine, &ev5, &alert_tag, &triggered) == 0);
    assert(triggered == false);

    printf("=== Test Edge CEP Engine Passed (123/123) ===\n");
    return 0;
}

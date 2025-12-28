#include <time.h>

#include "sched_time.h"

static wall_time_t to_ms(struct tm *tm) {
    return (wall_time_t)mktime(tm) * 1000;
}

wall_time_t calc_next_boundary(calendar_task_type_t type, wall_time_t now) {
    time_t sec = (time_t)(now / 1000);
    struct tm t;
    localtime_r(&sec, &t);
    t.tm_isdst = -1; // Let mktime determine DST

    switch (type) {
        case CAL_TASK_MINUTE_FREEZE:
            t.tm_sec = 0;
            t.tm_min += 1;
            break;

        case CAL_TASK_HOUR_FREEZE:
            t.tm_sec = 0;
            t.tm_min = 0;
            t.tm_hour += 1;
            break;

        case CAL_TASK_PERIODIC_READ:
            t.tm_sec = 0;
            // Correct alignment logic
            t.tm_min = ((t.tm_min / 15) + 1) * 15;
            break;

        case CAL_TASK_DAY_FREEZE:
            t.tm_sec = 0;
            t.tm_min = 0;
            t.tm_hour = 0;
            t.tm_mday += 1;
            break;

        case CAL_TASK_MONTH_FREEZE:
            t.tm_sec = 0;
            t.tm_min = 0;
            t.tm_hour = 0;
            t.tm_mday = 1;
            t.tm_mon += 1;
            break;
    }

    return to_ms(&t);
}

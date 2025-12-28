#include <time.h>

#include "sched_time.h"

static wall_time_t to_ms(struct tm *tm) {
    return (wall_time_t)mktime(tm) * 1000;
}

wall_time_t calc_next_boundary(calendar_task_t *task, wall_time_t now) {
    time_t sec = (time_t)(now / 1000);
    struct tm t;
    localtime_r(&sec, &t);

    switch (task->type) {
        case CAL_TASK_MINUTE_FREEZE:
            // 下一分钟的指定秒 (如果没有指定则为 00)
            t.tm_sec = (int)task->second;
            t.tm_min += 1;
            return to_ms(&t);

        case CAL_TASK_HOUR_FREEZE:
            // 下一小时的指定分:秒
            t.tm_sec = (int)task->second;
            t.tm_min = (int)task->minute;
            t.tm_hour += 1;
            return to_ms(&t);

        case CAL_TASK_PERIODIC_READ: {
            uint32_t interval = (task->align_min > 0) ? task->align_min : 15;
            t.tm_sec = (int)task->second;
            t.tm_min = (int)((t.tm_min / (int)interval + 1) * (int)interval);
            return to_ms(&t);
        }

        case CAL_TASK_DAY_FREEZE: {
            // 目标时刻：当日的 hour:minute:second
            t.tm_sec = (int)task->second;
            t.tm_min = (int)task->minute;
            t.tm_hour = (int)task->hour;

            wall_time_t target = to_ms(&t);
            if (target <= now) {
                t.tm_mday += 1;
                target = to_ms(&t);
            }
            return target;
        }

        case CAL_TASK_MONTH_FREEZE: {
            // 目标时刻：本月 day 号 hour:minute:second
            t.tm_sec = (int)task->second;
            t.tm_min = (int)task->minute;
            t.tm_hour = (int)task->hour;
            t.tm_mday = (int)((task->day > 0) ? task->day : 1);

            wall_time_t target = to_ms(&t);
            if (target <= now) {
                t.tm_mon += 1;
                target = to_ms(&t);
            }
            return target;
        }

        default:
            return now + 3600000;
    }
}

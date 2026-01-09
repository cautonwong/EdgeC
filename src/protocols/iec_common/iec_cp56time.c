/**
 * @file iec_cp56time.c
 * @brief IEC 60870-5 CP56Time2a 时标编解码实现
 */

#include "iec_cp56time.h"

#include <string.h>
#include <uv.h>

void iec_cp56time_now(iec_cp56time_t *out) {
    uint64_t now_ms = uv_hrtime() / 1000000;
    // 注意：uv_hrtime 是单调时钟，此处应使用实时时钟
    // 为了简化，我们使用标准 time()
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    iec_cp56time_from_ms(out, (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000);
}

void iec_cp56time_from_ms(iec_cp56time_t *out, uint64_t ms) {
    time_t sec = (time_t)(ms / 1000);
    struct tm tm_info;
    gmtime_r(&sec, &tm_info);

    memset(out, 0, sizeof(*out));
    out->msec = (uint16_t)(ms % 60000);
    out->min = (uint8_t)tm_info.tm_min;
    out->hour = (uint8_t)tm_info.tm_hour;
    out->mday = (uint8_t)tm_info.tm_mday;
    out->wday = (uint8_t)(tm_info.tm_wday == 0 ? 7 : tm_info.tm_wday);
    out->month = (uint8_t)(tm_info.tm_mon + 1);
    out->year = (uint8_t)(tm_info.tm_year % 100);
}

uint64_t iec_cp56time_to_ms(const iec_cp56time_t *ts) {
    struct tm tm_info = {0};
    tm_info.tm_sec = 0;  // msec 包含了秒和毫秒
    tm_info.tm_min = ts->min;
    tm_info.tm_hour = ts->hour;
    tm_info.tm_mday = ts->mday;
    tm_info.tm_mon = ts->month - 1;
    tm_info.tm_year = 100 + ts->year;  // 假设 20xx 年
    tm_info.tm_isdst = -1;

    time_t sec = timegm(&tm_info);
    return (uint64_t)sec * 1000 + ts->msec;
}

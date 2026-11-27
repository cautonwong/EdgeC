/**
 * @file iems_strategy_priority.c
 * @brief P5-03d c1: pure key-precedence resolution for EMS and per-LC settings.
 */

#include "iems_strategy_priority.h"

#include <math.h>
#include <stddef.h>

static bool key_finite(const double *key) {
    return key == NULL || isfinite(*key);
}

static void result_reset(iems_prio_result_t *out) {
    out->value = 0.0;
    out->source = IEMS_PRIO_SOURCE_ABSENT;
    out->deprecated_used = false;
    out->conflict = false;
}

/* high wins when present; low only supplies a value when high is absent.
 * conflict is an exact comparison: equal values are not a conflict. */
static iems_prio_error_t resolve_pair(const double *high, const double *low,
                                      iems_prio_source_t high_source,
                                      iems_prio_source_t low_source,
                                      bool low_is_deprecated,
                                      iems_prio_result_t *out) {
    if (out == NULL) return IEMS_PRIO_BAD_INPUT;
    result_reset(out);
    if (!key_finite(high) || !key_finite(low)) return IEMS_PRIO_BAD_INPUT;

    if (low_is_deprecated) out->deprecated_used = low != NULL;
    if (high != NULL) {
        out->value = *high;
        out->source = high_source;
        out->conflict = low != NULL && *high != *low;
        return IEMS_PRIO_OK;
    }
    if (low != NULL) {
        out->value = *low;
        out->source = low_source;
        return IEMS_PRIO_OK;
    }
    return IEMS_PRIO_ABSENT;
}

iems_prio_error_t iems_strategy_priority_resolve_ems(const double *current,
                                                     const double *deprecated,
                                                     iems_prio_result_t *out) {
    return resolve_pair(current, deprecated, IEMS_PRIO_SOURCE_CURRENT,
                        IEMS_PRIO_SOURCE_DEPRECATED, true, out);
}

iems_prio_error_t iems_strategy_priority_resolve_lc(const double *override_value,
                                                    const double *base,
                                                    iems_prio_result_t *out) {
    return resolve_pair(override_value, base, IEMS_PRIO_SOURCE_LC_OVERRIDE,
                        IEMS_PRIO_SOURCE_LC_BASE, false, out);
}

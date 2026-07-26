/**
 * @file device_twin.c
 * @brief State-Reconciled Device Twin Implementation
 */

#include "device_twin.h"
#include <string.h>
#include <math.h>

int device_twin_init(device_twin_t *twin, uint32_t device_id) {
    if (!twin || device_id == 0) {
        return -1;
    }

    memset(twin, 0, sizeof(device_twin_t));
    twin->device_id = device_id;
    return 0;
}

static device_twin_prop_t *find_or_create_prop(device_twin_t *twin, uint32_t prop_id) {
    for (size_t i = 0; i < twin->prop_count; i++) {
        if (twin->properties[i].property_id == prop_id) {
            return &twin->properties[i];
        }
    }

    if (twin->prop_count < DEVICE_TWIN_MAX_PROPERTIES) {
        device_twin_prop_t *prop = &twin->properties[twin->prop_count++];
        memset(prop, 0, sizeof(device_twin_prop_t));
        prop->property_id = prop_id;
        prop->in_sync = true;
        return prop;
    }

    return NULL;
}

int device_twin_set_desired(device_twin_t *twin, uint32_t prop_id, double val) {
    if (!twin || prop_id == 0) {
        return -1;
    }

    device_twin_prop_t *prop = find_or_create_prop(twin, prop_id);
    if (!prop) {
        return -2;
    }

    prop->desired_val = val;
    prop->desired_version++;
    if (fabs(prop->desired_val - prop->reported_val) > 1e-6) {
        prop->in_sync = false;
    }

    return 0;
}

int device_twin_set_reported(device_twin_t *twin, uint32_t prop_id, double val) {
    if (!twin || prop_id == 0) {
        return -1;
    }

    device_twin_prop_t *prop = find_or_create_prop(twin, prop_id);
    if (!prop) {
        return -2;
    }

    prop->reported_val = val;
    prop->reported_version++;
    if (fabs(prop->desired_val - prop->reported_val) <= 1e-6) {
        prop->in_sync = true;
    } else {
        prop->in_sync = false;
    }

    return 0;
}

int device_twin_reconcile(device_twin_t *twin, size_t *out_unsynced_count) {
    if (!twin || !out_unsynced_count) {
        return -1;
    }

    size_t unsynced = 0;
    for (size_t i = 0; i < twin->prop_count; i++) {
        if (fabs(twin->properties[i].desired_val - twin->properties[i].reported_val) <= 1e-6) {
            twin->properties[i].in_sync = true;
        } else {
            twin->properties[i].in_sync = false;
            unsynced++;
        }
    }

    *out_unsynced_count = unsynced;
    return 0;
}

/**
 * @file edge_anti_corruption.c
 * @brief Anti-Corruption Layer (ACL) translation engine implementation
 */

#include "edge_anti_corruption.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define DEFAULT_ACL_CAPACITY 256

int edge_anti_corruption_init(edge_anti_corruption_engine_t *acl, size_t capacity) {
    if (!acl) return -1;

    size_t cap = capacity > 0 ? capacity : DEFAULT_ACL_CAPACITY;
    acl->entries = (edge_anti_corruption_entry_t *)calloc(cap, sizeof(edge_anti_corruption_entry_t));
    if (!acl->entries) return -1;

    acl->capacity = cap;
    acl->count = 0;
    pthread_mutex_init(&acl->lock, NULL);
    acl->initialized = true;
    return 0;
}

void edge_anti_corruption_destroy(edge_anti_corruption_engine_t *acl) {
    if (!acl || !acl->initialized) return;

    pthread_mutex_lock(&acl->lock);
    if (acl->entries) {
        free(acl->entries);
        acl->entries = NULL;
    }
    acl->count = 0;
    acl->capacity = 0;
    acl->initialized = false;
    pthread_mutex_unlock(&acl->lock);
    pthread_mutex_destroy(&acl->lock);
}

int edge_anti_corruption_register(edge_anti_corruption_engine_t *acl, const edge_anti_corruption_entry_t *entry) {
    if (!acl || !acl->initialized || !entry) return -1;

    pthread_mutex_lock(&acl->lock);

    // Expand capacity if needed
    if (acl->count >= acl->capacity) {
        size_t new_cap = acl->capacity * 2;
        edge_anti_corruption_entry_t *new_entries = (edge_anti_corruption_entry_t *)realloc(acl->entries, new_cap * sizeof(edge_anti_corruption_entry_t));
        if (!new_entries) {
            pthread_mutex_unlock(&acl->lock);
            return -1;
        }
        acl->entries = new_entries;
        acl->capacity = new_cap;
    }

    // Check if tag_id or (proto, raw_address) already exists and update
    for (size_t i = 0; i < acl->count; i++) {
        if (acl->entries[i].tag_id == entry->tag_id ||
            (acl->entries[i].proto == entry->proto && acl->entries[i].raw_address == entry->raw_address)) {
            acl->entries[i] = *entry;
            pthread_mutex_unlock(&acl->lock);
            return 0;
        }
    }

    // Insert new entry
    acl->entries[acl->count] = *entry;
    acl->count++;

    pthread_mutex_unlock(&acl->lock);
    return 0;
}

int edge_anti_corruption_register_table(edge_anti_corruption_engine_t *acl, const edge_anti_corruption_entry_t *entries, size_t count) {
    if (!acl || !entries || count == 0) return -1;
    for (size_t i = 0; i < count; i++) {
        if (edge_anti_corruption_register(acl, &entries[i]) != 0) {
            return -1;
        }
    }
    return 0;
}

const edge_anti_corruption_entry_t *edge_anti_corruption_lookup_by_tag(edge_anti_corruption_engine_t *acl, uint32_t tag_id) {
    if (!acl || !acl->initialized) return NULL;

    pthread_mutex_lock(&acl->lock);
    for (size_t i = 0; i < acl->count; i++) {
        if (acl->entries[i].tag_id == tag_id) {
            pthread_mutex_unlock(&acl->lock);
            return &acl->entries[i];
        }
    }
    pthread_mutex_unlock(&acl->lock);
    return NULL;
}

const edge_anti_corruption_entry_t *edge_anti_corruption_lookup_by_address(edge_anti_corruption_engine_t *acl, edge_protocol_type_t proto, uint32_t raw_address) {
    if (!acl || !acl->initialized) return NULL;

    pthread_mutex_lock(&acl->lock);
    for (size_t i = 0; i < acl->count; i++) {
        if (acl->entries[i].proto == proto && acl->entries[i].raw_address == raw_address) {
            pthread_mutex_unlock(&acl->lock);
            return &acl->entries[i];
        }
    }
    pthread_mutex_unlock(&acl->lock);
    return NULL;
}

int edge_anti_corruption_translate_inbound(edge_anti_corruption_engine_t *acl, edge_protocol_type_t proto, uint32_t raw_address, double raw_value, pipe_msg_t *out_msg) {
    if (!acl || !acl->initialized || !out_msg) return -1;

    const edge_anti_corruption_entry_t *entry = edge_anti_corruption_lookup_by_address(acl, proto, raw_address);
    if (!entry) return -1;

    double phys_val = (raw_value * entry->scaler) + entry->offset;

    pipe_msg_init_scalar(out_msg, entry->tag_id, phys_val, 0);

    if (entry->data_type == EDGE_ACL_TYPE_INT32 || entry->data_type == EDGE_ACL_TYPE_UINT32 || entry->data_type == EDGE_ACL_TYPE_BOOL) {
        out_msg->payload.val.i = (int64_t)llround(phys_val);
    }

    return 0;
}

int edge_anti_corruption_translate_outbound(edge_anti_corruption_engine_t *acl, uint32_t tag_id, double physical_value, uint32_t *out_raw_address, double *out_raw_value) {
    if (!acl || !acl->initialized || !out_raw_address || !out_raw_value) return -1;

    const edge_anti_corruption_entry_t *entry = edge_anti_corruption_lookup_by_tag(acl, tag_id);
    if (!entry) return -1;

    if (fabs(entry->scaler) < 1e-9) {
        return -1; // Division by zero guard
    }

    *out_raw_address = entry->raw_address;
    *out_raw_value = (physical_value - entry->offset) / entry->scaler;
    return 0;
}

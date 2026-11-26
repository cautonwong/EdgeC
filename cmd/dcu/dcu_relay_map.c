#include "dcu_relay_map.h"

#include <string.h>

/* 对象表是进程级单例，绑定沿用同一模式：只指向聚合根拥有的表，不复制、不释放。 */
static const dcu_relay_map_t *s_active;

void dcu_relay_map_bind(const dcu_relay_map_t *map) { s_active = map; }

const dcu_relay_map_t *dcu_relay_map_active(void) { return s_active; }

const dcu_relay_entry_t *dcu_relay_map_find(const dcu_relay_map_t *map, uint32_t relay_oad) {
    if (!map) return NULL;
    for (size_t i = 0; i < map->count && i < (size_t)DCU_RELAY_MAP_CAPACITY; i++) {
        if (map->entries[i].relay_oad == relay_oad) return &map->entries[i];
    }
    return NULL;
}

int dcu_relay_map_set(dcu_relay_map_t *map, uint32_t relay_oad, const char *meter_addr,
                      uint8_t do_channel) {
    if (!map || !meter_addr || !meter_addr[0]) return -1;
    size_t addr_len = strlen(meter_addr);
    if (addr_len >= MAX_ADDR_LEN) return -1;

    size_t index = map->count;
    for (size_t i = 0; i < map->count && i < (size_t)DCU_RELAY_MAP_CAPACITY; i++) {
        if (map->entries[i].relay_oad == relay_oad) {
            index = i;
            break;
        }
    }
    if (index >= (size_t)DCU_RELAY_MAP_CAPACITY) return -1;

    dcu_relay_entry_t *entry = &map->entries[index];
    memset(entry, 0, sizeof(*entry));
    entry->relay_oad = relay_oad;
    memcpy(entry->meter_addr, meter_addr, addr_len + 1);
    entry->do_channel = do_channel;
    if (index == map->count) map->count++;
    return 0;
}

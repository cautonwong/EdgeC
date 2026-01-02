#ifndef DCU_INVENTORY_H
#define DCU_INVENTORY_H

#include "edge_service.h"

/**
 * @brief DCU 档案条目
 */
typedef struct {
    char name[32];
    int bus_id;
    uint32_t slave_id;
    char protocol[16];
} dcu_meter_t;

/**
 * @brief 添加电表档案 (Mock/DB)
 */
void dcu_inventory_add_meter(service_t *svc, const char *name, int bus_id, uint32_t slave_id,
                             const char *proto);

/**
 * @brief 加载档案并物理分发抄读任务
 */
void dcu_inventory_load_and_run(service_t *svc);

#endif

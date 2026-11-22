#ifndef DCU_SCHEME_STORE_H
#define DCU_SCHEME_STORE_H

#include <stdint.h>

#include "dcu_poller.h"
#include "dcu_storage.h"

/** @brief 建表 + 首次启动播种默认方案（ID 1），只播种一次；new DB 由调用方给默认 tag。 */
int dcu_scheme_store_init(dcu_storage_t *storage, const uint32_t *default_tags,
                          uint16_t default_count);

/** @brief 把库里的方案装进 poller；坏行跳过并告警，不清库。装载前先清 RAM。 */
int dcu_scheme_store_load(dcu_storage_t *storage, dcu_poller_t *poller);

/** @brief 6014 方法 127/128/129/130 的事务化落地；失败回滚 DB 与 RAM。 */
int dcu_scheme_store_apply(dcu_storage_t *storage, dcu_poller_t *poller, const dcu_scheme_op_t *op);

#endif /* DCU_SCHEME_STORE_H */

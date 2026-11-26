#ifndef DCU_RULE_STORE_H
#define DCU_RULE_STORE_H

#include <stdint.h>

#include "dcu_698_rules.h"

int dcu_rule_store_init(dcu_storage_t *storage, const dcu_rule_row_t *seed, uint16_t count);
int dcu_rule_store_load(dcu_storage_t *storage, dcu_rule_cache_t *out);
int dcu_rule_store_apply(dcu_storage_t *storage, const dcu_rule_row_t *seed, uint16_t seed_count,
                         const dcu_rule_op_t *op);
int dcu_rule_store_projected_count(dcu_storage_t *storage, uint16_t *out);
int dcu_rule_store_projected_oad(dcu_storage_t *storage, uint16_t pos, uint32_t *out);
int dcu_rule_store_unit_load(dcu_storage_t *storage, uint32_t oad, dcu_rule_unit_t *out);

#endif /* DCU_RULE_STORE_H */

#ifndef EDGE_COMMON_H
#define EDGE_COMMON_H

#include "edge_common_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "contract_data_types.h"

/**
 * @brief 可执行文件所在根目录 (edge_core 提供，非 foundation)
 */
int edge_get_app_root(char *out_path, size_t size);

#ifdef __cplusplus
}
#endif

#endif

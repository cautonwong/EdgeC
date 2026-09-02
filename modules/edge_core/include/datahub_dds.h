#ifndef EDGE_DATAHUB_DDS_H
#define EDGE_DATAHUB_DDS_H

#include "datahub.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief [ADR-0005] Cyclone DDS Backend VTable Structure
 */
extern const datahub_ops_t g_datahub_dds_ops;

/**
 * @brief Initialize Cyclone DDS DataHub Backend Instance
 */
int datahub_dds_init(datahub_t *hub, const datahub_config_t *config);

/**
 * @brief 四遥工业级 QoS 策略矩阵生成函数 (YC 遥测流: BEST_EFFORT / VOLATILE)
 */
void *datahub_dds_create_qos_yc(void);

/**
 * @brief 四遥工业级 QoS 策略矩阵生成函数 (YX 遥信告警: RELIABLE / TRANSIENT_LOCAL 支持晚加入节点追忆)
 */
void *datahub_dds_create_qos_yx(void);

/**
 * @brief 四遥工业级 QoS 策略矩阵生成函数 (YK 遥控指令: RELIABLE / VOLATILE / KEEP_ALL 确定性交付)
 */
void *datahub_dds_create_qos_yk(void);

/**
 * @brief 四遥工业级 QoS 策略矩阵生成函数 (YT 遥调定值: RELIABLE / VOLATILE / KEEP_LAST)
 */
void *datahub_dds_create_qos_yt(void);

/**
 * @brief 显式按 YC 遥测 QoS 主题发布数据
 */
int datahub_dds_publish_yc(datahub_t *hub, const pipe_msg_t *msg);

/**
 * @brief 显式按 YX 遥信/告警 QoS 主题发布数据 (支持晚加入节点历史留存)
 */
int datahub_dds_publish_yx(datahub_t *hub, const pipe_msg_t *msg);

/**
 * @brief 显式按 YK 遥控/定值 QoS 主题发布数据 (严格可靠保序下发)
 */
int datahub_dds_publish_yk(datahub_t *hub, const pipe_msg_t *msg);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_DATAHUB_DDS_H */

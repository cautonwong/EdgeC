/**
 * @file mqtt_json_codec.h
 * @brief 裸 JSON 北向协议的编解码入口（ADR-0011 N6-2）
 *
 * ## 为什么解码放在这里而不是 cmd
 *
 * 报文字节格式是**协议知识**，属于格式的所有者（这个驱动）。
 * 产品只负责"解析出来的字段 → 哪个聚合根方法"。
 *
 * 如果让每个产品各写一份 JSON 解析，格式一改就要改 N 处，
 * 而且各家的容错程度不同 —— 那才是真正的耦合。
 *
 * ## 下行约定
 *
 * 载荷：`{"p":"<metric 名>","v":<数值>}`（键顺序无关，允许空白）
 * 主题：由北向地址模板决定，编排层不解析
 */

#ifndef MQTT_JSON_CODEC_H
#define MQTT_JSON_CODEC_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 解析一条下行报文。
 *
 * @param payload   原始字节（不要求 NUL 结尾）
 * @param len       字节数
 * @param name      输出 metric 名（NUL 结尾）
 * @param name_sz   name 缓冲大小
 * @param value     输出数值
 * @return 0 = 解析成功；<0 = 格式不符（缺字段、类型不对、超长）
 *
 * 失败时**不写** name/value —— 调用方不该拿到"半解析"的结果。
 */
int mqtt_json_decode_downlink(const uint8_t *payload, size_t len, char *name, size_t name_sz,
                              double *value);

#ifdef __cplusplus
}
#endif

#endif /* MQTT_JSON_CODEC_H */

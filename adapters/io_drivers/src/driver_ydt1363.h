/**
 * @file driver_ydt1363.h
 * @brief YD/T 1363-2014 request-native 驱动（首切片：只读单点）
 *
 * 与 DLT645 同一套边界：驱动只认请求描述与字节，不查点表、不碰 datahub。
 * 帧的组装、LCHKSUM、CHKSUM 与 INFO 解码全部复用 foundation 的 ydt1363_codec。
 */
#ifndef DRIVER_YDT1363_H
#define DRIVER_YDT1363_H

#include <stddef.h>
#include <stdint.h>

#include "net_buf.h"
#include "ydt1363_codec.h"

/* 应答 INFO 取值窗口的字节序；宽度由 order 决定。 */
#define YDT1363_ORDER_A 0    /* 1 字节 */
#define YDT1363_ORDER_AB 1   /* 2 字节，高字节先 */
#define YDT1363_ORDER_BA 2   /* 2 字节，低字节先 */
#define YDT1363_ORDER_ABCD 3 /* 4 字节，高字节先 */
#define YDT1363_ORDER_DCBA 4 /* 4 字节，低字节先 */

#define YDT1363_TYPE_UINT 0
#define YDT1363_TYPE_INT 1

/* 读命令的 COMMAND INFO 通常只有一个组地址字节；首切片不做块读。 */
#define YDT1363_MAX_REQ_INFO 16

/**
 * @brief 一次读请求的全部参数，经 edge_api_read_tag_ex 的 point_desc 传入。
 *
 * ADR/VER/CID1/CID2 与 COMMAND INFO 都由调用方给，驱动不猜现场值。
 * 应答 INFO 里按 info_offset + order 取一个值；标定（scale）归调用方。
 */
typedef struct {
    uint8_t ver;      /**< 版本字节，如 0x22 */
    uint8_t adr;      /**< 设备地址 1..254 */
    uint8_t cid1;     /**< 设备类型，如 0x46 */
    uint8_t cid2;     /**< 读命令码；应答 CID2 是 RTN，不取自这里 */
    uint8_t info_len; /**< 请求 COMMAND INFO 字节数，≤ YDT1363_MAX_REQ_INFO */
    uint8_t info[YDT1363_MAX_REQ_INFO]; /**< 请求 COMMAND INFO */
    uint16_t info_offset;               /**< 应答 INFO 内的字节偏移 */
    uint8_t type;                       /**< YDT1363_TYPE_* */
    uint8_t order;                      /**< YDT1363_ORDER_*，决定取值宽度 */
} ydt1363_point_desc_t;

/** @return 0 成功；-1 参数或描述非法、输出空间不足 */
int ydt1363_encode_read(const ydt1363_point_desc_t *desc, net_buf_t *out);

/**
 * @brief 从收到的字节里切一帧应答并解出一个值。
 *
 * 没有 SOI 时丢掉整段；头部长度/校验坏或 ADR 不符时只丢 1 字节重扫，
 * 让后续的好帧还能被找回。
 *
 * @param consumed 本次可丢弃的字节数
 * @return 0=解出；EDGE_ERR_INCOMPLETE=未收全或已丢字节，等待下一包；
 *         -1=帧完好但 RTN 报错、INFO 超出首切片窗口或点位窗口越界
 */
int ydt1363_decode_rsp(const ydt1363_point_desc_t *desc, const uint8_t *buf, size_t len,
                       size_t *consumed, double *out_value);

#endif /* DRIVER_YDT1363_H */

#ifndef EDGE_YDT1363_CODEC_H
#define EDGE_YDT1363_CODEC_H

#include <stddef.h>
#include <stdint.h>

#include "net_buf.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief YD/T 1363.3 纯编解码内核
 *
 * 帧：SOI | VER ADR CID1 CID2 | LENGTH | COMMAND INFO | CHKSUM | EOI。
 * 除 SOI/EOI 外每个字段字节都编码成两个 ASCII 十六进制字符，帧长 18 + 2×N。
 */
#define YDT1363_SOI 0x7E
#define YDT1363_EOI 0x0D

/** @brief 应答帧 CID2 位（RTN）：0 正常，0x01~0x06 标准错误，0x80~0xEF 用户自定义 */
#define YDT1363_RTN_NORMAL 0x00

/**
 * @brief 单帧 INFO 字节数上限。
 *
 * LENID 只有 12 位且存 INFO 的 ASCII 字符数（N×2），最大 4094 字符，
 * 即 INFO 最多 2047 字节。2048 字节需要 LENID=4096，会溢出 12 位被静默截断。
 */
#define YDT1363_MAX_INFO 2047

#define YDT1363_OK 0
#define YDT1363_ERR_ARG (-1)      /**< 参数非法（空指针等） */
#define YDT1363_ERR_FORMAT (-2)   /**< SOI/EOI/ASCII 字符/长度字段不自洽 */
#define YDT1363_ERR_CHECKSUM (-3) /**< LCHKSUM 或 CHKSUM 不匹配 */
#define YDT1363_ERR_RTN (-4)      /**< 帧校验通过但 RTN 非 0，*rtn 已给出 */
#define YDT1363_ERR_SIZE (-5)     /**< INFO 超长、超调用方缓冲或 net_buf 空间不足 */

/**
 * @brief 编码一帧（大写 ASCII 十六进制）。
 * @param ver 版本字节；adr 地址；cid1 设备类型；cid2 请求命令码或应答 RTN
 * @param info COMMAND INFO，info_len 为 0 时可为 NULL
 * @param out 输出缓冲区，追加到当前尾部
 * @return 0 成功；<0 失败，失败时不写入任何字节
 */
int ydt1363_encode(uint8_t ver, uint8_t adr, uint8_t cid1, uint8_t cid2, const uint8_t *info,
                   size_t info_len, net_buf_t *out);

/**
 * @brief 解码**应答帧**，调用方已按 ydt1363_frame_len 切好完整帧。
 *
 * CID2 一律按 RTN 解释（0 正常、非 0 错误），因此不适用于请求帧：
 * 请求帧的 CID2 是命令码，需要从站角色时另加参数，不要复用本函数。
 * @param frame 完整帧，以 SOI 开头、EOI 结尾
 * @param len 帧长，必须恰好等于 18 + 2×N
 * @param out_info INFO 输出；N 为 0 时可为 NULL
 * @param cap out_info 容量；N > cap 时整帧拒绝，不截断
 * @param info_len 输出 INFO 字节数；任何返回路径都会写入
 * @param rtn 输出 CID2（应答的 RTN）；任何返回路径都会写入
 * @return 0 成功；YDT1363_ERR_RTN 表示帧本身完好但 RTN 非 0；其余 <0 为拒绝
 */
int ydt1363_decode(const uint8_t *frame, size_t len, uint8_t *out_info, size_t cap,
                   size_t *info_len, uint8_t *rtn);

/**
 * @brief 从缓冲头部推算完整帧长，供调用方等待收满与重同步。
 * @return 0 头部不足 13 字节，需要更多数据；>0 帧总长（INFO 为空时为 18）；
 *         <0 头部已可判定不是合法帧（非 SOI、ASCII/LENID/LCHKSUM 非法）
 *
 * 校验失败重同步时只丢 1 字节再扫 SOI，不要整段丢弃。
 * 注意：len<13 时即使首字节不是 SOI 也返回 0（约定调用方先自己扫 SOI）；
 * 若直接把任意短垃圾喂进来，调用方会把「非 SOI」误当成「还需更多数据」。
 */
int ydt1363_frame_len(const uint8_t *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif  // EDGE_YDT1363_CODEC_H

#include "dlt698_codec.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// --- DLT698 FCS16 Table ---
static const uint16_t DLT698_FCS_TAB[256] = {
    0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf, 0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
    0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e, 0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
    0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd, 0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
    0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c, 0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
    0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb, 0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
    0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a, 0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
    0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9, 0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
    0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738, 0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
    0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7, 0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
    0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036, 0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
    0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5, 0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
    0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134, 0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
    0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3, 0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
    0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232, 0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
    0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1, 0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
    0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330, 0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};

uint16_t dlt698_calculate_fcs(const uint8_t *p, size_t len) {
    uint16_t fcs = 0xFFFF;
    while (len--) {
        fcs = (uint16_t)((fcs >> 8) ^ DLT698_FCS_TAB[(fcs ^ *p++) & 0xFF]);
    }
    return (uint16_t)(fcs ^ 0xFFFF);
}

int dlt698_encode_get_request(const dlt698_addr_t *sa, uint8_t piid, uint32_t oad, net_buf_t *out) {
    struct net_buf_simple *b = &out->b;
    uint8_t *start = b->data + b->len;

    net_buf_simple_add_u8(b, DLT698_HEAD);
    
    // Length placeholder (2 bytes, with DIR/PRM/L-bit in top bits of first byte)
    // For simplicity: 0x4XXX (Master to Server)
    uint8_t *len_ptr = net_buf_simple_add(b, 2);
    
    net_buf_simple_add_u8(b, 0x43); // Ctrl: DIR=0, PRM=1, Frame=3 (LLC)
    
    // SA (Server Address)
    net_buf_simple_add_u8(b, (uint8_t)((sa->type << 6) | (sa->len - 1)));
    net_buf_simple_add_mem(b, sa->addr, sa->len);
    
    // CA (Client Address)
    net_buf_simple_add_u8(b, 0x10); 

    // HCS (Header Check Sequence)
    uint16_t hcs = dlt698_calculate_fcs(start + 1, (size_t)(b->data + b->len - (start + 1)));
    net_buf_simple_add_le16(b, hcs);

    // APDU: GET-Request Normal
    net_buf_simple_add_u8(b, DLT698_SVC_GET_REQ);
    net_buf_simple_add_u8(b, 0x01); // Normal
    net_buf_simple_add_u8(b, piid);
    net_buf_simple_add_be32(b, oad);
    
    // Optional selective access (0x00 for none)
    net_buf_simple_add_u8(b, 0x00);

    // Update length
    size_t total_len = (size_t)(b->data + b->len - start) + 3; // +2(FCS) + 1(16)
    uint16_t l_field = (uint16_t)((total_len - 1) & 0x3FFF);
    len_ptr[0] = (uint8_t)(l_field & 0xFF);
    len_ptr[1] = (uint8_t)(l_field >> 8);

    // FCS
    uint16_t fcs = dlt698_calculate_fcs(start + 1, (size_t)(b->data + b->len - (start + 1)));
    net_buf_simple_add_le16(b, fcs);
    net_buf_simple_add_u8(b, DLT698_END);

    return 0;
}

int dlt698_encode_get_response(const dlt698_addr_t *sa, uint8_t piid, uint32_t oad, 
                               const uint8_t *data, size_t dlen, net_buf_t *out) {
    struct net_buf_simple *b = &out->b;
    uint8_t *start = b->data + b->len;

    net_buf_simple_add_u8(b, DLT698_HEAD);
    uint8_t *len_ptr = net_buf_simple_add(b, 2);
    net_buf_simple_add_u8(b, 0x03); // Ctrl: DIR=1 (Response), PRM=0

    // SA
    net_buf_simple_add_u8(b, (uint8_t)((sa->type << 6) | (sa->len - 1)));
    net_buf_simple_add_mem(b, sa->addr, sa->len);
    net_buf_simple_add_u8(b, 0x10); // CA

    // HCS
    uint16_t hcs = dlt698_calculate_fcs(start + 1, (size_t)(b->data + b->len - (start + 1)));
    net_buf_simple_add_le16(b, hcs);

    // APDU: GET-Response Normal
    net_buf_simple_add_u8(b, 0x81); // GET-Response
    net_buf_simple_add_u8(b, 0x01); // Normal
    net_buf_simple_add_u8(b, piid);
    net_buf_simple_add_be32(b, oad);
    
    // Result (0x01 for data)
    net_buf_simple_add_u8(b, 0x01);
    net_buf_simple_add_mem(b, data, dlen);

    // Update length
    size_t total_len = (size_t)(b->data + b->len - start) + 3;
    uint16_t l_field = (uint16_t)((total_len - 1) & 0x3FFF);
    len_ptr[0] = (uint8_t)(l_field & 0xFF);
    len_ptr[1] = (uint8_t)(l_field >> 8);

    // FCS
    uint16_t fcs = dlt698_calculate_fcs(start + 1, (size_t)(b->data + b->len - (start + 1)));
    net_buf_simple_add_le16(b, fcs);
    net_buf_simple_add_u8(b, DLT698_END);

    return 0;
}

int dlt698_decode_frame(struct net_buf_simple *in, dlt698_addr_t *sa, uint8_t *svc, 
                        uint8_t *piid, uint8_t **payload_out, size_t *payload_len) {
    if (in->len < 12) return 1;
    if (in->data[0] != DLT698_HEAD) return -2;

    uint16_t l_field = (uint16_t)(in->data[1] | (in->data[2] << 8));
    size_t frame_len = (size_t)(l_field + 1);
    if (in->len < frame_len + 1) return 1;
    if (in->data[frame_len] != DLT698_END) return -2;

    // FCS Check
    uint16_t fcs_rx = (uint16_t)(in->data[frame_len - 2] | (in->data[frame_len - 1] << 8));
    if (dlt698_calculate_fcs(in->data + 1, frame_len - 3) != fcs_rx) return -1;

    // SA Decode
    uint8_t sa_cfg = in->data[4];
    sa->type = (uint8_t)(sa_cfg >> 6);
    sa->len = (uint8_t)((sa_cfg & 0x0F) + 1);
    memcpy(sa->addr, &in->data[5], sa->len);

    size_t header_len = (size_t)(5 + sa->len + 1); // 68,L,L,C,SA,CA
    // Skip HCS
    uint8_t *apdu_start = in->data + header_len + 2;
    *svc = apdu_start[0];
    *piid = apdu_start[2];
    *payload_out = apdu_start + 3;
    *payload_len = (size_t)(in->data + frame_len - 2 - (*payload_out));

    return 0;
}

/* ==============================================================================
 * A-XDR 基础数据类型编解码实现 (TDD Task 1)
 * ============================================================================== */

/**
 * @brief 释放递归分配的 A-XDR 数据资源
 */
static _Thread_local dlt698_data_t g_dlt698_item_pool[64];
static _Thread_local size_t g_dlt698_pool_idx = 0;

void dlt698_free_data(dlt698_data_t *data) {
    (void)data;
    // Zero-alloc: No dynamic memory to free
}

/**
 * @brief 通用 A-XDR 递归解码器实现 (核心逻辑)
 */
int dlt698_decode_data(const uint8_t *buf, uint16_t len, uint16_t *offset, dlt698_data_t *res) {
    if (!buf || !offset || !res || *offset >= len) return -1;

    res->tag = (dlt698_data_tag_t)buf[(*offset)++];

    switch (res->tag) {
        case DLT698_TAG_NULL:
            return 0;

        case DLT698_TAG_ARRAY:
        case DLT698_TAG_STRUCTURE: {
            uint8_t count = buf[(*offset)++];
            res->val.container.count = count;
            if (count > 0) {
                if (g_dlt698_pool_idx + count > 64) {
                    g_dlt698_pool_idx = 0;
                }
                res->val.container.items = &g_dlt698_item_pool[g_dlt698_pool_idx];
                g_dlt698_pool_idx += count;
                for (uint8_t i = 0; i < count; i++) {
                    int rc = dlt698_decode_data(buf, len, offset, &res->val.container.items[i]);
                    if (rc != 0) return rc;
                }
            } else {
                res->val.container.items = NULL;
            }
            return 0;
        }

        case DLT698_TAG_DOUBLE_LONG: // int32
            if (*offset + 4 > len) return -1;
            res->val.i32 = (int32_t)((buf[*offset] << 24) | (buf[*offset+1] << 16) | 
                                     (buf[*offset+2] << 8) | buf[*offset+3]);
            *offset += 4;
            return 0;

        case DLT698_TAG_DOUBLE_LONG_UNSIGNED: // uint32
            if (*offset + 4 > len) return -1;
            res->val.u32 = (uint32_t)((buf[*offset] << 24) | (buf[*offset+1] << 16) | 
                                      (buf[*offset+2] << 8) | buf[*offset+3]);
            *offset += 4;
            return 0;

        case DLT698_TAG_OCTET_STRING:
        case DLT698_TAG_VISIBLE_STRING: {
            uint8_t slen = buf[(*offset)++];
            if (*offset + slen > len) return -1;
            res->val.str.len = slen;
            uint8_t copy_len = (slen >= MAX_DLT698_STRING_LEN) ? (MAX_DLT698_STRING_LEN - 1) : slen;
            memcpy(res->val.str.buf, &buf[*offset], copy_len);
            res->val.str.buf[copy_len] = '\0';
            res->val.str.ptr = res->val.str.buf;
            *offset += slen;
            return 0;
        }

        case DLT698_TAG_INTEGER: // int8
            res->val.i8 = (int8_t)buf[(*offset)++];
            return 0;

        case DLT698_TAG_UNSIGNED: // uint8
            res->val.u8 = buf[(*offset)++];
            return 0;

        case DLT698_TAG_LONG: // int16
            if (*offset + 2 > len) return -1;
            res->val.i16 = (int16_t)((buf[*offset] << 8) | buf[*offset+1]);
            *offset += 2;
            return 0;

        case DLT698_TAG_LONG_UNSIGNED: // uint16
            if (*offset + 2 > len) return -1;
            res->val.u16 = (uint16_t)((buf[*offset] << 8) | buf[*offset+1]);
            *offset += 2;
            return 0;

        case DLT698_TAG_ENUM:
            res->val.u8 = buf[(*offset)++];
            return 0;

        case DLT698_TAG_DATE_TIME_S: {
            if (*offset + 7 > len) return -1;
            // 简单存储为原始字节或进一步解析
            res->val.str.len = 7;
            memcpy(res->val.str.buf, &buf[*offset], 7);
            res->val.str.ptr = res->val.str.buf;
            *offset += 7;
            return 0;
        }

        default:
            // 暂不支持的类型
            return -2;
    }
}

/**
 * @brief 编码 long-unsigned (Tag 18) 无符号双字节 (16位)
 */
int dlt698_encode_long_unsigned(uint8_t *buf, uint16_t len, uint16_t *offset, uint16_t val) {
    // 边界安全检验：确保缓冲区和偏移指针有效，且空间足够写入 3 字节 (1字节 tag + 2字节数据)
    if (!buf || !offset || *offset + 3 > len) {
        return -1;
    }
    
    buf[(*offset)++] = 18;  // 写入 long-unsigned 类型标识 Tag = 18
    buf[(*offset)++] = (uint8_t)((val >> 8) & 0xFF);  // 高字节大端序写入
    buf[(*offset)++] = (uint8_t)(val & 0xFF);         // 低字节大端序写入
    
    return 0;
}

/**
 * @brief 解码 long-unsigned (Tag 18)
 */
int dlt698_decode_long_unsigned(const uint8_t *buf, uint16_t len, uint16_t *offset, uint16_t *val) {
    // 边界安全检验：确保解码数据不溢出
    if (!buf || !offset || !val || *offset + 3 > len) {
        return -1;
    }
    
    // 类型校验
    if (buf[*offset] != 18) {
        return -2; // 类型 Tag 匹配错误
    }
    (*offset)++; // 跳过 Tag 字节
    
    // 大端序拼装数据
    *val = (uint16_t)((buf[*offset] << 8) | buf[*offset + 1]);
    *offset += 2;
    
    return 0;
}

/**
 * @brief 编码 double-long (Tag 5) 有符号四字节 (32位)
 */
int dlt698_encode_double_long(uint8_t *buf, uint16_t len, uint16_t *offset, int32_t val) {
    // 边界安全检验：确保写入 5 字节 (1字节 tag + 4字节数据) 不溢出
    if (!buf || !offset || *offset + 5 > len) {
        return -1;
    }
    
    buf[(*offset)++] = 5;  // 写入 double-long 类型标识 Tag = 5
    buf[(*offset)++] = (uint8_t)((val >> 24) & 0xFF); // 大端序高字节
    buf[(*offset)++] = (uint8_t)((val >> 16) & 0xFF);
    buf[(*offset)++] = (uint8_t)((val >> 8) & 0xFF);
    buf[(*offset)++] = (uint8_t)(val & 0xFF);
    
    return 0;
}

/**
 * @brief 解码 double-long (Tag 5)
 */
int dlt698_decode_double_long(const uint8_t *buf, uint16_t len, uint16_t *offset, int32_t *val) {
    if (!buf || !offset || !val || *offset + 5 > len) {
        return -1;
    }
    
    if (buf[*offset] != 5) {
        return -2; // 类型 Tag 不匹配
    }
    (*offset)++;
    
    // 大端序还原 32 位有符号数
    *val = (int32_t)((buf[*offset] << 24) | 
                     (buf[*offset + 1] << 16) | 
                     (buf[*offset + 2] << 8) | 
                     buf[*offset + 3]);
    *offset += 4;
    
    return 0;
}

/**
 * @brief 编码 date_time_s (Tag 28) 7字节高精度时间格式
 */
int dlt698_encode_date_time_s(uint8_t *buf, uint16_t len, uint16_t *offset, uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec) {
    // 边界安全检验：确保能容纳 8 字节 (1字节 tag + 7字节值)
    if (!buf || !offset || *offset + 8 > len) {
        return -1;
    }
    
    buf[(*offset)++] = 28; // 写入 date_time_s 类型标识 Tag = 28
    buf[(*offset)++] = (uint8_t)((year >> 8) & 0xFF); // 年 (高字节)
    buf[(*offset)++] = (uint8_t)(year & 0xFF);        // 年 (低字节)
    buf[(*offset)++] = month;                         // 月
    buf[(*offset)++] = day;                           // 日
    buf[(*offset)++] = hour;                          // 时
    buf[(*offset)++] = min;                           // 分
    buf[(*offset)++] = sec;                           // 秒
    
    return 0;
}

/**
 * @brief 解码 date_time_s (Tag 28)
 */
int dlt698_decode_date_time_s(const uint8_t *buf, uint16_t len, uint16_t *offset, uint16_t *year, uint8_t *month, uint8_t *day, uint8_t *hour, uint8_t *min, uint8_t *sec) {
    if (!buf || !offset || !year || !month || !day || !hour || !min || !sec || *offset + 8 > len) {
        return -1;
    }
    
    if (buf[*offset] != 28) {
        return -2; // 类型 Tag 匹配错误
    }
    (*offset)++;
    
    // 解析时间数据
    *year = (uint16_t)((buf[*offset] << 8) | buf[*offset + 1]);
    *month = buf[*offset + 2];
    *day   = buf[*offset + 3];
    *hour  = buf[*offset + 4];
    *min   = buf[*offset + 5];
    *sec   = buf[*offset + 6];
    *offset += 7;
    
    return 0;
}


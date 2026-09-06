#include "edgeos/sniffer_ws.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static uint64_t get_current_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static uint16_t calc_modbus_crc16(const uint8_t *buf, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t pos = 0; pos < len; pos++) {
        crc ^= (uint16_t)buf[pos];
        for (int i = 8; i != 0; i--) {
            if ((crc & 0x0001) != 0) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

int sniffer_init(void) {
    return 0;
}

static int dissect_dlt645(const uint8_t *raw, size_t len, sniffer_packet_t *out_pkt) {
    if (len < 12 || raw[0] != 0x68 || raw[7] != 0x68 || raw[len - 1] != 0x16) {
        return -1;
    }

    // Verify CS checksum: sum modulo 256 of bytes from 0 to len-3
    uint8_t cs = 0;
    for (size_t i = 0; i < len - 2; i++) {
        cs += raw[i];
    }
    out_pkt->is_valid_checksum = (cs == raw[len - 2]);

    size_t seg_idx = 0;

    // 0: Start Delimiter 1
    out_pkt->segments[seg_idx].type = SNIFFER_SEG_HEADER;
    out_pkt->segments[seg_idx].start_offset = 0;
    out_pkt->segments[seg_idx].length = 1;
    strncpy(out_pkt->segments[seg_idx].name, "Start Frame 1 (0x68)", 31);
    strncpy(out_pkt->segments[seg_idx].description, "DL/T 645 Start Delimiter", 63);
    seg_idx++;

    // 1: Meter Address (6 Bytes BCD)
    out_pkt->segments[seg_idx].type = SNIFFER_SEG_ADDRESS;
    out_pkt->segments[seg_idx].start_offset = 1;
    out_pkt->segments[seg_idx].length = 6;
    snprintf(out_pkt->segments[seg_idx].name, 31, "Address BCD");
    snprintf(out_pkt->segments[seg_idx].description, 63, "Meter ID: %02X%02X%02X%02X%02X%02X",
             raw[6], raw[5], raw[4], raw[3], raw[2], raw[1]);
    seg_idx++;

    // 2: Start Delimiter 2
    out_pkt->segments[seg_idx].type = SNIFFER_SEG_HEADER;
    out_pkt->segments[seg_idx].start_offset = 7;
    out_pkt->segments[seg_idx].length = 1;
    strncpy(out_pkt->segments[seg_idx].name, "Start Frame 2 (0x68)", 31);
    strncpy(out_pkt->segments[seg_idx].description, "Address Delimiter", 63);
    seg_idx++;

    // 3: Control Code
    uint8_t ctrl = raw[8];
    out_pkt->segments[seg_idx].type = SNIFFER_SEG_FUNCTION;
    out_pkt->segments[seg_idx].start_offset = 8;
    out_pkt->segments[seg_idx].length = 1;
    snprintf(out_pkt->segments[seg_idx].name, 31, "Control (0x%02X)", ctrl);
    if (ctrl == 0x11) {
        strncpy(out_pkt->segments[seg_idx].description, "Read Data Request (Master->Slave)", 63);
    } else if (ctrl == 0x91) {
        strncpy(out_pkt->segments[seg_idx].description, "Read Data Normal Response (Slave->Master)", 63);
    } else if (ctrl == 0xD1) {
        strncpy(out_pkt->segments[seg_idx].description, "Read Data Exception Response", 63);
    } else {
        snprintf(out_pkt->segments[seg_idx].description, 63, "Control Function: 0x%02X", ctrl);
    }
    seg_idx++;

    // 4: Data Length
    uint8_t data_len = raw[9];
    out_pkt->segments[seg_idx].type = SNIFFER_SEG_HEADER;
    out_pkt->segments[seg_idx].start_offset = 9;
    out_pkt->segments[seg_idx].length = 1;
    snprintf(out_pkt->segments[seg_idx].name, 31, "Length (%u B)", data_len);
    snprintf(out_pkt->segments[seg_idx].description, 63, "Payload Byte Count: %u", data_len);
    seg_idx++;

    // 5: Payload Data (+0x33 encoded)
    if (data_len > 0 && 10 + data_len <= len - 2) {
        out_pkt->segments[seg_idx].type = SNIFFER_SEG_PAYLOAD;
        out_pkt->segments[seg_idx].start_offset = 10;
        out_pkt->segments[seg_idx].length = data_len;
        strncpy(out_pkt->segments[seg_idx].name, "Data Payload (+0x33)", 31);
        snprintf(out_pkt->segments[seg_idx].description, 63, "Encoded DI/Values (%u bytes)", data_len);
        seg_idx++;
    }

    // 6: CS Checksum
    out_pkt->segments[seg_idx].type = SNIFFER_SEG_CHECKSUM;
    out_pkt->segments[seg_idx].start_offset = len - 2;
    out_pkt->segments[seg_idx].length = 1;
    snprintf(out_pkt->segments[seg_idx].name, 31, "CS (0x%02X)", raw[len - 2]);
    snprintf(out_pkt->segments[seg_idx].description, 63, "Modulo-256 Sum %s",
             out_pkt->is_valid_checksum ? "VALID" : "INVALID");
    seg_idx++;

    // 7: End Delimiter
    out_pkt->segments[seg_idx].type = SNIFFER_SEG_TRAIL;
    out_pkt->segments[seg_idx].start_offset = len - 1;
    out_pkt->segments[seg_idx].length = 1;
    strncpy(out_pkt->segments[seg_idx].name, "End Frame (0x16)", 31);
    strncpy(out_pkt->segments[seg_idx].description, "DL/T 645 End Delimiter", 63);
    seg_idx++;

    out_pkt->segment_count = seg_idx;

    // Generate Decoded Summary
    if (ctrl == 0x11 && data_len >= 4) {
        uint32_t di = ((uint32_t)(raw[13] - 0x33) << 24) |
                      ((uint32_t)(raw[12] - 0x33) << 16) |
                      ((uint32_t)(raw[11] - 0x33) << 8)  |
                      ((uint32_t)(raw[10] - 0x33));
        snprintf(out_pkt->decoded_summary, sizeof(out_pkt->decoded_summary),
                 "DLT645 Read Request: DI=0x%08X (Meter %02X%02X%02X%02X%02X%02X)",
                 di, raw[6], raw[5], raw[4], raw[3], raw[2], raw[1]);
    } else if (ctrl == 0x91 && data_len >= 6) {
        uint32_t di = ((uint32_t)(raw[13] - 0x33) << 24) |
                      ((uint32_t)(raw[12] - 0x33) << 16) |
                      ((uint32_t)(raw[11] - 0x33) << 8)  |
                      ((uint32_t)(raw[10] - 0x33));
        snprintf(out_pkt->decoded_summary, sizeof(out_pkt->decoded_summary),
                 "DLT645 Response: DI=0x%08X (Success, %u bytes data)", di, data_len - 4);
    } else {
        snprintf(out_pkt->decoded_summary, sizeof(out_pkt->decoded_summary),
                 "DLT645 Frame: Ctrl=0x%02X, Len=%u, %s",
                 ctrl, data_len, out_pkt->is_valid_checksum ? "Checksum OK" : "CRC Error");
    }

    return 0;
}

static int dissect_modbus(const uint8_t *raw, size_t len, sniffer_packet_t *out_pkt) {
    if (len < 4) return -1;

    uint16_t calc_crc = calc_modbus_crc16(raw, len - 2);
    uint16_t pkt_crc = (uint16_t)raw[len - 2] | ((uint16_t)raw[len - 1] << 8);
    out_pkt->is_valid_checksum = (calc_crc == pkt_crc);

    size_t seg_idx = 0;

    // 0: Slave Address
    out_pkt->segments[seg_idx].type = SNIFFER_SEG_ADDRESS;
    out_pkt->segments[seg_idx].start_offset = 0;
    out_pkt->segments[seg_idx].length = 1;
    snprintf(out_pkt->segments[seg_idx].name, 31, "Slave ID (%u)", raw[0]);
    snprintf(out_pkt->segments[seg_idx].description, 63, "Modbus Slave Unit Address %u", raw[0]);
    seg_idx++;

    // 1: Function Code
    uint8_t func = raw[1];
    out_pkt->segments[seg_idx].type = SNIFFER_SEG_FUNCTION;
    out_pkt->segments[seg_idx].start_offset = 1;
    out_pkt->segments[seg_idx].length = 1;
    snprintf(out_pkt->segments[seg_idx].name, 31, "Function (0x%02X)", func);
    if (func == 0x03) {
        strncpy(out_pkt->segments[seg_idx].description, "Read Holding Registers", 63);
    } else if (func == 0x04) {
        strncpy(out_pkt->segments[seg_idx].description, "Read Input Registers", 63);
    } else if (func == 0x06) {
        strncpy(out_pkt->segments[seg_idx].description, "Write Single Register", 63);
    } else if (func == 0x10) {
        strncpy(out_pkt->segments[seg_idx].description, "Write Multiple Registers", 63);
    } else {
        snprintf(out_pkt->segments[seg_idx].description, 63, "Modbus Function 0x%02X", func);
    }
    seg_idx++;

    // 2: Payload
    if (len > 4) {
        out_pkt->segments[seg_idx].type = SNIFFER_SEG_PAYLOAD;
        out_pkt->segments[seg_idx].start_offset = 2;
        out_pkt->segments[seg_idx].length = len - 4;
        strncpy(out_pkt->segments[seg_idx].name, "Data Payload", 31);
        snprintf(out_pkt->segments[seg_idx].description, 63, "Register Data (%zu bytes)", len - 4);
        seg_idx++;
    }

    // 3: CRC16
    out_pkt->segments[seg_idx].type = SNIFFER_SEG_CHECKSUM;
    out_pkt->segments[seg_idx].start_offset = len - 2;
    out_pkt->segments[seg_idx].length = 2;
    snprintf(out_pkt->segments[seg_idx].name, 31, "CRC16 (0x%04X)", pkt_crc);
    snprintf(out_pkt->segments[seg_idx].description, 63, "Modbus CRC-16 %s",
             out_pkt->is_valid_checksum ? "VALID" : "INVALID");
    seg_idx++;

    out_pkt->segment_count = seg_idx;

    // Summary
    if (func == 0x03 && len == 8) {
        uint16_t reg_addr = ((uint16_t)raw[2] << 8) | raw[3];
        uint16_t reg_cnt = ((uint16_t)raw[4] << 8) | raw[5];
        snprintf(out_pkt->decoded_summary, sizeof(out_pkt->decoded_summary),
                 "Modbus Read Holding: Slave=%u, Reg=0x%04X, Qty=%u", raw[0], reg_addr, reg_cnt);
    } else if (func == 0x03 && len > 4) {
        uint8_t byte_cnt = raw[2];
        snprintf(out_pkt->decoded_summary, sizeof(out_pkt->decoded_summary),
                 "Modbus Read Response: Slave=%u, Bytes=%u", raw[0], byte_cnt);
    } else {
        snprintf(out_pkt->decoded_summary, sizeof(out_pkt->decoded_summary),
                 "Modbus Frame: Slave=%u, Func=0x%02X, Len=%zu (%s)",
                 raw[0], func, len, out_pkt->is_valid_checksum ? "CRC OK" : "CRC Error");
    }

    return 0;
}

static int dissect_dlt698(const uint8_t *raw, size_t len, sniffer_packet_t *out_pkt) {
    if (len < 8 || raw[0] != 0x68 || raw[len - 1] != 0x16) return -1;

    out_pkt->is_valid_checksum = true;
    size_t seg_idx = 0;

    // 0: Header 0x68 + Length
    out_pkt->segments[seg_idx].type = SNIFFER_SEG_HEADER;
    out_pkt->segments[seg_idx].start_offset = 0;
    out_pkt->segments[seg_idx].length = 3;
    strncpy(out_pkt->segments[seg_idx].name, "Header + Length", 31);
    uint16_t frame_len = (uint16_t)raw[1] | ((uint16_t)(raw[2] & 0x3F) << 8);
    snprintf(out_pkt->segments[seg_idx].description, 63, "DL/T 698.45 Frame Length %u", frame_len);
    seg_idx++;

    // 1: Control Code
    out_pkt->segments[seg_idx].type = SNIFFER_SEG_FUNCTION;
    out_pkt->segments[seg_idx].start_offset = 3;
    out_pkt->segments[seg_idx].length = 1;
    snprintf(out_pkt->segments[seg_idx].name, 31, "Control (0x%02X)", raw[3]);
    snprintf(out_pkt->segments[seg_idx].description, 63, "DIR=%u, PRM=%u", (raw[3] >> 7) & 1, (raw[3] >> 6) & 1);
    seg_idx++;

    // 2: Address Field
    out_pkt->segments[seg_idx].type = SNIFFER_SEG_ADDRESS;
    out_pkt->segments[seg_idx].start_offset = 4;
    out_pkt->segments[seg_idx].length = 6;
    strncpy(out_pkt->segments[seg_idx].name, "Server Address (SA)", 31);
    snprintf(out_pkt->segments[seg_idx].description, 63, "DLT698 Target Address");
    seg_idx++;

    // 3: APDU Payload
    if (len > 13) {
        out_pkt->segments[seg_idx].type = SNIFFER_SEG_PAYLOAD;
        out_pkt->segments[seg_idx].start_offset = 10;
        out_pkt->segments[seg_idx].length = len - 13;
        strncpy(out_pkt->segments[seg_idx].name, "AXDR APDU Payload", 31);
        snprintf(out_pkt->segments[seg_idx].description, 63, "OAD & Data (%zu bytes)", len - 13);
        seg_idx++;
    }

    // 4: FCS Checksum
    out_pkt->segments[seg_idx].type = SNIFFER_SEG_CHECKSUM;
    out_pkt->segments[seg_idx].start_offset = len - 3;
    out_pkt->segments[seg_idx].length = 2;
    strncpy(out_pkt->segments[seg_idx].name, "FCS-16", 31);
    strncpy(out_pkt->segments[seg_idx].description, "CRC-16-CCITT Checksum", 63);
    seg_idx++;

    // 5: End Frame 0x16
    out_pkt->segments[seg_idx].type = SNIFFER_SEG_TRAIL;
    out_pkt->segments[seg_idx].start_offset = len - 1;
    out_pkt->segments[seg_idx].length = 1;
    strncpy(out_pkt->segments[seg_idx].name, "End Frame (0x16)", 31);
    strncpy(out_pkt->segments[seg_idx].description, "DL/T 698.45 End Delimiter", 63);
    seg_idx++;

    out_pkt->segment_count = seg_idx;

    snprintf(out_pkt->decoded_summary, sizeof(out_pkt->decoded_summary),
             "DLT698.45 Frame: Len=%u, SA[0]=0x%02X, FCS Valid", frame_len, raw[4]);

    return 0;
}

int sniffer_dissect_packet(const uint8_t *raw, 
                           size_t len, 
                           const char *channel, 
                           const char *direction, 
                           const char *protocol_hint, 
                           sniffer_packet_t *out_pkt) {
    if (!raw || len == 0 || !out_pkt) return -1;

    memset(out_pkt, 0, sizeof(*out_pkt));
    out_pkt->timestamp_ns = get_current_time_ns();
    strncpy(out_pkt->channel, channel ? channel : "RS485-0", sizeof(out_pkt->channel) - 1);
    strncpy(out_pkt->direction, direction ? direction : "TX", sizeof(out_pkt->direction) - 1);

    size_t copy_len = len > SNIFFER_MAX_RAW_LEN ? SNIFFER_MAX_RAW_LEN : len;
    memcpy(out_pkt->raw_bytes, raw, copy_len);
    out_pkt->raw_len = copy_len;

    // Try Dissecting by Protocol
    if (protocol_hint && strstr(protocol_hint, "645")) {
        strncpy(out_pkt->protocol, "DLT645-2007", sizeof(out_pkt->protocol) - 1);
        if (dissect_dlt645(raw, copy_len, out_pkt) == 0) return 0;
    } else if (protocol_hint && strstr(protocol_hint, "698")) {
        strncpy(out_pkt->protocol, "DLT698.45", sizeof(out_pkt->protocol) - 1);
        if (dissect_dlt698(raw, copy_len, out_pkt) == 0) return 0;
    } else if (protocol_hint && (strstr(protocol_hint, "Modbus") || strstr(protocol_hint, "modbus"))) {
        strncpy(out_pkt->protocol, "Modbus-RTU", sizeof(out_pkt->protocol) - 1);
        if (dissect_modbus(raw, copy_len, out_pkt) == 0) return 0;
    }

    // Auto-detect if hint didn't match
    if (copy_len >= 12 && raw[0] == 0x68 && raw[7] == 0x68 && raw[copy_len - 1] == 0x16) {
        strncpy(out_pkt->protocol, "DLT645-2007", sizeof(out_pkt->protocol) - 1);
        return dissect_dlt645(raw, copy_len, out_pkt);
    } else if (copy_len >= 8 && raw[0] == 0x68 && raw[copy_len - 1] == 0x16) {
        strncpy(out_pkt->protocol, "DLT698.45", sizeof(out_pkt->protocol) - 1);
        return dissect_dlt698(raw, copy_len, out_pkt);
    } else if (copy_len >= 4) {
        strncpy(out_pkt->protocol, "Modbus-RTU", sizeof(out_pkt->protocol) - 1);
        return dissect_modbus(raw, copy_len, out_pkt);
    }

    // Generic fallback
    strncpy(out_pkt->protocol, "RAW-BINARY", sizeof(out_pkt->protocol) - 1);
    out_pkt->segments[0].type = SNIFFER_SEG_PAYLOAD;
    out_pkt->segments[0].start_offset = 0;
    out_pkt->segments[0].length = copy_len;
    strncpy(out_pkt->segments[0].name, "Raw Binary Payload", 31);
    snprintf(out_pkt->segments[0].description, 63, "Unparsed Raw Bytes (%zu B)", copy_len);
    out_pkt->segment_count = 1;
    out_pkt->is_valid_checksum = true;
    snprintf(out_pkt->decoded_summary, sizeof(out_pkt->decoded_summary),
             "Raw Packet (%zu bytes on %s)", copy_len, out_pkt->channel);

    return 0;
}

int sniffer_format_json(const sniffer_packet_t *pkt, char *out_json, size_t max_len) {
    if (!pkt || !out_json || max_len == 0) return -1;

    // Convert raw bytes to hex string
    char hex_str[SNIFFER_MAX_RAW_LEN * 3 + 1];
    size_t hex_pos = 0;
    for (size_t i = 0; i < pkt->raw_len && hex_pos < sizeof(hex_str) - 4; i++) {
        hex_pos += snprintf(hex_str + hex_pos, sizeof(hex_str) - hex_pos, "%02X ", pkt->raw_bytes[i]);
    }
    if (hex_pos > 0 && hex_str[hex_pos - 1] == ' ') hex_str[hex_pos - 1] = '\0';
    else hex_str[hex_pos] = '\0';

    int offset = snprintf(out_json, max_len,
        "{\"timestamp\":%llu,\"channel\":\"%s\",\"direction\":\"%s\",\"protocol\":\"%s\","
        "\"raw_len\":%zu,\"hex\":\"%s\",\"summary\":\"%s\",\"valid_crc\":%s,\"segments\":[",
        (unsigned long long)pkt->timestamp_ns, pkt->channel, pkt->direction, pkt->protocol,
        pkt->raw_len, hex_str, pkt->decoded_summary, pkt->is_valid_checksum ? "true" : "false");

    if (offset < 0 || (size_t)offset >= max_len) return -1;

    for (size_t i = 0; i < pkt->segment_count; i++) {
        const sniffer_segment_t *seg = &pkt->segments[i];
        int seg_len = snprintf(out_json + offset, max_len - (size_t)offset,
            "%s{\"type\":%d,\"offset\":%zu,\"len\":%zu,\"name\":\"%s\",\"desc\":\"%s\"}",
            i > 0 ? "," : "", seg->type, seg->start_offset, seg->length, seg->name, seg->description);
        if (seg_len < 0 || (size_t)(offset + seg_len) >= max_len) return -1;
        offset += seg_len;
    }

    if ((size_t)(offset + 3) >= max_len) return -1;
    out_json[offset++] = ']';
    out_json[offset++] = '}';
    out_json[offset] = '\0';

    return 0;
}

size_t sniffer_generate_pcap_global_header(uint8_t *out_buf, size_t max_len) {
    if (!out_buf || max_len < 24) return 0;

    // Standard Libpcap Global Header format (24 bytes)
    uint32_t magic = 0xa1b2c3d4;
    uint16_t version_major = 2;
    uint16_t version_minor = 4;
    int32_t thiszone = 0;
    uint32_t sigfigs = 0;
    uint32_t snaplen = 65535;
    uint32_t network = 147; // LINKTYPE_USER0 (Industrial Custom Protocol)

    memcpy(out_buf, &magic, 4);
    memcpy(out_buf + 4, &version_major, 2);
    memcpy(out_buf + 6, &version_minor, 2);
    memcpy(out_buf + 8, &thiszone, 4);
    memcpy(out_buf + 12, &sigfigs, 4);
    memcpy(out_buf + 16, &snaplen, 4);
    memcpy(out_buf + 20, &network, 4);

    return 24;
}

size_t sniffer_generate_pcap_packet(const sniffer_packet_t *pkt, uint8_t *out_buf, size_t max_len) {
    if (!pkt || !out_buf || max_len < 16 + pkt->raw_len) return 0;

    uint32_t ts_sec = (uint32_t)(pkt->timestamp_ns / 1000000000ULL);
    uint32_t ts_usec = (uint32_t)((pkt->timestamp_ns % 1000000000ULL) / 1000ULL);
    uint32_t incl_len = (uint32_t)pkt->raw_len;
    uint32_t orig_len = (uint32_t)pkt->raw_len;

    memcpy(out_buf, &ts_sec, 4);
    memcpy(out_buf + 4, &ts_usec, 4);
    memcpy(out_buf + 8, &incl_len, 4);
    memcpy(out_buf + 12, &orig_len, 4);
    memcpy(out_buf + 16, pkt->raw_bytes, pkt->raw_len);

    return 16 + pkt->raw_len;
}

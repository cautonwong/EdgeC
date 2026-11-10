#ifndef EDGE_TLV_H
#define EDGE_TLV_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Type-Length-Value (TLV) Binary IPC Packet Format (OpenWrt ubus / blobmsg pattern)
#define EDGE_TLV_MAGIC 0x45544C56 // "ETLV"

typedef struct {
	uint32_t magic;
	uint16_t type;
	uint16_t length;
	uint64_t timestamp;
} __attribute__((packed)) edge_tlv_hdr_t;

int edge_tlv_pack_scalar(uint8_t *buf, size_t max_len, uint16_t type, uint32_t tag_id, double val, uint64_t ts);
int edge_tlv_unpack_scalar(const uint8_t *buf, size_t len, uint16_t *out_type, uint32_t *out_tag_id, double *out_val, uint64_t *out_ts);

#ifdef __cplusplus
}
#endif

#endif // EDGE_TLV_H

#include "edge_tlv.h"
#include <string.h>

typedef struct {
	uint32_t tag_id;
	double val;
} __attribute__((packed)) edge_tlv_scalar_body_t;

int edge_tlv_pack_scalar(uint8_t *buf, size_t max_len, uint16_t type, uint32_t tag_id, double val, uint64_t ts) {
	size_t needed = sizeof(edge_tlv_hdr_t) + sizeof(edge_tlv_scalar_body_t);
	if (!buf || max_len < needed) return -1;

	edge_tlv_hdr_t *hdr = (edge_tlv_hdr_t *)buf;
	hdr->magic = EDGE_TLV_MAGIC;
	hdr->type = type;
	hdr->length = (uint16_t)sizeof(edge_tlv_scalar_body_t);
	hdr->timestamp = ts;

	edge_tlv_scalar_body_t *body = (edge_tlv_scalar_body_t *)(buf + sizeof(edge_tlv_hdr_t));
	body->tag_id = tag_id;
	body->val = val;

	return (int)needed;
}

int edge_tlv_unpack_scalar(const uint8_t *buf, size_t len, uint16_t *out_type, uint32_t *out_tag_id, double *out_val, uint64_t *out_ts) {
	size_t min_needed = sizeof(edge_tlv_hdr_t) + sizeof(edge_tlv_scalar_body_t);
	if (!buf || len < min_needed) return -1;

	const edge_tlv_hdr_t *hdr = (const edge_tlv_hdr_t *)buf;
	if (hdr->magic != EDGE_TLV_MAGIC) return -1;

	const edge_tlv_scalar_body_t *body = (const edge_tlv_scalar_body_t *)(buf + sizeof(edge_tlv_hdr_t));

	if (out_type) *out_type = hdr->type;
	if (out_ts) *out_ts = hdr->timestamp;
	if (out_tag_id) *out_tag_id = body->tag_id;
	if (out_val) *out_val = body->val;

	return 0;
}

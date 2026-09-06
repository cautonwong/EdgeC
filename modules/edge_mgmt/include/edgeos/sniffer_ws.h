#ifndef EDGEOS_SNIFFER_WS_H
#define EDGEOS_SNIFFER_WS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SNIFFER_MAX_RAW_LEN       1024
#define SNIFFER_MAX_SEGS          16
#define SNIFFER_MAX_SUMMARY_LEN   256
#define SNIFFER_MAX_JSON_LEN      4096

typedef enum {
    SNIFFER_SEG_HEADER = 0,
    SNIFFER_SEG_ADDRESS,
    SNIFFER_SEG_FUNCTION,
    SNIFFER_SEG_PAYLOAD,
    SNIFFER_SEG_CHECKSUM,
    SNIFFER_SEG_TRAIL
} sniffer_segment_type_t;

typedef struct {
    sniffer_segment_type_t type;
    size_t start_offset;
    size_t length;
    char name[32];
    char description[64];
} sniffer_segment_t;

typedef struct {
    uint64_t timestamp_ns;                  /* MTT timestamp */
    char channel[32];                       /* RS485-0 / COM1, HPLC, TCP, etc. */
    char direction[8];                      /* "TX" or "RX" */
    char protocol[32];                     /* DLT645-2007, DLT698.45, Modbus-RTU, DDS-RTPS */
    uint8_t raw_bytes[SNIFFER_MAX_RAW_LEN];
    size_t raw_len;
    sniffer_segment_t segments[SNIFFER_MAX_SEGS];
    size_t segment_count;
    char decoded_summary[SNIFFER_MAX_SUMMARY_LEN];
    bool is_valid_checksum;
} sniffer_packet_t;

/**
 * Initialize protocol sniffer subsystem
 */
int sniffer_init(void);

/**
 * Dissect raw frame bytes into segmented protocol packet structure
 */
int sniffer_dissect_packet(const uint8_t *raw, 
                           size_t len, 
                           const char *channel, 
                           const char *direction, 
                           const char *protocol_hint, 
                           sniffer_packet_t *out_pkt);

/**
 * Serialize a dissected packet into JSON envelope for WebSocket/REST transfer
 */
int sniffer_format_json(const sniffer_packet_t *pkt, char *out_json, size_t max_len);

/**
 * Generate standard Libpcap Global Header (24 bytes)
 */
size_t sniffer_generate_pcap_global_header(uint8_t *out_buf, size_t max_len);

/**
 * Generate standard Libpcap Packet Header (16 bytes) + Packet Data
 */
size_t sniffer_generate_pcap_packet(const sniffer_packet_t *pkt, uint8_t *out_buf, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif /* EDGEOS_SNIFFER_WS_H */

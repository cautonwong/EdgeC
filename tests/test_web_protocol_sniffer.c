/**
 * test_web_protocol_sniffer.c - Verification of Protocol Sniffer & Hex Inspector (AEP-503)
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <cmocka.h>
#include "edgeos/sniffer_ws.h"

static void read_file_content(const char *rel_path, char *buf, size_t max_len) {
    char path[512];
    FILE *fp = NULL;

    const char *prefixes[] = {
        "",
        "../",
        "../../",
        "/workspaces/edgeos/EdgeC/",
        NULL
    };

    for (int i = 0; prefixes[i] != NULL; i++) {
        snprintf(path, sizeof(path), "%s%s", prefixes[i], rel_path);
        fp = fopen(path, "r");
        if (fp) break;
    }

    assert_non_null(fp);
    size_t n = fread(buf, 1, max_len - 1, fp);
    buf[n] = '\0';
    fclose(fp);
}

static void test_web_sniffer_vue_components_structure(void **state) {
    (void)state;

    // 1. Verify ProtocolSnifferView.vue
    char sniffer_content[65536];
    read_file_content("web/core/src/components/console/ProtocolSnifferView.vue", sniffer_content, sizeof(sniffer_content));

    assert_non_null(strstr(sniffer_content, "Live Sniffer (50Hz)"));
    assert_non_null(strstr(sniffer_content, "exportJSON"));
    assert_non_null(strstr(sniffer_content, "exportPCAP"));
    assert_non_null(strstr(sniffer_content, "selectedChannel"));
    assert_non_null(strstr(sniffer_content, "selectedProtocol"));
    assert_non_null(strstr(sniffer_content, "selectedDirection"));
    assert_non_null(strstr(sniffer_content, "searchKeyword"));
    assert_non_null(strstr(sniffer_content, "HexPayloadViewer"));
    assert_non_null(strstr(sniffer_content, "0xa1b2c3d4")); // PCAP Magic Number

    // 2. Verify HexPayloadViewer.vue
    char hex_content[65536];
    read_file_content("web/core/src/components/console/HexPayloadViewer.vue", hex_content, sizeof(hex_content));

    assert_non_null(strstr(hex_content, "Hex / ASCII Inspector"));
    assert_non_null(strstr(hex_content, "byteRows"));
    assert_non_null(strstr(hex_content, "formatOffset"));
    assert_non_null(strstr(hex_content, "activeByteInfo"));
    assert_non_null(strstr(hex_content, "copyHexString"));
    assert_non_null(strstr(hex_content, "Header"));
    assert_non_null(strstr(hex_content, "Address"));
    assert_non_null(strstr(hex_content, "Function"));
    assert_non_null(strstr(hex_content, "Payload"));
    assert_non_null(strstr(hex_content, "Checksum"));

    // 3. Verify index.ts exports
    char index_content[4096];
    read_file_content("web/core/src/index.ts", index_content, sizeof(index_content));

    assert_non_null(strstr(index_content, "export { default as ProtocolSnifferView }"));
    assert_non_null(strstr(index_content, "export { default as HexPayloadViewer }"));
}

static void test_c_dlt645_packet_dissection(void **state) {
    (void)state;
    sniffer_packet_t pkt;

    assert_int_equal(sniffer_init(), 0);

    // DLT645-2007 Read Voltage Request
    const uint8_t dlt645_req[] = {
        0x68, 0x11, 0x4D, 0x3C, 0x2B, 0x1A, 0x00, 0x68, 0x11, 0x04, 0x33, 0x33, 0x34, 0x33, 0x91, 0x16
    };

    assert_int_equal(sniffer_dissect_packet(dlt645_req, sizeof(dlt645_req), "RS485-0", "TX", "DLT645-2007", &pkt), 0);
    assert_string_equal(pkt.protocol, "DLT645-2007");
    assert_string_equal(pkt.direction, "TX");
    assert_string_equal(pkt.channel, "RS485-0");
    assert_int_equal(pkt.raw_len, 16);
    assert_true(pkt.is_valid_checksum);
    assert_true(pkt.segment_count >= 7);
    assert_non_null(strstr(pkt.decoded_summary, "DLT645 Read Request"));

    // DLT645-2007 Read Voltage Response (220.5V)
    const uint8_t dlt645_resp[] = {
        0x68, 0x11, 0x4D, 0x3C, 0x2B, 0x1A, 0x00, 0x68, 0x91, 0x06, 0x33, 0x33, 0x34, 0x33, 0x85, 0x45, 0xDD, 0x16
    };

    assert_int_equal(sniffer_dissect_packet(dlt645_resp, sizeof(dlt645_resp), "RS485-0", "RX", "DLT645-2007", &pkt), 0);
    assert_string_equal(pkt.direction, "RX");
    assert_true(pkt.is_valid_checksum);
    assert_non_null(strstr(pkt.decoded_summary, "DLT645 Response"));
}

static void test_c_modbus_packet_dissection(void **state) {
    (void)state;
    sniffer_packet_t pkt;

    // Modbus Read Holding Registers (Slave=1, Addr=0x0064, Qty=2, CRC=0xD485)
    const uint8_t modbus_req[] = { 0x01, 0x03, 0x00, 0x64, 0x00, 0x02, 0x85, 0xD4 };

    assert_int_equal(sniffer_dissect_packet(modbus_req, sizeof(modbus_req), "RS485-1", "TX", "Modbus-RTU", &pkt), 0);
    assert_string_equal(pkt.protocol, "Modbus-RTU");
    assert_string_equal(pkt.channel, "RS485-1");
    assert_true(pkt.is_valid_checksum);
    assert_true(pkt.segment_count >= 4);
    assert_non_null(strstr(pkt.decoded_summary, "Modbus Read Holding"));

    // Modbus Read Response (Active Power = 100.0 kW, CRC=0x437A)
    const uint8_t modbus_resp[] = { 0x01, 0x03, 0x04, 0x03, 0xE8, 0x00, 0x00, 0x7A, 0x43 };

    assert_int_equal(sniffer_dissect_packet(modbus_resp, sizeof(modbus_resp), "RS485-1", "RX", "Modbus-RTU", &pkt), 0);
    assert_true(pkt.is_valid_checksum);
    assert_non_null(strstr(pkt.decoded_summary, "Modbus Read Response"));
}

static void test_c_dlt698_packet_dissection(void **state) {
    (void)state;
    sniffer_packet_t pkt;

    // DLT698.45 Response Frame
    const uint8_t dlt698_frame[] = {
        0x68, 0x17, 0x00, 0x43, 0x05, 0x12, 0x34, 0x56, 0x78, 0x90, 0x00, 0x05, 0x01, 0x00, 0x20, 0x01, 0x02, 0x00, 0x00, 0x4A, 0x8C, 0x16
    };

    assert_int_equal(sniffer_dissect_packet(dlt698_frame, sizeof(dlt698_frame), "HPLC-Mesh", "RX", "DLT698.45", &pkt), 0);
    assert_string_equal(pkt.protocol, "DLT698.45");
    assert_true(pkt.is_valid_checksum);
    assert_true(pkt.segment_count >= 5);
    assert_non_null(strstr(pkt.decoded_summary, "DLT698.45 Frame"));
}

static void test_c_sniffer_json_and_pcap_serialization(void **state) {
    (void)state;
    sniffer_packet_t pkt;

    const uint8_t dlt645_req[] = {
        0x68, 0x11, 0x4D, 0x3C, 0x2B, 0x1A, 0x00, 0x68, 0x11, 0x04, 0x33, 0x33, 0x34, 0x33, 0x91, 0x16
    };
    assert_int_equal(sniffer_dissect_packet(dlt645_req, sizeof(dlt645_req), "RS485-0", "TX", "DLT645-2007", &pkt), 0);

    // 1. JSON Serialization
    char json_buf[4096];
    assert_int_equal(sniffer_format_json(&pkt, json_buf, sizeof(json_buf)), 0);
    assert_non_null(strstr(json_buf, "\"protocol\":\"DLT645-2007\""));
    assert_non_null(strstr(json_buf, "\"direction\":\"TX\""));
    assert_non_null(strstr(json_buf, "\"valid_crc\":true"));
    assert_non_null(strstr(json_buf, "\"segments\":["));

    // 2. PCAP Global Header
    uint8_t pcap_global[64];
    size_t g_len = sniffer_generate_pcap_global_header(pcap_global, sizeof(pcap_global));
    assert_int_equal(g_len, 24);

    uint32_t magic = 0;
    memcpy(&magic, pcap_global, 4);
    assert_int_equal(magic, 0xa1b2c3d4);

    // 3. PCAP Packet Record
    uint8_t pcap_pkt[256];
    size_t p_len = sniffer_generate_pcap_packet(&pkt, pcap_pkt, sizeof(pcap_pkt));
    assert_int_equal(p_len, 16 + 16); // 16B header + 16B payload
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_web_sniffer_vue_components_structure),
        cmocka_unit_test(test_c_dlt645_packet_dissection),
        cmocka_unit_test(test_c_modbus_packet_dissection),
        cmocka_unit_test(test_c_dlt698_packet_dissection),
        cmocka_unit_test(test_c_sniffer_json_and_pcap_serialization),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}

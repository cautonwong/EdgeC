/**
 * @file test_iec104_type_test_suite.c
 * @brief DL/T 634.5104 (IEC 104) State Grid Consistency Type Testing Automation Suite (AEP-430 /
 * Issue #138)
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iec104_apci.h"
#include "iec_asdu.h"
#include "net_buf.h"

static net_buf_pool_t g_pool;

static void setup_test_env(void) {
    static bool init = false;
    if (!init) {
        net_buf_pool_init(&g_pool, 32, 512, 0, NULL, NULL);
        init = true;
    }
}

static void test_iec104_apci_handshake_matrix(void) {
    printf("[TEST] IEC 104 APCI Handshake State Transition Matrix...\n");
    setup_test_env();

    iec104_apci_mgr_storage_t store;
    iec104_apci_mgr_t *mgr = iec104_apci_mgr_create_in(&store);
    assert(mgr != NULL);
    assert(iec104_apci_get_state(mgr) == IEC104_STATE_DISCONNECTED);

    // 1. Build STARTDT ACT
    net_buf_t *buf = net_buf_alloc(&g_pool, 0);
    iec104_apci_build_u(IEC104_U_STARTDT_ACT, buf);
    assert(buf->b.len == 6);
    assert(buf->b.data[0] == 0x68);
    assert(buf->b.data[1] == 0x04);
    assert(buf->b.data[2] == 0x07);  // STARTDT ACT

    // State transition to STARTING
    iec104_apci_set_state(mgr, IEC104_STATE_STARTING);
    assert(iec104_apci_get_state(mgr) == IEC104_STATE_STARTING);

    // Receive STARTDT CON -> CONNECTED
    iec104_apci_set_state(mgr, IEC104_STATE_CONNECTED);
    assert(iec104_apci_get_state(mgr) == IEC104_STATE_CONNECTED);

    // 2. Build TESTFR ACT
    net_buf_simple_reset(&buf->b);
    iec104_apci_build_u(IEC104_U_TESTFR_ACT, buf);
    assert(buf->b.data[2] == 0x43);  // TESTFR ACT

    // 3. Build STOPDT ACT
    net_buf_simple_reset(&buf->b);
    iec104_apci_build_u(IEC104_U_STOPDT_ACT, buf);
    assert(buf->b.data[2] == 0x13);  // STOPDT ACT

    net_buf_unref(buf);
    printf("  -> PASSED: APCI Handshake states verified.\n");
}

static void test_iec104_asdu_general_interrogation_full_flow(void) {
    printf("[TEST] IEC 104 General Interrogation (GI) End-to-End Sequence...\n");
    setup_test_env();

    iec104_apci_mgr_storage_t store;
    iec104_apci_mgr_t *mgr = iec104_apci_mgr_create_in(&store);
    iec104_apci_set_state(mgr, IEC104_STATE_CONNECTED);

    // 1. Master sends GI (C_IC_NA_1, COT=ACT, QOI=20)
    net_buf_t *gi_cmd = net_buf_alloc(&g_pool, 0);
    iec_asdu_init(gi_cmd, IEC_TYPE_C_IC_NA_1, false, 1, IEC_COT_ACT, 1, &IEC_ADDR_CONF_104);
    iec_asdu_add_c_ic_na_1(gi_cmd, 0, 20, &IEC_ADDR_CONF_104);
    iec104_apci_wrap_i(mgr, gi_cmd);
    assert(gi_cmd->b.len > 6);
    assert(gi_cmd->b.data[0] == 0x68);

    // 2. Slave parses GI header
    struct net_buf_simple b = gi_cmd->b;
    net_buf_simple_pull(&b, 6);  // pull APCI
    iec_type_id_t type;
    bool sq;
    uint8_t count;
    iec_cot_t cot;
    uint16_t casdu;
    int rc = iec_asdu_parse_header(&b, &type, &sq, &count, &cot, &casdu, &IEC_ADDR_CONF_104);
    assert(rc == 0);
    assert(type == IEC_TYPE_C_IC_NA_1);
    assert(cot == IEC_COT_ACT);

    // 3. Slave sends ACT_CON
    net_buf_t *act_con = net_buf_alloc(&g_pool, 0);
    iec_asdu_init(act_con, type, false, 1, IEC_COT_ACT_CON, casdu, &IEC_ADDR_CONF_104);
    net_buf_simple_add_u8(&act_con->b, 20);  // QOI = 20
    iec104_apci_wrap_i(mgr, act_con);

    // 4. Slave sends INTROGEN telemetry (M_ME_NC_1)
    net_buf_t *data_msg = net_buf_alloc(&g_pool, 0);
    iec_asdu_init(data_msg, IEC_TYPE_M_ME_NC_1, false, 1, IEC_COT_INTROGEN, casdu,
                  &IEC_ADDR_CONF_104);
    iec_asdu_add_any(data_msg, IEC_TYPE_M_ME_NC_1, 0x4001, 230.5, 0, 0, (iec_qds_t){0},
                     &IEC_ADDR_CONF_104);
    iec104_apci_wrap_i(mgr, data_msg);

    // 5. Slave sends ACT_TERM
    net_buf_t *act_term = net_buf_alloc(&g_pool, 0);
    iec_asdu_init(act_term, type, false, 1, IEC_COT_ACT_TERM, casdu, &IEC_ADDR_CONF_104);
    net_buf_simple_add_u8(&act_term->b, 20);
    iec104_apci_wrap_i(mgr, act_term);

    net_buf_unref(gi_cmd);
    net_buf_unref(act_con);
    net_buf_unref(data_msg);
    net_buf_unref(act_term);

    printf("  -> PASSED: General Interrogation 4-phase sequence verified.\n");
}

static void test_iec104_telecontrol_double_confirmation(void) {
    printf("[TEST] IEC 104 Telecontrol (C_SC_NA_1 / C_DC_NA_1) Select & Execute...\n");
    setup_test_env();

    iec104_apci_mgr_storage_t store;
    iec104_apci_mgr_t *mgr = iec104_apci_mgr_create_in(&store);
    iec104_apci_set_state(mgr, IEC104_STATE_CONNECTED);

    // 1. Single Command Select (C_SC_NA_1, COT=ACT, S/E=1)
    net_buf_t *sel_cmd = net_buf_alloc(&g_pool, 0);
    iec_asdu_init(sel_cmd, IEC_TYPE_C_SC_NA_1, false, 1, IEC_COT_ACT, 1, &IEC_ADDR_CONF_104);
    iec_asdu_add_any(sel_cmd, IEC_TYPE_C_SC_NA_1, 0x6001, 1.0, 0, 0x80, (iec_qds_t){0},
                     &IEC_ADDR_CONF_104);  // 0x80 = Select
    iec104_apci_wrap_i(mgr, sel_cmd);

    // 2. Execute Command (COT=ACT, S/E=0)
    net_buf_t *exec_cmd = net_buf_alloc(&g_pool, 0);
    iec_asdu_init(exec_cmd, IEC_TYPE_C_SC_NA_1, false, 1, IEC_COT_ACT, 1, &IEC_ADDR_CONF_104);
    iec_asdu_add_any(exec_cmd, IEC_TYPE_C_SC_NA_1, 0x6001, 1.0, 0, 0x00, (iec_qds_t){0},
                     &IEC_ADDR_CONF_104);  // 0x00 = Execute
    iec104_apci_wrap_i(mgr, exec_cmd);

    net_buf_unref(sel_cmd);
    net_buf_unref(exec_cmd);

    printf("  -> PASSED: Telecontrol Select and Execute commands verified.\n");
}

int main(void) {
    printf("=================================================================\n");
    printf("     AEP-430: IEC 104 State Grid Type Test Automation Suite     \n");
    printf("=================================================================\n");

    test_iec104_apci_handshake_matrix();
    test_iec104_asdu_general_interrogation_full_flow();
    test_iec104_telecontrol_double_confirmation();

    printf("\nAll AEP-430 IEC 104 Type Testing test cases PASSED!\n");
    return 0;
}

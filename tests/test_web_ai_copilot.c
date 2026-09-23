/**
 * @file test_web_ai_copilot.c
 * @brief Unit tests for Industrial AI Copilot Drawer & MCP Service (AEP-507)
 * @copyright (c) 2026 EdgeOS Project. All rights reserved.
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void read_file_content(const char *rel_path, char *buf, size_t max_len) {
    char path[512];
    FILE *fp = NULL;

    const char *prefixes[] = {
        "",  "../", "../../", "/workspaces/edgeos/EdgeC/", "/workspaces/edgeos/EdgeC-wt-AEP-507/",
        NULL};

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

static void test_ai_provider_service_structure(void **state) {
    (void)state;
    char content[65536];
    read_file_content("web/core/src/services/AIProviderService.ts", content, sizeof(content));

    // Multi-Provider Type Definitions
    assert_non_null(strstr(content, "AIProviderType"));
    assert_non_null(strstr(content, "ollama"));
    assert_non_null(strstr(content, "deepseek"));
    assert_non_null(strstr(content, "claude"));
    assert_non_null(strstr(content, "openai"));
    assert_non_null(strstr(content, "local_edge"));

    // 13 Industrial MCP Tools Definitions
    assert_non_null(strstr(content, "INDUSTRIAL_MCP_TOOLS"));
    assert_non_null(strstr(content, "datahub_read_tags"));
    assert_non_null(strstr(content, "datahub_write_tag"));
    assert_non_null(strstr(content, "datahub_get_tag_meta"));
    assert_non_null(strstr(content, "datahub_scan_abnormal"));
    assert_non_null(strstr(content, "io_list_channels"));
    assert_non_null(strstr(content, "io_get_channel_health"));
    assert_non_null(strstr(content, "io_trace_raw_packets"));
    assert_non_null(strstr(content, "pipeline_get_dag_status"));
    assert_non_null(strstr(content, "pipeline_query_metrics"));
    assert_non_null(strstr(content, "dds_list_discovered_nodes"));
    assert_non_null(strstr(content, "dds_inspect_topics"));
    assert_non_null(strstr(content, "storage_query_soe_events"));
    assert_non_null(strstr(content, "storage_query_waveform"));

    // Core ReAct Agent Engine Methods
    assert_non_null(strstr(content, "callMcpTool"));
    assert_non_null(strstr(content, "executeChat"));
    assert_non_null(strstr(content, "synthesizeDiagnosis"));
    assert_non_null(strstr(content, "useAICopilot"));
    assert_non_null(strstr(content, "openWithContext"));
}

static void test_mcp_tool_call_card_component(void **state) {
    (void)state;
    char content[32768];
    read_file_content("web/core/src/components/ai/McpToolCallCard.vue", content, sizeof(content));

    // Status Indicators & Domain Tagging
    assert_non_null(strstr(content, "tool.status"));
    assert_non_null(strstr(content, "executing"));
    assert_non_null(strstr(content, "getToolDomain"));
    assert_non_null(strstr(content, "domainColorClass"));
    assert_non_null(strstr(content, "DataHub"));
    assert_non_null(strstr(content, "IO Bus"));
    assert_non_null(strstr(content, "Pipeline"));
    assert_non_null(strstr(content, "DDS Mesh"));
    assert_non_null(strstr(content, "Storage"));

    // Collapsible & Copy Actions
    assert_non_null(strstr(content, "isExpanded"));
    assert_non_null(strstr(content, "copyJson"));
    assert_non_null(strstr(content, "formatJson"));
}

static void test_ai_copilot_drawer_component(void **state) {
    (void)state;
    char content[65536];
    read_file_content("web/core/src/components/ai/AICopilotDrawer.vue", content, sizeof(content));

    // Header & Liveliness
    assert_non_null(strstr(content, "EdgeOS AI 工业伴侣"));
    assert_non_null(strstr(content, "isMcpConnected"));
    assert_non_null(strstr(content, "MCP LIVE"));
    assert_non_null(strstr(content, "MCP OFFLINE"));

    // Settings Modal
    assert_non_null(strstr(content, "isSettingsOpen"));
    assert_non_null(strstr(content, "config.provider"));
    assert_non_null(strstr(content, "config.endpoint"));
    assert_non_null(strstr(content, "config.apiKey"));

    // Context Pill Strip
    assert_non_null(strstr(content, "activeContext"));
    assert_non_null(strstr(content, "selectedTagName"));

    // Quick Prompts
    assert_non_null(strstr(content, "quickPrompts"));
    assert_non_null(strstr(content, "全站异常扫描"));
    assert_non_null(strstr(content, "储能策略诊断"));
    assert_non_null(strstr(content, "SOE 事故反演"));

    // Hotkeys & Markdown
    assert_non_null(strstr(content, "handleKeyDown"));
    assert_non_null(strstr(content, "renderMarkdown"));
    assert_non_null(strstr(content, "McpToolCallCard"));
}

static void test_admin_layout_and_tag_card_integration(void **state) {
    (void)state;
    char layout_content[32768];
    read_file_content("web/core/src/layouts/AdminLayout.vue", layout_content,
                      sizeof(layout_content));

    assert_non_null(strstr(layout_content, "AICopilotDrawer"));
    assert_non_null(strstr(layout_content, "AI Copilot"));
    assert_non_null(strstr(layout_content, "useAICopilot"));
    assert_non_null(strstr(layout_content, "toggleCopilot"));

    char tag_card_content[32768];
    read_file_content("web/core/src/components/LiveTagCard.vue", tag_card_content,
                      sizeof(tag_card_content));

    assert_non_null(strstr(tag_card_content, "diagnoseWithAI"));
    assert_non_null(strstr(tag_card_content, "AI 诊断"));
    assert_non_null(strstr(tag_card_content, "openWithContext"));
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_ai_provider_service_structure),
        cmocka_unit_test(test_mcp_tool_call_card_component),
        cmocka_unit_test(test_ai_copilot_drawer_component),
        cmocka_unit_test(test_admin_layout_and_tag_card_integration),
    };

    printf("=== Running EdgeOS Industrial AI Copilot (AEP-507) Web Tests ===\n");
    return cmocka_run_group_tests(tests, NULL, NULL);
}

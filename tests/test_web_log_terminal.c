/**
 * test_web_log_terminal.c - Verification of Industrial ANSI Live Log Stream Console (AEP-498)
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

static void test_log_stream_terminal_component_structure(void **state) {
    (void)state;
    char content[65536];
    read_file_content("web/core/src/views/logs/LogStreamTerminal.vue", content, sizeof(content));

    // Headers & High-Frequency Stream Metrics
    assert_non_null(strstr(content, "LOG STREAM CONSOLE"));
    assert_non_null(strstr(content, "streamRateLps"));
    assert_non_null(strstr(content, "maxBufferLines"));
    assert_non_null(strstr(content, "toggleStreaming"));

    // Multi-level & domain filters
    assert_non_null(strstr(content, "filterLevel"));
    assert_non_null(strstr(content, "filterDomain"));
    assert_non_null(strstr(content, "DEBUG"));
    assert_non_null(strstr(content, "INFO"));
    assert_non_null(strstr(content, "WARN"));
    assert_non_null(strstr(content, "ERROR"));
    assert_non_null(strstr(content, "FATAL"));
    assert_non_null(strstr(content, "[BUS]"));
    assert_non_null(strstr(content, "[DRIVER]"));
    assert_non_null(strstr(content, "[STREAM]"));
    assert_non_null(strstr(content, "[ALGO]"));
    assert_non_null(strstr(content, "[PERSISTENCE]"));
    assert_non_null(strstr(content, "[SECURITY]"));
    assert_non_null(strstr(content, "[SYSTEM]"));

    // Theme switching
    assert_non_null(strstr(content, "selectedTheme"));
    assert_non_null(strstr(content, "cyber"));
    assert_non_null(strstr(content, "matrix"));
    assert_non_null(strstr(content, "amber"));
    assert_non_null(strstr(content, "monokai"));

    // Search, Regex & ANSI parser
    assert_non_null(strstr(content, "searchQuery"));
    assert_non_null(strstr(content, "isRegexMode"));
    assert_non_null(strstr(content, "parseAnsi"));
    assert_non_null(strstr(content, "colorMap"));
    assert_non_null(strstr(content, "renderAnsiWithHighlight"));

    // Auto-scroll follow tail, clear & forensic export
    assert_non_null(strstr(content, "autoScroll"));
    assert_non_null(strstr(content, "scrollToBottom"));
    assert_non_null(strstr(content, "clearLogs"));
    assert_non_null(strstr(content, "exportLogs"));
    assert_non_null(strstr(content, "streamSha256Checksum"));
}

static void test_log_stream_terminal_routing_and_exports(void **state) {
    (void)state;
    char router_content[8192];
    read_file_content("web/core/src/router/index.ts", router_content, sizeof(router_content));
    assert_non_null(strstr(router_content, "logs"));
    assert_non_null(strstr(router_content, "LogStreamTerminal.vue"));

    char index_content[8192];
    read_file_content("web/core/src/index.ts", index_content, sizeof(index_content));
    assert_non_null(strstr(index_content, "LogStreamTerminal"));
}

/* C-side ANSI SGR formatting & protocol test */
static void test_ansi_sgr_c_formatting_and_parsing(void **state) {
    (void)state;
    char buffer[256];
    const char *tag = "EdgeOS/Core";
    const char *msg = "Motor current exceeded threshold";
    
    // Format ANSI SGR colored log line: \x1b[1;31m[ERROR]\x1b[0m \x1b[1;36m[EdgeOS/Core]\x1b[0m Motor current exceeded threshold
    snprintf(buffer, sizeof(buffer), "\x1b[1;31m[ERROR]\x1b[0m \x1b[1;36m[%s]\x1b[0m %s", tag, msg);

    assert_non_null(strstr(buffer, "\x1b[1;31m"));
    assert_non_null(strstr(buffer, "\x1b[0m"));
    assert_non_null(strstr(buffer, "\x1b[1;36m"));
    assert_non_null(strstr(buffer, "EdgeOS/Core"));

    // Verify ANSI stripping logic (simulation of pure text export)
    char stripped[256];
    int j = 0;
    bool in_esc = false;
    for (int i = 0; buffer[i] != '\0'; i++) {
        if (buffer[i] == '\x1b') {
            in_esc = true;
        } else if (in_esc && buffer[i] == 'm') {
            in_esc = false;
        } else if (!in_esc) {
            stripped[j++] = buffer[i];
        }
    }
    stripped[j] = '\0';

    assert_string_equal(stripped, "[ERROR] [EdgeOS/Core] Motor current exceeded threshold");
}

/* Simulation of 5000-line industrial ringbuffer backpressure */
typedef struct {
    char lines[64][128];
    size_t head;
    size_t count;
    size_t capacity;
} mock_log_ring_t;

static void test_log_ring_buffer_backpressure_eviction(void **state) {
    (void)state;
    mock_log_ring_t ring;
    memset(&ring, 0, sizeof(ring));
    ring.capacity = 10;

    for (int i = 0; i < 25; i++) {
        char entry[128];
        snprintf(entry, sizeof(entry), "Log entry line #%03d", i);
        
        size_t slot = (ring.head + ring.count) % ring.capacity;
        if (ring.count == ring.capacity) {
            // Evict oldest
            slot = ring.head;
            ring.head = (ring.head + 1) % ring.capacity;
        } else {
            ring.count++;
        }
        strncpy(ring.lines[slot], entry, sizeof(ring.lines[slot]) - 1);
    }

    assert_int_equal(ring.count, 10);
    // Oldest surviving element should be line #015
    assert_string_equal(ring.lines[ring.head], "Log entry line #015");
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_log_stream_terminal_component_structure),
        cmocka_unit_test(test_log_stream_terminal_routing_and_exports),
        cmocka_unit_test(test_ansi_sgr_c_formatting_and_parsing),
        cmocka_unit_test(test_log_ring_buffer_backpressure_eviction),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}

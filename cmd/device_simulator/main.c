#include <uv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

#include "edge_log.h"
#include "net_buf.h"
#include "contracts.h"

#define MAX_SIM_TAGS TAG_MAX_ID
#define MOCK_DEVICE_ID 123456789

// --- State ---
static double g_sim_values[MAX_SIM_TAGS];
static uint8_t g_addr_bcd[6];

// --- DLT-645 Helpers ---
static uint8_t dlt645_calc_cs(const uint8_t *buf, size_t len) {
    uint8_t cs = 0;
    for (size_t i = 0; i < len; i++) cs += buf[i];
    return cs;
}

static void dlt645_double_to_bcd(double val, uint8_t *bcd, int bytes) {
    long long v = (long long)(val * 100 + 0.5); 
    for (int i = 0; i < bytes; i++) {
        uint8_t low = (uint8_t)(v % 10);
        v /= 10;
        uint8_t high = (uint8_t)(v % 10);
        v /= 10;
        bcd[i] = (uint8_t)((high << 4) | low);
    }
}

static void dlt645_encode_addr(uint64_t addr, uint8_t *out_bcd) {
    for (int i = 0; i < 6; i++) {
        out_bcd[i] = (uint8_t)(((addr / 10 % 10) << 4) | (addr % 10));
        addr /= 100;
    }
}

extern edge_tag_id_t edge_lookup_dlt645_num(uint32_t di, float *out_scale);

// --- Simulator Logic ---
void simulator_state_init(const char* product) {
    LOG_INFO("SIM", "Initializing simulated state for product: %s", product);
    for (int i = 0; i < MAX_SIM_TAGS; i++) {
        g_sim_values[i] = 1000.0 + (double)i;
    }
    if (TAG_VOLTAGE_PHASE_A < MAX_SIM_TAGS) g_sim_values[TAG_VOLTAGE_PHASE_A] = 221.7;
    if (TAG_GRID_POWER < MAX_SIM_TAGS) g_sim_values[TAG_GRID_POWER] = 5400.1;
    if (TAG_ENERGY_ACTIVE_IMPORT < MAX_SIM_TAGS) g_sim_values[TAG_ENERGY_ACTIVE_IMPORT] = 12345.67;
    LOG_INFO("SIM", "Simulated state initialized.");
    (void)product;
}

int handle_dlt645_request(net_buf_t *in, net_buf_t *resp_out) {
    struct net_buf_simple *b = &in->b;
    while (b->len > 0 && b->data[0] == 0xFE) net_buf_simple_pull(b, 1);
    if (b->len < 12 || b->data[0] != 0x68 || b->data[b->len - 1] != 0x16) return -1;
    if (dlt645_calc_cs(b->data, b->len - 2) != b->data[b->len - 2]) {
        LOG_WARN("SIM", "DLT645 checksum error");
        return -2;
    }
    if (memcmp(b->data + 1, g_addr_bcd, 6) != 0) return 0; 

    uint8_t ctrl = b->data[8];
    uint8_t req_len = b->data[10];
    struct net_buf_simple *rb = &resp_out->b;

    net_buf_simple_add_u8(rb, 0x68);
    net_buf_simple_add_mem(rb, g_addr_bcd, 6);
    net_buf_simple_add_u8(rb, 0x68);

    if (ctrl == 0x11 && req_len >= 4) { // Read request
        uint32_t req_di = 0;
        for (int i = 0; i < 4; i++) req_di |= (uint32_t)(b->data[11 + i] - 0x33) << (i * 8);
        
        float scaler = 1.0f;
        uint32_t tag_id = edge_lookup_dlt645_num(req_di, &scaler);
        
        if (tag_id != 0 && tag_id < MAX_SIM_TAGS) {
            double value = g_sim_values[tag_id]; // Direct access to state
            LOG_INFO("SIM", "Found TagID %u for DI %08X, value=%f", (unsigned int)tag_id, (unsigned int)req_di, value);
            
            net_buf_simple_add_u8(rb, 0x91); // Success
            int data_len = 4;
            uint8_t bcd_buf[4] = {0};
            dlt645_double_to_bcd(value, bcd_buf, data_len);

            net_buf_simple_add_u8(rb, 4 + data_len);
            for (int i = 0; i < 4; i++) net_buf_simple_add_u8(rb, (uint8_t)((req_di >> (i*8)) & 0xFF) + 0x33);
            for (int i = 0; i < data_len; i++) net_buf_simple_add_u8(rb, bcd_buf[i] + 0x33);
        } else {
            LOG_WARN("SIM", "DI %08X not found in contract", (unsigned int)req_di);
            net_buf_simple_add_u8(rb, 0xD1); 
            net_buf_simple_add_u8(rb, 1);
            net_buf_simple_add_u8(rb, 0x02 + 0x33);
        }
    } else if (ctrl == 0x14) {
        net_buf_simple_add_u8(rb, 0x94);
        net_buf_simple_add_u8(rb, 0);
    } else {
        net_buf_simple_add_u8(rb, 0xC0);
        net_buf_simple_add_u8(rb, 0);
    }

    net_buf_simple_add_u8(rb, dlt645_calc_cs(rb->data, rb->len));
    net_buf_simple_add_u8(rb, 0x16);
    return 0;
}

typedef struct {
    uv_poll_t poll_handle;
    int fd;
    net_buf_pool_t *pool;
    int (*handle_request)(net_buf_t *in, net_buf_t *out);
} app_context_t;

static void on_serial_data(uv_poll_t *handle, int status, int events) {
    if (status < 0) {
        LOG_ERROR("SIM", "Poll error: %s", uv_strerror(status));
        return;
    }
    if (!(events & UV_READABLE)) return;

    app_context_t *context = (app_context_t *)handle->data;
    net_buf_t *req = net_buf_alloc(context->pool, 0);
    net_buf_t *resp = net_buf_alloc(context->pool, 0);

    ssize_t nread = read(context->fd, req->b.data, net_buf_tailroom(req));
    if (nread > 0) {
        req->b.len = (uint16_t)nread;
        LOG_INFO("SIM", "Read %ld bytes", (long)nread);
        if (context->handle_request(req, resp) == 0 && resp->b.len > 0) {
            write(context->fd, resp->b.data, resp->b.len);
            LOG_INFO("SIM", "Wrote %ld bytes", (long)resp->b.len);
        }
    } else if (nread < 0 && errno != EAGAIN) {
        LOG_ERROR("SIM", "Read error: %s", strerror(errno));
    }
    net_buf_unref(req);
    net_buf_unref(resp);
}

static int open_serial_port(const char *port_path) {
    int fd = open(port_path, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) { perror("Failed to open serial port"); return -1; }
    struct termios tio;
    tcgetattr(fd, &tio);
    cfsetospeed(&tio, B9600);
    cfsetispeed(&tio, B9600);
    tio.c_cflag = (tio.c_cflag & ~CSIZE) | CS8 | CLOCAL | CREAD;
    tio.c_iflag = IGNPAR;
    tio.c_oflag = 0;
    tio.c_lflag = 0;
    tcflush(fd, TCIFLUSH);
    tcsetattr(fd, TCSANOW, &tio);
    return fd;
}

int main(int argc, char *argv[]) {
    if (edge_contract_self_test() != 0) {
        fprintf(stderr, "Contract self-test failed!\n");
        return -1;
    }

    const char *protocol = "DLT645-2007";
    const char *port = "/tmp/vport-slave";
    const char *product = "iems";

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--protocol") == 0 && i + 1 < argc) protocol = argv[++i];
        else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) port = argv[++i];
        else if (strcmp(argv[i], "--product") == 0 && i + 1 < argc) product = argv[++i];
    }

    edge_log_init(NULL, "device_sim");
    simulator_state_init(product);
    dlt645_encode_addr(MOCK_DEVICE_ID, g_addr_bcd);

    app_context_t context;
    if (strcmp(protocol, "DLT645-2007") == 0) {
        context.handle_request = handle_dlt645_request;
    } else {
        LOG_FATAL("SIM", "Protocol '%s' not supported by simulator.", protocol);
        return -1;
    }

    int fd = open_serial_port(port);
    if (fd < 0) { LOG_FATAL("SIM", "Failed to open port: %s", port); return -1; }
    LOG_INFO("SIM", "Opened port %s for %s", port, protocol);

    uv_loop_t *loop = uv_default_loop();
    net_buf_pool_t pool;
    net_buf_pool_init(&pool, 10, 256, 0, NULL, NULL);
    context.fd = fd;
    context.pool = &pool;
    context.poll_handle.data = &context;
    
    uv_poll_init(loop, &context.poll_handle, fd);
    uv_poll_start(&context.poll_handle, UV_READABLE, on_serial_data);
    LOG_INFO("SIM", "Device simulator started for product [%s]. Listening...", product);
    uv_run(loop, UV_RUN_DEFAULT);

    uv_loop_close(loop);
    close(fd);
    net_buf_pool_destroy(&pool);
    edge_log_fini();
    return 0;
}

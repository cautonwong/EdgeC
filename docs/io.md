# IO Executor & Driver Development Guide

This document outlines the architecture of the Input/Output (IO) subsystem in EdgeOS and provides a step-by-step guide for implementing industrial protocols (Drivers).

## 1. Architecture Overview

The IO Executor is the "Execution Plane" of EdgeOS. Its primary responsibility is to interface with physical hardware (UART, CAN, GPIO, TCP/IP) to exchange data with external devices.

### 1.1 Current Architecture (V2: Poller Thread)
Currently, the IO Executor runs as a **dedicated polling thread**.

*   **Mechanism**: A `while(running)` loop that iterates over a list of "Poll Groups".
*   **Scheduling**: Uses `usleep()` to control sampling frequency.
*   **Concurrency**: All IO operations in this thread are synchronous (blocking). This ensures strict ordering but limits concurrency.
*   **Data Flow**:
    *   **Read**: Driver -> Buffer -> Parse -> `DataHub.publish` -> ShmRing.
    *   **Write**: Arbiter -> Command Queue -> Driver -> Write.

### 1.2 Future Architecture (V3: Event Driven)
The target architecture moves towards a fully event-driven model using `libuv`.

*   **Mechanism**: `uv_poll_t` monitors file descriptors (UART/Sockets) for readability.
*   **Scheduling**: Timers (`uv_timer_t`) trigger requests.
*   **Concurrency**: Non-blocking IO allows handling hundreds of devices on a single thread.

---

## 2. Driver Development Guide

EdgeOS uses a **Contract-Driven** driver model. Drivers **do not** contain business logic or hardcoded register maps. Instead, they use the **Contract Lookup Table** to resolve physical addresses to logical Tag IDs.

### 2.1 Interface Definition (`edge_driver.h`)

A driver must implement the `edge_driver_t` interface:

```c
typedef struct edge_driver_s {
    const char *name;
    int (*init)(void *ctx);

    // --- Master Mode (Polling) ---
    int (*build_batch_req)(edge_poll_group_t *group, uint8_t *tx_buf, size_t *tx_len);
    int (*parse_batch_res)(edge_poll_group_t *group, uint8_t *rx_buf, size_t rx_len,
                           void (*on_msg)(pipe_msg_t *msg, void *user_data), void *user_data);

    // --- Slave Mode (Listening) ---
    int (*parse_request)(const uint8_t *rx_buf, size_t rx_len, uint32_t *out_slave_id,
                         uint16_t *out_di);
    int (*build_response)(uint32_t slave_id, uint8_t ctrl_code, uint32_t di, const pipe_msg_t *data,
                          uint8_t *tx_buf, size_t *tx_len);
    
    // ... cleanup ...
} edge_driver_t;
```

### 2.2 Implementing Master Mode (e.g., Reading a Meter)

**Goal**: Construct a request frame and parse the response frame.

1.  **Build Request**: Use `group->start_addr` and `group->count` to form the packet.
2.  **Parse Response**:
    *   Validate Checksum/CRC.
    *   Extract raw values.
    *   **Crucial Step**: Use `edge_lookup_PROTOCOL(address, &scale)` to find the `tag_id`.
    *   Publish using `on_msg` callback.

**Example (Modbus):**
```c
// Looking up what "Register 100" means
float scale = 1.0f;
uint32_t tid = edge_lookup_modbus(100, &scale);

// Publish
pipe_msg_init_scalar(&msg, tid, raw_value * scale, timestamp);
on_msg(&msg, user_data);
```

### 2.3 Implementing Slave Mode (e.g., Acting as a Simulator)

**Goal**: Parse an incoming request from a Master and generate a response.

1.  **Parse Request**:
    *   Use `net_buf_simple` for safe buffer parsing.
    *   Extract `slave_id` and the requested Data Identifier (`di`).
    *   Return 0 on success.

2.  **Build Response**:
    *   Receive the requested data (`pipe_msg_t *data`).
    *   Format the frame (add headers, encode data, calculate checksum).

---

## 3. Case Study: DLT645-2007 Implementation

The DLT645 driver (`src/io_executor/driver_dlt645.c`) demonstrates advanced features:

### 3.1 BCD Encoding/Decoding
DLT645 uses "Excess-33" encoding and BCD. Helper functions are essential:
*   `dlt645_bcd_to_double`: Decodes BCD bytes to float.
*   `dlt645_double_to_bcd`: Encodes float to BCD bytes.

### 3.2 Safe Parsing with `net_buf`
We use the `net_buf` library (from Zephyr) to avoid buffer overflows.

**Bad (Unsafe):**
```c
uint8_t len = buf[9];
for (int i=0; i<len; i++) ptr[i] = buf[10+i]; // Potential Overflow!
```

**Good (EdgeOS Style):**
```c
struct net_buf_simple b;
net_buf_simple_init_with_data(&b, buf, len);
net_buf_simple_pull(&b, 9); // Skip header safely
uint8_t data_len = net_buf_simple_pull_u8(&b);
if (b.len < data_len) return -1; // Overflow check
```

### 3.3 Contract Binding
The driver declares `extern uint32_t edge_lookup_dlt645(...)`. This symbol is provided by the **Contract Compiler** generated code (`contract_lookup.c`). This decouples the driver from the specific site configuration.

---

## 4. Testing

Drivers should be tested using `cmocka`.
*   **Batch Cycle Test**: Simulates a Master polling loop.
*   **Slave Mode Test**: Simulates receiving a packet and generating a response.

See `tests/test_driver_dlt645.c` for a complete example.

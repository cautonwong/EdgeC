## Parent

Reference: /workspaces/edgeos/IEMS_PRD.md

## What to build

Implement the `passthrough_manager` in the Control Plane to enable transparent TCP-to-Serial/TCP tunnels for remote debugging. 
The service must listen on designated TCP ports. When a connection is active, it must signal the `arbiter` to safely suspend or interleave regular protocol polling (e.g., Modbus/DLT) to acquire an 'exclusive bus hold'. Raw byte streams must be bidirectionally routed between the remote socket and the underlying `io_handle_t` using `libuv`.

## Acceptance criteria

- [ ] A TCP server is created that binds to a configured passthrough port.
- [ ] The `arbiter` is modified to support an 'exclusive hold' state that temporarily halts standard polling sequences.
- [ ] Raw bytes sent to the TCP socket are transmitted directly via `io_handle_t` to the field bus, and vice versa.
- [ ] Existing protocol decoders do not crash when proprietary bytes are injected during passthrough.
- [ ] Regular polling seamlessly resumes when the passthrough TCP connection drops.

## Blocked by

None - can start immediately

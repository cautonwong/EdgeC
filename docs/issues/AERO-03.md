## Parent

Reference: /workspaces/edgeos/EdgeC/AERO_EDGE_PRD.md

## What to build

Implement the `ccsds_telemetry_driver`. This protocol driver will format the outgoing telemetry data (extracted from the DataHub) into standard Consultative Committee for Space Data Systems (CCSDS) space packets. It must interface with the lower-level Zephyr hardware abstractions (UART/SPI) to transmit the binary packets down to the ground station radio module.

## Acceptance criteria

- [ ] A new driver implementing the `edge_driver_t` interface for CCSDS framing is created.
- [ ] The driver correctly formats DataHub variables into standard CCSDS primary and secondary packet headers.
- [ ] The driver handles packet sequence control and length calculation dynamically based on payload size.
- [ ] A mock test verifies that generated packets conform to the CCSDS 133.0-B-1 structural specification.

## Blocked by

None - can start immediately
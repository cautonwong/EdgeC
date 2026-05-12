## Parent

Reference: /workspaces/edgeos/IEMS_PRD.md

## What to build

Implement a new `sparkplug_b_driver` as a north-bound protocol interface. It must conform to the `edge_driver_t` standard and leverage the Eclipse Tahu specification (via the existing Paho MQTT client) to publish edge node metrics. It should automatically serialize DataHub tag updates into Sparkplug B payloads, allowing cloud infrastructures to discover the tags without manual configuration mapping.

## Acceptance criteria

- [ ] `sparkplug_b_driver` is integrated into the protocol framework.
- [ ] The driver successfully connects to a standard MQTT broker.
- [ ] It correctly publishes NBIRTH, DBIRTH, and NDATA/DDATA messages conforming to the Sparkplug B payload specification.
- [ ] Shadow updates in the DataHub automatically trigger corresponding DDATA publishes.

## Blocked by

None - can start immediately
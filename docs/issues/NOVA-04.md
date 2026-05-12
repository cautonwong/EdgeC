## Parent

Reference: /workspaces/edgeos/EdgeC/NOVA_HMI_PRD.md

## What to build

Develop the `stream_topology_graph` component for the frontend. This interactive component will fetch the active EdgeC `StreamEngine` configuration (the JSON definition of pipelines, stages, and tags) and render a visual node graph. It should allow operators to visually trace how data flows from a physical Modbus device, through a mathematical filter (e.g., EKF or FFT), and out to a control target or MQTT sink.

## Acceptance criteria

- [ ] The backend is updated to serve the current Pipeline JSON configuration via a REST API endpoint.
- [ ] A frontend component is built using an SVG or Canvas-based node graphing library (e.g., React Flow).
- [ ] Nodes represent Stages and Edges represent data flow (Tags).
- [ ] The visual design of the graph conforms to the "Utilitarian Cyber-Physical" aesthetic (monochrome wiring, neon highlights for active flow).
- [ ] Clicking on a node displays its configuration parameters (e.g., filter coefficients, matrix dimensions).

## Blocked by

- [NOVA-01] Frontend Core Scaffold & Design System
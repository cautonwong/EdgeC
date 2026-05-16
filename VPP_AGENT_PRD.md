## Problem Statement

Distributed Energy Resources (DERs) like solar inverters and battery storage are often underutilized. To maximize economic returns, these assets need to participate in Virtual Power Plants (VPP) and Demand Response programs. This requires an edge controller that can perform real-time optimization against dynamic energy prices, handle high-frequency control of the PCS, and provide an immutable audit trail for carbon credits.

## Solution

The EdgeC VPP Agent (VPP-Agent) is a specialized application that turns any EdgeC node into a grid-aware financial optimizer. It utilizes the `mpc_dispatcher_stage` to compute optimal charge/discharge schedules based on price forecasts retrieved via REST/MQTT. It ensures grid-code compliance using the `control_lqr_solve` to stabilize local voltage/frequency. All energy transactions and carbon reductions are locally signed and stored in the `SQLite Historian` for future monetization.

## User Stories

1. As a **battery asset owner**, I want the VPP Agent to automatically discharge my batteries when grid prices are highest, so that I can generate maximum revenue from price arbitrage.
2. As a **VPP Aggregator**, I want to send a "Fast Frequency Response" command via IEC 104, and have the EdgeC node react in < 100ms, so that I can provide primary frequency control services to the grid.
3. As a **carbon trader**, I want to pull an immutable record of my CO2 savings from the EdgeC local database, so that I can mint verified carbon credits.

## Implementation Decisions

- **`module: price_forecast_subscriber`**: A management thread that fetches 24-hour electricity price curves and stores them in the DataHub.
- **`module: transaction_logger`**: A specialized SQLite sink that logs every energy "event" (charge/discharge start, kWh transferred, price at time of event) with a cryptographic hash.

## Testing Decisions

- **Economic Optimization Test (AFK)**: We will simulate a fluctuating price curve and assert that the MPC stage chooses to charge during low-price valleys and discharge during high-price peaks, maximizing the "Total Profit" metric in the test results.

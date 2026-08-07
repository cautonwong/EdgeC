# EdgeOS / EdgeC Contract & TagID Specification

## TagID Segment Partitioning Rules

| Segment Range | Category / Purpose | Description & Examples |
|---|---|---|
| `0` | Reserved | `TAG_INVALID` |
| `1000 - 11999` | Business Telemetry & Metrology | Standard power, telemetry, metrology, energy, battery, control, DLMS, policy, environment tags (`VOLTAGE_PHASE_A`=1001, `CURRENT_PHASE_A`=1002, `GRID_POWER`=1003, `ENERGY_ACTIVE_IMPORT`=3001, `BATTERY_SOC`=4001) |
| `12000 - 12999` | Metrics, Performance & System Audit | DataHub performance metrics (`12001-12004`), SmartBox system audit & state tags (`12006-12035`) |
| `13000 - 65535` | Custom & Project Extensions | Product-specific custom domain extension tag IDs |
| `0x00010000 - 0x7FFFFFFF` | Protocol Native OAD Identifiers | DLT698 32-bit hex OAD identifiers (`OAD_VOLTAGE`=`0x20000200`, `OAD_ACTIVE_ENERGY`=`0x00100200`) |

## Architectural Governance & Contract Rules

1. **TagID Range Rules**: All tag IDs MUST fall within valid defined segment ranges (`1000-65535` or `0x00010000-0x7FFFFFFF`).
2. **TagID & Name Uniqueness**: Tag IDs and tag names MUST be unique across all active contracts.
3. **Data Type Whitelist**: Data types MUST be selected from: `float32`, `float64`, `int32`, `int64`, `uint32`, `uint64`, `bool`, `string`, `bytes`.
4. **Physical Units Requirement**: Numeric telemetry/metrology tags in strict validation mode MUST define a physical unit (`V`, `A`, `W`, `Hz`, `%`, `°C`, `s`, `N/A`).
5. **No Hardcoded OAD Duplication**: Protocol OADs are declared in protocol bindings (`contracts/bindings/dlt698/*.yaml`) and resolved dynamically to business TagIDs via contract-compiler generated `edge_lookup_dlt698()`. Code-level hardcoded dual-encodings are forbidden.

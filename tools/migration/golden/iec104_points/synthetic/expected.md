# IEC104 CA point table

- config: `config/iec104_config.json`
- kind ranges: `src/automatic_north/iec104_create.c:2` start_RemotePoint=1 (telemetry); `src/automatic_north/iec104_create.c:3` start_RemoteMeas=100 (telemetry); `src/automatic_north/iec104_create.c:4` start_RemoteCtrl=200 (control); `src/automatic_north/iec104_create.c:5` start_RemoteRegu=300 (control)
- points: 7 across 2 CAs

| CA | points | telemetry | control | unknown |
| ---: | ---: | ---: | ---: | ---: |
| 1 | 6 | 3 | 2 | 1 |
| 2 | 1 | 1 | 0 | 0 |
| total | 7 | 4 | 2 | 1 |

## Points

| CA | name | IOA | type | scale | kind | evidence |
| ---: | --- | ---: | --- | ---: | --- | --- |
| 1 | A.none | 0 | int | 1.0 | unknown | config/iec104_config.json:11; no kind range for IOA 0 (first start 1) |
| 1 | A.sig | 5 | bool | 1.0 | telemetry | config/iec104_config.json:7; src/automatic_north/iec104_create.c:2 start_RemotePoint=1 |
| 1 | A.dup | 150 | int | 1.0 | telemetry | config/iec104_config.json:12; src/automatic_north/iec104_create.c:3 start_RemoteMeas=100 |
| 1 | A.meas | 150 | float | 0.1 | telemetry | config/iec104_config.json:8; src/automatic_north/iec104_create.c:3 start_RemoteMeas=100 |
| 1 | A.ctrl | 245 | int | 1.0 | control | config/iec104_config.json:9; src/automatic_north/iec104_create.c:4 start_RemoteCtrl=200 |
| 1 | A.regu | 350 | float | 2.5 | control | config/iec104_config.json:10; src/automatic_north/iec104_create.c:5 start_RemoteRegu=300 |
| 2 | B.sig | 6 | bool | 1.0 | telemetry | config/iec104_config.json:18; src/automatic_north/iec104_create.c:2 start_RemotePoint=1 |

## Anomalies

- CA 1: IOA 150 used by 2 points: A.dup, A.meas
- CA 1: IOA 0 (A.none) has no kind range

# IEC104 CA point table

- config: `config/iec104_config.json`
- kind ranges: `src/automatic_north/iec104_create.c:64` start_RemotePoint=1 (telemetry); `src/automatic_north/iec104_create.c:65` start_RemoteMeas=16385 (telemetry); `src/automatic_north/iec104_create.c:66` start_RemoteCtrl=24577 (control); `src/automatic_north/iec104_create.c:67` start_RemoteRegu=25089 (control)
- points: 178 across 2 CAs

| CA | points | telemetry | control | unknown |
| ---: | ---: | ---: | ---: | ---: |
| 1 | 176 | 176 | 0 | 0 |
| 2 | 2 | 2 | 0 | 0 |
| total | 178 | 178 | 0 | 0 |

## Points

| CA | name | IOA | type | scale | kind | evidence |
| ---: | --- | ---: | --- | ---: | --- | --- |
| 1 | EMS.BmsLimited | 1 | bool | 1.0 | telemetry | config/iec104_config.json:10; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.SysStoped | 2 | bool | 1.0 | telemetry | config/iec104_config.json:11; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.PowerReflux | 3 | bool | 1.0 | telemetry | config/iec104_config.json:12; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.PowerLimited | 4 | bool | 1.0 | telemetry | config/iec104_config.json:13; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.IoProtect | 5 | bool | 1.0 | telemetry | config/iec104_config.json:14; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.MeterError | 6 | bool | 1.0 | telemetry | config/iec104_config.json:15; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.EMS.DcTotalVolUnusual | 7 | bool | 1.0 | telemetry | config/iec104_config.json:16; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.ProhibiteCha | 8 | bool | 1.0 | telemetry | config/iec104_config.json:17; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.ProhibiteDisc | 9 | bool | 1.0 | telemetry | config/iec104_config.json:18; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.UserStop1 | 10 | bool | 1.0 | telemetry | config/iec104_config.json:19; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.UserStop2 | 11 | bool | 1.0 | telemetry | config/iec104_config.json:20; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.UserStop3 | 12 | bool | 1.0 | telemetry | config/iec104_config.json:21; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.UserAlarm1 | 13 | bool | 1.0 | telemetry | config/iec104_config.json:22; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.UserAlarm2 | 14 | bool | 1.0 | telemetry | config/iec104_config.json:23; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.UserAlarm3 | 15 | bool | 1.0 | telemetry | config/iec104_config.json:24; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.EnBmsAlarm | 16 | bool | 1.0 | telemetry | config/iec104_config.json:25; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.EnProtectDOD | 17 | bool | 1.0 | telemetry | config/iec104_config.json:26; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.EnIoProtect | 18 | bool | 1.0 | telemetry | config/iec104_config.json:27; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.EnProtectTransf | 19 | bool | 1.0 | telemetry | config/iec104_config.json:28; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.EnTraceLoad | 20 | bool | 1.0 | telemetry | config/iec104_config.json:29; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.LoadTrackingLimit | 21 | bool | 1.0 | telemetry | config/iec104_config.json:30; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.EnProtectReverse | 22 | bool | 1.0 | telemetry | config/iec104_config.json:31; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.EnDraughtFanPower | 23 | bool | 1.0 | telemetry | config/iec104_config.json:32; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.EnDraughtFanTem | 24 | bool | 1.0 | telemetry | config/iec104_config.json:33; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.MeterInvert | 25 | bool | 1.0 | telemetry | config/iec104_config.json:34; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.ClrAlarm | 26 | bool | 1.0 | telemetry | config/iec104_config.json:35; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.OnOff | 27 | bool | 1.0 | telemetry | config/iec104_config.json:36; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.DemandThreshold1 | 16385 | int | 1.0 | telemetry | config/iec104_config.json:37; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.DemandThreshold2 | 16386 | int | 1.0 | telemetry | config/iec104_config.json:38; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.DemandThreshold3 | 16387 | int | 1.0 | telemetry | config/iec104_config.json:39; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.DemandThreshold4 | 16388 | int | 1.0 | telemetry | config/iec104_config.json:40; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.DemandThreshold5 | 16389 | int | 1.0 | telemetry | config/iec104_config.json:41; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.DemandThreshold6 | 16390 | int | 1.0 | telemetry | config/iec104_config.json:42; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.DemandThreshold7 | 16391 | int | 1.0 | telemetry | config/iec104_config.json:43; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.DemandThreshold8 | 16392 | int | 1.0 | telemetry | config/iec104_config.json:44; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.DemandThreshold9 | 16393 | int | 1.0 | telemetry | config/iec104_config.json:45; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.DemandThreshold10 | 16394 | int | 1.0 | telemetry | config/iec104_config.json:46; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.DemandThreshold11 | 16395 | int | 1.0 | telemetry | config/iec104_config.json:47; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.DemandThreshold12 | 16396 | int | 1.0 | telemetry | config/iec104_config.json:48; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.Pmlmax | 16396 | int | 1.0 | telemetry | config/iec104_config.json:49; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.SOCmax | 16397 | int | 1.0 | telemetry | config/iec104_config.json:50; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.SOCmin | 16398 | int | 1.0 | telemetry | config/iec104_config.json:51; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.DemandErr | 16399 | int | 1.0 | telemetry | config/iec104_config.json:52; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.TempThreshold | 16400 | int | 1.0 | telemetry | config/iec104_config.json:53; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S1Power | 16401 | int | 1.0 | telemetry | config/iec104_config.json:54; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S1StMin | 16402 | int | 1.0 | telemetry | config/iec104_config.json:55; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S1StHour | 16403 | int | 1.0 | telemetry | config/iec104_config.json:56; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S1EnMin | 16404 | int | 1.0 | telemetry | config/iec104_config.json:57; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S1EnHour | 16405 | int | 1.0 | telemetry | config/iec104_config.json:58; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S2Power | 16406 | int | 1.0 | telemetry | config/iec104_config.json:59; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S2StMin | 16407 | int | 1.0 | telemetry | config/iec104_config.json:60; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S2StHour | 16408 | int | 1.0 | telemetry | config/iec104_config.json:61; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S2EnMin | 16409 | int | 1.0 | telemetry | config/iec104_config.json:62; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S2EnHour | 16410 | int | 1.0 | telemetry | config/iec104_config.json:63; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S3Power | 16411 | int | 1.0 | telemetry | config/iec104_config.json:64; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S3StMin | 16412 | int | 1.0 | telemetry | config/iec104_config.json:65; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S3StHour | 16413 | int | 1.0 | telemetry | config/iec104_config.json:66; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S3EnMin | 16415 | int | 1.0 | telemetry | config/iec104_config.json:67; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S3EnHour | 16416 | int | 1.0 | telemetry | config/iec104_config.json:68; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S4Power | 16417 | int | 1.0 | telemetry | config/iec104_config.json:69; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S4StMin | 16418 | int | 1.0 | telemetry | config/iec104_config.json:70; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S4StHour | 16419 | int | 1.0 | telemetry | config/iec104_config.json:71; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S4EnMin | 16420 | int | 1.0 | telemetry | config/iec104_config.json:72; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S4EnHour | 16421 | int | 1.0 | telemetry | config/iec104_config.json:73; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S5Power | 16422 | int | 1.0 | telemetry | config/iec104_config.json:74; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S5StMin | 16423 | int | 1.0 | telemetry | config/iec104_config.json:75; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S5StHour | 16424 | int | 1.0 | telemetry | config/iec104_config.json:76; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S5EnMin | 16425 | int | 1.0 | telemetry | config/iec104_config.json:77; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S5EnHour | 16426 | int | 1.0 | telemetry | config/iec104_config.json:78; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S6Power | 16427 | int | 1.0 | telemetry | config/iec104_config.json:79; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S6StMin | 16428 | int | 1.0 | telemetry | config/iec104_config.json:80; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S6StHour | 16429 | int | 1.0 | telemetry | config/iec104_config.json:81; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S6EnMin | 16430 | int | 1.0 | telemetry | config/iec104_config.json:82; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S6EnHour | 16431 | int | 1.0 | telemetry | config/iec104_config.json:83; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.PcsAutoTurn | 16432 | int | 1.0 | telemetry | config/iec104_config.json:84; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.PcsAutoTurnOffTime | 16433 | int | 1.0 | telemetry | config/iec104_config.json:85; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.SetConnect | 16434 | int | 1.0 | telemetry | config/iec104_config.json:86; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.SetRePower | 16435 | int | 1.0 | telemetry | config/iec104_config.json:87; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.SetPower | 16436 | int | 1.0 | telemetry | config/iec104_config.json:88; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.StratagyType | 16437 | int | 1.0 | telemetry | config/iec104_config.json:89; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.ControlMode | 16438 | int | 1.0 | telemetry | config/iec104_config.json:90; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.Adjpreriod | 16439 | int | 1.0 | telemetry | config/iec104_config.json:91; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.K2 | 16440 | int | 1.0 | telemetry | config/iec104_config.json:92; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.Pset | 16442 | int | 1.0 | telemetry | config/iec104_config.json:93; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.DemandStartMin | 16443 | int | 1.0 | telemetry | config/iec104_config.json:94; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.DemandStartHour | 16444 | int | 1.0 | telemetry | config/iec104_config.json:95; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.DemandEndMin | 16445 | int | 1.0 | telemetry | config/iec104_config.json:96; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.DemandEndHour | 16446 | int | 1.0 | telemetry | config/iec104_config.json:97; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.K1 | 16447 | int | 1.0 | telemetry | config/iec104_config.json:98; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.DFanPowerThreShold | 16448 | int | 1.0 | telemetry | config/iec104_config.json:99; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.DFanTemThreShold | 16449 | int | 1.0 | telemetry | config/iec104_config.json:100; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.LocalCtrlPower | 16450 | int | 1.0 | telemetry | config/iec104_config.json:101; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.Income | 16451 | int | 1.0 | telemetry | config/iec104_config.json:102; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.Efficiency | 16452 | int | 1.0 | telemetry | config/iec104_config.json:103; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.ExpectPower | 16453 | int | 1.0 | telemetry | config/iec104_config.json:104; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.SysStatus | 16454 | int | 1.0 | telemetry | config/iec104_config.json:105; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.CabPower | 16455 | int | 1.0 | telemetry | config/iec104_config.json:106; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.CabCapacity | 16456 | int | 1.0 | telemetry | config/iec104_config.json:107; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.GridPower | 16457 | float | 1.0 | telemetry | config/iec104_config.json:108; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.LoadPower | 16458 | float | 1.0 | telemetry | config/iec104_config.json:109; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.ActiveEnergy | 16459 | float | 1.0 | telemetry | config/iec104_config.json:110; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.ReactiveEnergy | 16460 | float | 1.0 | telemetry | config/iec104_config.json:111; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.AphaseVoltage | 16461 | float | 1.0 | telemetry | config/iec104_config.json:112; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.BphaseVoltage | 16462 | float | 1.0 | telemetry | config/iec104_config.json:113; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.CphaseVoltage | 16463 | float | 1.0 | telemetry | config/iec104_config.json:114; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.AphaseCurrent | 16464 | float | 1.0 | telemetry | config/iec104_config.json:115; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.BphaseCurrent | 16465 | float | 1.0 | telemetry | config/iec104_config.json:116; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.CphaseCurrent | 16466 | float | 1.0 | telemetry | config/iec104_config.json:117; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.ActivePower | 16467 | float | 1.0 | telemetry | config/iec104_config.json:118; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.AphaseActivePower | 16468 | float | 1.0 | telemetry | config/iec104_config.json:119; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.BphaseActivePower | 16469 | float | 1.0 | telemetry | config/iec104_config.json:120; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.CphaseActivePower | 16470 | float | 1.0 | telemetry | config/iec104_config.json:121; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.ReactivePower | 16471 | float | 1.0 | telemetry | config/iec104_config.json:122; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.AphaseReactivePower | 16472 | float | 1.0 | telemetry | config/iec104_config.json:123; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.BphaseReactivePower | 16473 | float | 1.0 | telemetry | config/iec104_config.json:124; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.CphaseReactivePower | 16474 | float | 1.0 | telemetry | config/iec104_config.json:125; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.ApparentPower | 16475 | float | 1.0 | telemetry | config/iec104_config.json:126; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.AphaseApparentPower | 16476 | float | 1.0 | telemetry | config/iec104_config.json:127; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.BphaseApparentPower | 16477 | float | 1.0 | telemetry | config/iec104_config.json:128; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.CphaseApparentPower | 16478 | float | 1.0 | telemetry | config/iec104_config.json:129; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.PowerFactor | 16479 | float | 1.0 | telemetry | config/iec104_config.json:130; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.AphasePowerFactor | 16480 | float | 1.0 | telemetry | config/iec104_config.json:131; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.BphasePowerFactor | 16481 | float | 1.0 | telemetry | config/iec104_config.json:132; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.CphasePowerFactor | 16482 | float | 1.0 | telemetry | config/iec104_config.json:133; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.ActiveEnergy | 16483 | float | 1.0 | telemetry | config/iec104_config.json:134; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.PositiveActiveRate1 | 16484 | float | 1.0 | telemetry | config/iec104_config.json:135; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.PositiveActiveRate2 | 16485 | float | 1.0 | telemetry | config/iec104_config.json:136; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.PositiveActiveRate3 | 16486 | float | 1.0 | telemetry | config/iec104_config.json:137; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.PositiveActiveRate4 | 16487 | float | 1.0 | telemetry | config/iec104_config.json:138; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.ReactiveEnergy | 16488 | float | 1.0 | telemetry | config/iec104_config.json:139; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.ReverseActiveRate1 | 16489 | float | 1.0 | telemetry | config/iec104_config.json:140; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.ReverseActiveRate2 | 16490 | float | 1.0 | telemetry | config/iec104_config.json:141; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.ReverseActiveRate3 | 16491 | float | 1.0 | telemetry | config/iec104_config.json:142; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.ReverseActiveRate4 | 16492 | float | 1.0 | telemetry | config/iec104_config.json:143; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.AphaseVoltage | 16493 | float | 1.0 | telemetry | config/iec104_config.json:144; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.BphaseVoltage | 16494 | float | 1.0 | telemetry | config/iec104_config.json:145; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.CphaseVoltage | 16495 | float | 1.0 | telemetry | config/iec104_config.json:146; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.AphaseCurrent | 16496 | float | 1.0 | telemetry | config/iec104_config.json:147; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.BphaseCurrent | 16497 | float | 1.0 | telemetry | config/iec104_config.json:148; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.CphaseCurrent | 16498 | float | 1.0 | telemetry | config/iec104_config.json:149; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.ActivePower | 16499 | float | 1.0 | telemetry | config/iec104_config.json:150; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.AphaseActivePower | 16500 | float | 1.0 | telemetry | config/iec104_config.json:151; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.BphaseActivePower | 16501 | float | 1.0 | telemetry | config/iec104_config.json:152; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.CphaseActivePower | 16502 | float | 1.0 | telemetry | config/iec104_config.json:153; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.ReactivePower | 16503 | float | 1.0 | telemetry | config/iec104_config.json:154; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.AphaseReactivePower | 16504 | float | 1.0 | telemetry | config/iec104_config.json:155; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.BphaseReactivePower | 16505 | float | 1.0 | telemetry | config/iec104_config.json:156; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.CphaseReactivePower | 16506 | float | 1.0 | telemetry | config/iec104_config.json:157; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.ApparentPower | 16507 | float | 1.0 | telemetry | config/iec104_config.json:158; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.AphaseApparentPower | 16508 | float | 1.0 | telemetry | config/iec104_config.json:159; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.BphaseApparentPower | 16509 | float | 1.0 | telemetry | config/iec104_config.json:160; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.CphaseApparentPower | 16510 | float | 1.0 | telemetry | config/iec104_config.json:161; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.PowerFactor | 16511 | float | 1.0 | telemetry | config/iec104_config.json:162; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.AphasePowerFactor | 16512 | float | 1.0 | telemetry | config/iec104_config.json:163; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.BphasePowerFactor | 16513 | float | 1.0 | telemetry | config/iec104_config.json:164; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.CphasePowerFactor | 16514 | float | 1.0 | telemetry | config/iec104_config.json:165; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS.AphaseVoltage | 16515 | float | 1.0 | telemetry | config/iec104_config.json:166; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS.BphaseVoltage | 16516 | float | 1.0 | telemetry | config/iec104_config.json:167; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS.CphaseVoltage | 16517 | float | 1.0 | telemetry | config/iec104_config.json:168; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS.AphaseCurrent | 16518 | float | 1.0 | telemetry | config/iec104_config.json:169; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS.BphaseCurrent | 16519 | float | 1.0 | telemetry | config/iec104_config.json:170; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS.CphaseCurrent | 16520 | float | 1.0 | telemetry | config/iec104_config.json:171; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS.AphaseActPower | 16521 | float | 1.0 | telemetry | config/iec104_config.json:172; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS.BphaseActPower | 16522 | float | 1.0 | telemetry | config/iec104_config.json:173; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS.CphaseActPower | 16523 | float | 1.0 | telemetry | config/iec104_config.json:174; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS.ActivePower | 16524 | float | 1.0 | telemetry | config/iec104_config.json:175; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS.ReactivePower | 16525 | float | 1.0 | telemetry | config/iec104_config.json:176; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS.ApparentPower | 16526 | float | 1.0 | telemetry | config/iec104_config.json:177; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS.Frequency | 16527 | float | 1.0 | telemetry | config/iec104_config.json:178; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | BMS.RackVoltage | 16528 | float | 1.0 | telemetry | config/iec104_config.json:179; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | BMS.RackCurrent | 16529 | float | 1.0 | telemetry | config/iec104_config.json:180; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | BMS.RackRunState | 16530 | int | 1.0 | telemetry | config/iec104_config.json:181; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | BMS.RackConnectionState | 16531 | int | 1.0 | telemetry | config/iec104_config.json:182; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | BMS.RackCagDsgState | 16532 | int | 1.0 | telemetry | config/iec104_config.json:183; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | BMS.SOC | 16533 | int | 1.0 | telemetry | config/iec104_config.json:184; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | BMS.SOH | 16534 | int | 1.0 | telemetry | config/iec104_config.json:185; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 2 | EMS.BmsLimited | 1 | bool | 1.0 | telemetry | config/iec104_config.json:192; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 2 | EMS.SysStoped | 2 | bool | 1.0 | telemetry | config/iec104_config.json:193; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |

## Anomalies

- CA 1: IOA 16396 used by 2 points: EMS.DemandThreshold12, EMS.Pmlmax

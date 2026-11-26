# FORMAL-P0 实测时间

机器：WSL2，CBMC 5.12，Java 未参与本轮。每次运行机器负载不同，绝对秒数有波动。

| 运行 | 范围 | unwind | properties | 耗时 |
| --- | --- | --- | --- | --- |
| 主入口 | 0..16 | 17 | 1012 | 4 s |
| 主入口 | 0..64 | 65 | 1012 | 13 s |
| 主入口 | 0..260 | 261 | 1012 | 44 s / 63 s / 85 s（三次） |
| `crc_reject` | 具体 7 字节帧 | 不需要 | 1011（以该入口为根） | 2 s |
| cover | 0..16 | 17 | 1 个 cover | 3 s |
| 负例（mutation 副本） | 0..16 | 17 | — | 5 s |

`check_formal.sh --strict` 含主入口 + `crc_reject` + cover + 版本/哈希检查，实测总耗时 44–85 s，低于 300 s 总预算。负例独立运行 5 s。

回归：`ctest` 354/354 passed，326.92 s；`run_arch_gates.sh build 2bbf729e` rc=0，55 s；`run_guard_selftest.py` rc=0，38 s。

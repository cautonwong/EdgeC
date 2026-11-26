# DCU-D4-04：Class 19 设备管理只读/DAR 诚实骨架

分支 `dcu/D4-04-class19`，工作树 `/tmp/edgec-land/d4-04-class19`，基线 `feat/dcu`
`a926964b`（D4-10 已合入）。

## 背景与标准

- 表178（`docs/698_parsed.md:4911-4984`）：Class 19 属性 A1..A10、方法 1..6
  （复位、执行、数据初始化、恢复出厂、事件初始化、需量初始化）。
- 目录实例（`698_parsed.md:8027-8056`）：4300 电气设备、4307 水表、4308 气表、
  4309 热表；4307..4309 另有 OI 专属方法 127（出厂启用）、128（阀门控制）、129（机电同步）。
- 产品没有设备描述/版本/复位/初始化端口；`edge_version` 是 SDK 版本，不是 A3 的六字段
  设备身份（厂商码/软硬件版本与日期/厂家扩展），不拿来拼装冒充真值。

## 实现

- `cmd/dcu/dcu_router.h`：新增 `IC_DEVICE_MGMT 19` 与
  `OI_DEVICE_ELECTRIC/WATER/GAS/HEAT`。
- `cmd/dcu/dcu_698_object_model.c`：4 个对象行（空属性/方法掩码）；
  `k_class_max_method` 的 Class 19 上限按表178 从 2 修正为 6；
  `k_oi_max_method` 登记 4307..4309 的 127..129。
- `cmd/dcu/dcu_698_appconn.c`：4 行只读 ACL（A1..A10 rights=1、方法 1..6）。
- 设计文档：对象 120 行、ACL 106 项、对象列表 4306 B；Class 19 行 none→DAR；
  `DCU_698_DEVIATIONS.md` 增 Class 19 producer/端口缺口行。

## DAR 行为（实测断言）

| 请求 | resolver | 主站（先过 ACL） |
| --- | --- | --- |
| GET A1..A10（类定义、无来源） | DAR 5 | DAR 5（ACL 广告） |
| GET A11（类未定义） | DAR 4 | DAR 3（ACL 未广告） |
| GET 未知 OI 4301/4310 | DAR 6 | DAR 3 |
| ACTION 方法 1..6（复位/执行/数据/出厂/事件/需量） | DAR 5 | DAR 5，且绝不执行 |
| ACTION 方法 7（类未定义） | DAR 4 | DAR 3 |
| ACTION 4307..4309 方法 127..129（标准定义、无处理器） | DAR 5 | DAR 3（ACL 未广告） |
| SET A1..A10 | DAR 5（空掩码无写处理器） | DAR 3（ACL 只读） |

没有任何路径返回假成功、执行复位或清数据。注意：4307..4309 的 127..129 只在 resolver
层是 DAR 5；主站 ACTION 因 ACL 未广告先回 **DAR 3**，不是“所有类定义方法在主站都是
DAR 5”。

## 缓冲区缺陷与修复（verifier 打回）

- 复现：106 项对象列表 4306 B + 22 B 帧 = **4328 B**；从站响应原来从共享池
  `net_buf_pool_new(8192, 4096)` 分配，旧码 `b.len=4328 > b.size=4096`，越过 296 B
  写坏下一个 net_buf 节点（verifier gdb：node1 首字节被写成对象列表头，next 指针
  变成 0x0201160211020203）。`net_buf_simple_add` 的 assert 在 NDEBUG 下被编译掉，
  不能当防线。
- 修复 1（池）：`bus_orchestrator_setup_slave` 给从站单独建
  `BUS_SLAVE_POOL_BUF_SIZE = EDGE_SLAVE_AXDR_MAX + 256 = 10496 B`、32 项的池
  （约 328 KB）。共享池保持 8192×4096 ≈ 33.5 MB；若抬到 ~10.3 KB 会到约 85 MB，
  故不采用。`bus_orchestrator_destroy` 归还该池。
- 修复 2（tailroom 守卫）：`dlt698_codec.c` 的 GET response / DAR / list begin /
  list item / item DAR / list end / report frame 编码前检查剩余空间，放不下返回 -1；
  驱动单 OAD GET 失败回 DAR 2，列表项失败回该项 DAR 2，绝不越过 `b.size` 写。
- 修复 3（小池暴露的泄漏）：`slave_engine.c` 在 `write_async` 后没有归还本次分配的
  响应缓冲引用（`write_async` 自己 ref/unref），每次请求漏一个 buffer；共享池 8192
  掩盖了它，32 项池约 20 次请求后耗尽，验收出现 22 个 TimeoutError。补
  `net_buf_unref(resp)` 后验收 35/35 通过。
- 回归测试：4096 B 池上驱动 106 项 GET → 干净 DAR 2，缓冲末尾之后 64 B 保持 0；
  `BUS_SLAVE_POOL_BUF_SIZE` 池上 → 完整 4328 B 响应，末尾之后 64 B 保持 0。
  旧码在第一条断言 `out->b.len <= out->b.size` 红（4328 > 4096）。

## 第二轮：列表尾预留与半帧拒发（verifier 打回）

- 复现：`dlt698_encode_get_response_list_item` 只预留本项 5 B（OAD+choice），没给帧尾
  5 B（跟随上报|时间标签|FCS|END）留空间。生产池 usable 容量是 10432 B（`net_buf_alloc`
  预留 64 B headroom）；item1 10240 B 后剩 174 B，item2 写 165 B 后恰好剩 4 B，
  `list_end` 返回 -1，但 `b.len=10427` 的半帧仍被 `write_async` 发上线（L=00 00、无 END），
  且请求未被消费，后续每个 RX 事件重发。
- 修复：item 守卫改为 `5 + dlen + 5 <= tailroom`，item-DAR 改为 `6 + 5 <= tailroom`，
  end 永远放得下；任何 exchange 错误由驱动把 `out->b.len = 0`，引擎只在
  `rc >= 0 && len > 0` 时 `write_async`（契约：只发已封口帧）。
- 回归：`test_slave_list_item_reserves_frame_trailer`（最后一项回 DAR 2、L 与 END 自洽）
  与 `test_slave_encode_failure_clears_half_frame`（rc<0 时 len==0）。旧码分别红在
  `rc=-1` 与 `len=10427 != 0`。
- 变异守卫：`ci/mutation_guards.tsv` 新增 `DCU-D4-04-list-trailer-reserve`（把 item
  守卫改回不预留 5 B）；`run_mutation_guards.py --only` 证明变异后红（rc=8）、恢复后
  绿（rc=0）。

## 评审记录（不在本单修复）

- UDP/HTTP/OPCUA 的 `write_async` 不持有引用；slave 当前只用 poll IO，暂安全，属既有
  契约缺口，另行登记。
- `IEMS_MAX_VBUS=32` 时 32 个从站池缓冲可能耗尽并静默丢响应；DCU 单从站不受影响，
  另行登记。

## 拒绝的替代

1. **用 `edge_version` 填 A3 版本信息**：SDK 版本不是设备厂商/型号身份，六字段
   （固定长度厂商码/日期/厂家扩展）无来源；拼装即伪造，故整类回 DAR 5。
2. **方法回 DAR 2（临时失败）**：没有 transient 条件可归因；按 D5 系列“类定义但无
   producer”的统一口径回 DAR 5。
3. **把 4307..4309 的 127..129 也广告进 ACL**：每对象 `method_ids[8]`，类方法已占 6，
   加 3 个会超上限并让驱动拒绝整张 ACL。故只广告类方法，127..129 记 resolver DAR 5、
   主站 DAR 3。

## 红先绿后

- 测试提交 `5688c365`：5 个测试文件，计数 116/102→120/106 并新增 Class 19 行/ACL/DAR
  断言；旧码 5/5 目标二进制红（原文 `tests.log`：`116 != 120`、`102 != 106`、
  `-20 != -21`、driver DAR `3 != 5`）。
- 修复提交 `3788a76d`：4 行 + 类方法上限 + OI 方法 + ACL + 设计文档。
- 返工测试提交 `24b49fd5`：缓冲尺寸回归两例；旧码红
  （`out->b.len <= out->b.size is not true`，4328 > 4096）。
- 返工修复提交 `73e729fc`：从站独立响应池 + codec tailroom 守卫 + DAR 2 回退 +
  slave_engine 缓冲引用泄漏修复 + 设计文档池说明。
- 第二轮测试提交 `f1b05ff7`：列表尾预留 + 半帧拒发两例；旧码红（`rc=-1`、
  `len=10427 != 0`）。
- 第二轮修复提交 `de424dad`：item/item-DAR 预留 5 B 帧尾、驱动失败清长度、引擎只发
  已封口帧、mutation guard 登记。
- 最终 `ctest -R 'dcu|dlt698'` 45/45 绿；验收连续 3 次绿（run9/10/11，35 passed ×3）；
  mutation guard 1/1 敏感。

## 验证命令与结果

```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DPython3_EXECUTABLE=/home/edge/EdgeC/.venv/bin/python3
cmake --build build --clean-first -j4
ctest --test-dir build -R 'dcu|dlt698' --output-on-failure
python3 ci/scripts/check_build_warnings.py --log <clean-build.log> --root .
python3 ci/scripts/check_build_freshness.py --root . --build build
python3 ci/scripts/check_doc_references.py --root .
python3 ci/scripts/check_no_dynamic_memory.py --root .
python3 ci/scripts/check_public_structs.py --root .
python3 ci/scripts/check_stack_usage.py --root .
TESTPLATFORM=/home/edge/test-platform PYTHON=/home/edge/EdgeC/.venv/bin/python3 \
      ./acceptance/run.sh dcu
```

结果：clean build 0 第一方 warning；45/45 绿；六个守卫 0；mutation guard
`DCU-D4-04-list-trailer-reserve` 1/1 敏感；验收连续 3 次 `35 passed`（第二轮
run9/10/11）。返工前 run2 红：22 个 northbound TimeoutError（缓冲引用泄漏耗尽 32 项
小池）；补泄漏修复后 run3/4 绿。run1 因平台 venv 缺 pytest 未跑成（用
`PYTHON=/home/edge/EdgeC/.venv/bin/python3` 后正常）。run5 出现一次
`test_master_uses_spare_di_after_primary_failure` 时序失败（34 passed, 1 failed），
随后连续 3 次全绿，记为验收台架时序 flake，与本次代码改动无关。

## 计数与预算

- 对象表 120 行；Class 20 ACL 106 对象（95 手写 + 11 参数宏）。
- 对象列表实测 4306 B / 106 项 < `EDGE_SLAVE_AXDR_MAX 10240`。
- ACL 会话存储 106 × 16 B = 1696 B + header < `DRIVER_PRIV_SIZE 4096`（静态断言编译通过）；
  106 < `EDGE_SLAVE_ACL_MAX_OBJECTS 224`。

## 未决项

- 4307..4309 的 127..129 因 ACL 每对象方法上限 8 未广告，主站 ACTION 回 DAR 3；
  若要广告，需扩 `method_ids` 并重算 D1-21 会话预算，或裁减类方法广告。
- 无设备描述/版本/复位/初始化 producer 与平台端口；有来源后补 mask、处理器与验收。
- 计划/交接文字由 verifier 更新。

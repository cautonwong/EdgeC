# DCU-D4-04 注释自审

范围：测试提交 `5688c365` / `24b49fd5`、修复提交 `3788a76d` / `73e729fc` 的新增注释
（未跑 comment-sicko；按仓库注释纪律人工自评）。

## 保留

- `cmd/dcu/dcu_698_object_model.c` 的 Class 19 行块注释：产品无设备管理端口、空掩码让
  resolver 回 DAR 5、复位/初始化方法没有处理器绝不执行。
- `cmd/dcu/dcu_698_appconn.c` 的 `k_attrs_devmgr` 注释：127..129 只在 resolver 层是
  DAR 5，ACL 每对象方法上限 8 已被类方法占用，主站回 DAR 3；不能写成“所有类定义方法
  都 DAR 5”。
- `foundation/src/protocol/dlt698_codec.c` 的 `_frame_overhead` / `_fits` 注释：帧开销
  公式与“NDEBUG 下 assert 不是防线”的理由，代码本身看不出。
- `modules/edge_io/include/bus_orchestrator.h` 的池常量注释：10496 B 与 328 KB 的
  选择依据、共享池抬到 85 MB 不采用。
- `modules/edge_io/src/bus_orchestrator.c` 的 setup_slave 注释：从站响应可能是一整张
  对象列表，共享池按采集/上报尺寸建。
- `modules/edge_io/src/engines/slave_engine.c` 的 `write_async` 后 unref 注释：
  所有权规则（write_async 自己 ref/unref），没有它每次请求漏一个 buffer。
- `tests/test_driver_dlt698.c` 的 4096 B/生产池两例注释：4328 B 的组成与旧码红判据。

## 删除

0 条（初版 summary/comments 中“所有类定义方法 DAR 5”的表述已在本轮返工修正）。

## 第二轮新增注释

- `foundation/src/protocol/dlt698_codec.c` 的 `DLT698_GET_LIST_TRAILER_LEN` 注释：
  说明 5 = FollowReport+时间标签+FCS+END，以及“项编码必须先预留，否则最后一项写满后
  end 必失败”的原因；守卫表达式本身看不出这层因果。
- `adapters/io_drivers/src/driver_dlt698.c` 的 wrapper 注释：契约“write_async 只接收
  已封口帧”与失败清长度的理由。
- `modules/edge_io/src/engines/slave_engine.c` 的 rc<0 兜底注释：半帧绝不发上线；
  与“write_async 自己持有引用”的既有注释并列，避免读者误以为重复 unref。

## 行为核对

Class 19 对象/ACL、从站响应池、codec tailroom 守卫（含列表尾 5 B 预留）、GET DAR 2
回退、驱动半帧清长度、slave_engine 引用泄漏与只发已封口帧；`ctest -R 'dcu|dlt698'`
45/45、六项静态/文档守卫 0、mutation guard 1/1 敏感、验收连续 3 次 35/35。

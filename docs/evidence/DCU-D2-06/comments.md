# Comment Sicko pass（D2-06 diff）

范围：`git diff 1c65b293..HEAD`（D2-06 分支）。以下按实际 tree 记录；旧稿关于 banner 与 `/* 对象属性数据 */` 的删除不成立，已删除。

## 已提交的注释改写（a96bddbe）

- `cmd/dcu/dcu_698_axdr_internal.h` 的 `_take_ti`：长布局注释删除，保留一行“表 24：TI 没有内层标签。”。原注释重复下方字节布局；标准表 24 且无内层标签是协议事实。
- `cmd/dcu/dcu_698_axdr.c` 的旧同内容注释随之删除。

## 独立 Comment Sicko（本轮）

- 删除 1 处：`cmd/dcu/dcu_scheme_store_internal.h` 的共用连接说明（“init 绑定、拆分后的 TU 借用”）。它解释的是隐藏单例依赖，见 MUST KILL。
- 拒绝删除 1 处并恢复：include guard 尾注 `#endif /* DCU_SCHEME_STORE_INTERNAL_H */`。相邻头文件同格式；这是格式约定，不是有罪的 WHY。
- 旧稿声称删除的 601C/601D 分节横幅与 `/* 对象属性数据 */` 在 base 与 a96bddbe 都不存在，不作为本 diff 的删除记录。

## 保留（附理由）

- `dcu_698_axdr.h` 解码失败 DAR 7/2 与元素索引语义：公共 API 合同，签名无法表达。
- `dcu_router.h` 601C/601D 的 OI 注释：标准 A.7 与 IC 11/8，编号无法推导。
- `dcu_scheme_store.h` RECORD 类型协议合法但无存储/执行路径：头文件合同。
- `dcu_scheme_store.h` `report_apply` 的事务与返回契约：相邻 6016/6018 同风格。
- `dcu_scheme_store_internal.h` include guard 尾注：相邻头文件格式。
- `dcu_698_axdr_internal.h` “表 24：TI 没有内层标签”：标准表 24 的协议事实。
- `acceptance/dcu/tests/test_dcu_northbound.py` 601C 参数 wire-shape docstring：相邻 helper 同约定，字面量否则不可读。

## MUST KILL（开放）

- `dcu_scheme_store_storage()`：文件级可变单例访问器，拆分后的 report store 通过它取连接，隐藏依赖。D2-06-r 把 storage 显式传给 report store（组合根注入），并删除访问器。
- 连带：`dcu_scheme_store.c` 的 `_exec/_begin/_commit/_rollback` 与 internal header 的 inline 事务助手重复；D2-06-r 收敛到单一来源。
- ACL 到 44 上限；D2-08 的六个状态 OI 需要提升上限。

结论：本轮删除 1 处注释、恢复 1 处格式尾注；D2-06 应用代码未改；D2-06-r 处理隐藏单例与事务助手单一来源。

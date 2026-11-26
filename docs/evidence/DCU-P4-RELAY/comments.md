# 注释清单（DCU-P4-RELAY）

- `cmd/dcu/dcu_relay_map.h` 文件头：表 A.9 用继电器 OAD 寻址、未填 DAR 2、不按 OAD 猜继电器。
- `cmd/dcu/dcu_relay_map.c` `s_active`：进程级绑定沿用对象表单例模式，只指向聚合根拥有的表，不复制、不释放。
- `cmd/dcu/dcu_node.c` 装配处：映射默认空（storage 清零），点位表/测试经 setter 填充。
- `cmd/dcu/dcu_node.c` `dcu_node_destroy`：表随节点存储释放，先解绑；只解自己绑的那张，避免误伤另一节点。
- `cmd/dcu/dcu_node.h` `dcu_node_set_relay`：未填 DAR 2、命中未接线 DAR 5、不返回成功。
- `cmd/dcu/dcu_698_class_control.c`：两条日志分别说明“未配置映射”与“已解析但未接执行”；无 phase 叙事注释。

未删除任何既有 why 注释。

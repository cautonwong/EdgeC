# D1-21 合并后告警修复

`tests/test_dcu_698_axdr.c` 调用 `dcu_698_archive_encode_axdr` 时缺声明，增量构建
未暴露；合并树 clean build 报 1 条 `-Wimplicit-function-declaration`。

修复：加 `#include "dcu_698_object_internal.h"`（声明在
`cmd/dcu/dcu_698_object_internal.h:30`）。

验证（本分支 clean-first 全量构建）：
- `check_build_warnings.py --log /tmp/d121fix-build.log`：0 new violations。
- `ctest -R 'dcu|dlt698'`：44/44。

# Matrix(已弃用)

这套东西是 EdgeC 原来的测试执行引擎:自己起仿真基座(`server.py`,vbus unix socket +
actors + adc 仿真)、自己驱动场景(`matrix_test.py` + YAML)、用例散在 `cmd/*/matrix/`。

**测试已经改由 test-platform 跑和判。** 新入口:

```bash
./test.sh --omnis [产品]     # 包装 acceptance/run.sh
./acceptance/run.sh dcu      # 直接调
```

判据、证据、归因、门禁都在 test-platform 里(`acceptance/testplatform.sha` 锁版本)。
这里不重复实现,也不要再往这套框架加东西。

## 为什么不立刻删

`cmd/iems/matrix/` 那 7 个用例(`test_raw_io` / `test_high_freq` / `test_protection` 等)
唯一能跑的仿真基座就是 `tools/matrix/server.py`。把它们搬成 test-platform 的
`acceptance/<产品>/` 需要先重写台架(见 `acceptance/dcu/dcu_product.py` 那种产品壳)。

搬迁顺序:`cmd/smart_box/matrix/`(4 个)→ `cmd/iems/matrix/`(7 个)。搬完再删本目录。

## 现状

- CI 已经不再调用 `test.sh --omnis` 里的 matrix 路径(`ci.yml` 的 Run Acceptance 步骤)。
- `cmd/*/matrix/conftest.py` 还 `from tools.matrix.conftest import *`,依赖
  `pytest-asyncio`;平台 venv 里没有这个包,所以这批用例目前连收集都会失败。
- `test.sh` 的 `--omnis` / `--only` 已经指向 `acceptance/run.sh`;`stop_all()` 里
  pkill matrix server 的两行是收尾清理,留着是为了杀掉历史残留进程。

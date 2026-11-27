# DCU-P3-8-CATALOG-GAPS 设计

## 问题

`scripts/extract_dcu_698_catalog.py` 把 7.3 类表的方法列表压缩成上限
（`render_header` 的 `class_own_method = max(...)`，再与附录 A 的
OI 方法上限取 max），并只在附录 A 窗口解析 per-OI 方法号：

- 7.3 类表的方法集合没有逐号校验。原文里删掉中间一个方法号（例如
  Class 7 的 {1,2,4,5} 变成 {1,2,4}）时，只要最大号不越过旧值，生成器
  仍退 0 并覆盖生成物；多一个号（Class 7 增加 3）同样退 0。
- 附录 A 的 per-OI 方法号没有和 7.3 类表对照：把某 OI 行的
  `方法 127` 改成 127 以下的未定义号，生成器仍退 0，并把它写进
  `dcu698_catalog_oi_methods[]` 当作该 OI 的专属方法。

标准本身的 7.3 类表存在真实空号：Class 7/24 没有方法 3、Class 9 没有
方法 6。因此“逐号匹配”不能是连续区间规则，必须以记录下来的真实集合为
参照；真实输入（`docs/698_parsed.md`）必须继续通过，生成物逐字节不变。

## 校验规则

1. 生成器内置 `CLASS_METHODS`：26 个 7.3 类各自真实的方法号元组
   （含 Class 7/24 缺 3、Class 9 缺 6 这两个标准里的空号）。
2. 解析出的每个类，其方法号按升序排列后必须与 `CLASS_METHODS[class_id]`
   完全相等：
   - 缺号：报 `missing <号>`；
   - 多号或重复号：报 `extra <号>` / `duplicate <号>`；
   - 类集合本身有缺类/多类：报 `class <id> ...`。
   校验在写文件和 `--check` 两种模式下都先执行，失败退 1，不落盘。
3. per-OI 方法检查接上 7.3 表：附录 A 里某 OI 的方法号若小于 127，必须
   属于该 OI 所属类的 7.3 方法集合，或属于记录在案的 OI 例外
   `OI_METHOD_EXCEPTIONS`。号码 ≥127 的按标准约定是 OI 专属扩展，不参与
   类集合校验。
   - 已记录的例外：F001（Class 18，表 A.11）方法 9/10，号段在 127 以下但
     不在 7.3.18 的类表里；产品侧
     `cmd/dcu/dcu_698_object_model.c::_class_defines_method` 已有同一例外。

## 数据结构

```python
CLASS_METHODS: dict[int, tuple[int, ...]] = {1: (1, 2), ..., 7: (1, 2, 4, 5), ...}
OI_METHOD_EXCEPTIONS: dict[str, frozenset[int]] = {"F001": frozenset({9, 10})}
```

校验函数返回错误字符串列表；`main()` 在解析完成后、渲染前调用：

```python
errors = validate_class_methods(classes) + validate_oi_methods(ois, oi_methods)
if errors:
    for error in errors: print(error, file=sys.stderr)
    return 1
```

## 夹具形态

新建 `tests/test_dcu_698_catalog_method_sets.py`，沿用现有
`test_dcu_698_catalog_source_window.py` 的夹具方式：把 `docs/698_parsed.md`
复制到临时 `--root` 下，原地构造变异源，运行生成器子进程并检查退出码和
stderr。

- 缺中间号：在 Class 7 段内删除 `5．删除一个事件关联对象属性` 行，
  期望新码非零退出并报 `class 7 ... missing 5`。旧码退 0（红）。
- 多号：在 Class 7 段内 `2．执行` 行后插入 `3．注入方法 ... 可选`，
  期望新码非零退出并报 `class 7 ... extra 3`。旧码退 0（红）。
- per-OI 越界：把附录 A 中 4307 行的 `方法 127：出厂启用（参数）`
  改成 `方法 126：...`，期望新码非零退出并报 OI 4307 / 方法 126。
  旧码退 0（红）。
- 正例：真实 `docs/698_parsed.md` 运行生成器退 0，四个生成物与仓库内
  文件逐字节一致；`--check` 退 0。

## 逐字节不变承诺与验证方式

生成物共四个：
`docs/design/DCU_698_OI_CATALOG.md`、
`docs/design/DCU_698_OI_CATALOG.json`、
`docs/design/DCU_698_CLASS_RECONCILIATION.md`、
`tests/fixtures/dcu_698_catalog_expected.h`。

实现前记录四个文件的 sha256；实现后用同一条 `python3 -I
scripts/extract_dcu_698_catalog.py`（写模式）重新生成并再次计算 sha256，
要求前后完全一致，且 `--check` 退 0。差异写入
`sha256-before-after.txt`。

## 守卫

`ci/mutation_guards.tsv` 追加一行：把
`if got == list(expected):`（逐号相等判断）替换成
`if got and max(got) <= max(expected):`，恢复“只看最大号”的旧放行逻辑；
命令运行新负例测试，要求变异后测试红、恢复后测试绿。

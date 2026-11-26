# D0-16 注释审计

- 保留 `_chain_finish_visit` 的单行 why：解释为什么用 visit mask 而不是 task_due 判归属，
  这是代码无法表达的相位约束。
- 测试注释说明“提交在到期 tick、回调在非到期 tick”的场景意图；无叙述性注释。

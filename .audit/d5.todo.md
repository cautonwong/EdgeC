# D5-01 + D5-02 todo (poteto-mode Feature playbook)

1. `how` over the affected subsystem.
2. `architect` for parallel design exploration.
3. Write the throughput checkpoint as four todo items.
4. Delegate code-writing to a subagent with scope.
5. Verify on the matching surface.
6. Rebase into small, ordered commits. Stack follow-ups.
7. If the design is contested, `interrogate` before shipping.
8. Opening a PR. skip: campaign delivers branches to dcu-verifier.

- [x] Survey Class 2 (1010 family) and Class 5 (200D harmonics) OI/attr/type/scaler shapes.
- [x] Survey value sources: poller demand calculation, dcu_power_quality 2nd-31st results, storage.
- [ ] Decide per-bit advertise/DAR 5, methods DAR 5, no placeholders. BLOCKED on dcu-verifier.
- [ ] Tests + acceptance case.

## Survey evidence (2026-11-22)

### Class 2 shape

- Design: `docs/design/DCU_698_OBJECT_MODEL.md:261-265`. Attrs 1 logical name,
  2 total-and-tariff max demand array (dyn), 3 scaler/unit. Methods 1 reset,
  2 execute. DCU level `optional`. "最大需量值来自电表，DCU 不本地重算。"
- Catalog `docs/design/DCU_698_OI_CATALOG.md:133-147`: OI 1010-1013 and
  1020-1023, each `最大需量值::=double-long-unsigned`.
- Plan exit evidence: `docs/plans/2026-11-21-dcu-698-production-plan.md:572`
  "GET 1010 返回电表需量".

### Class 5 shape

- Design `docs/design/DCU_698_OBJECT_MODEL.md:279-283`. Attrs 1 logical name,
  2 A-phase n-order value group, 3 B, 4 C, 5 harmonic order n, 6 scaler/unit.
  DCU level `read`. "dcu_power_quality 计算 2 到 31 次谐波与 THD，结果写入影子，
  再由本类读出."
- Catalog 200D voltage harmonic content, 200E current harmonic content,
  `long`, unit %, scaler -2.

### Source reality

- No Class 2 or Class 5 row exists in `cmd/dcu/dcu_698_var.c:30-44` (`k_vars`),
  and no object row in `cmd/dcu/dcu_698_object_model.c`.
- No demand DI or OAD exists in the three point tables
  `cmd/dcu/dcu_driver_adapter.c:43-60, 65-81, 119-135`. The meter path cannot
  deliver 1010 today.
- Bench simulates only voltage: `acceptance/dcu/bench.py:54,64-73`,
  `acceptance/dcu/configs/devices.yaml` `simulated_645`.
- Class 5 producer: `dcu_analyze_harmonics_thd()` (`cmd/dcu/dcu_power_quality.h:108`)
  is a pure function needing a caller-supplied 2..31 component array. No caller,
  no engine instance, not in the product build, no shadow writer.
- The only demand-like state is `dcu_vertical_tasks.h` `max_demand_kw/ts_ms`,
  RAM per meter, written by `dcu_vertical_1s_sample()` from instantaneous power.
  The engine has no production caller and no tariff or phase dimension.
- Active power itself is polled and archived
  (`cmd/dcu/dcu_698_var.c:39-41`, `cmd/dcu/dcu_698_record.c:23`), so a
  DCU-computed demand would be possible, but that contradicts the design line
  "DCU 不本地重算" for Class 2.

### Consequence

Both plan exit criteria name a source that does not exist. Design assigns
Class 2 level `optional`, which permits DAR 5 in the first release. Class 5 is
level `read`, so DAR 5 conflicts with the assigned level until a spectrum
producer exists. Nothing has been coded; awaiting dcu-verifier's scope call.

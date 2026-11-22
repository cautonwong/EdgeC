# D2-03 synthesis (architect arena)

Base: candidate-3 (sonnet). Grafts: candidate-1 (opus) two-pass rule, candidate-4 (haiku) TI tag constant + ti_to_seconds + task_id-ordered A2 encoding + no stack staging array. Candidate-2 (fable) dropped: model unavailable in this provider.

Rubric scores (1-5): safety decode / standard mapping / persistence / poller / DAR+masks / simplicity
- candidate-1: 5/3/4/5/3/5 (drops advertised M1 and refuses TI 0, losing exact readback)
- candidate-3: 5/5/5/5/5/4
- candidate-4: 5/5/5/5/5/4

Decisions taken from base c3:
- `dcu_698_accept_action_axdr(omd, axdr, len)` in dcu_698_axdr.c is the single wire entry; object-table check first (DRY DAR 6/5/4), bounded cursor decode, then `dcu_task_op_t` to the thin 6012 handler.
- New `dcu_task_store.{h,c}` owns `dcu_task_config` schema on `storage->db`, the authoritative `dcu_task_cfg_t tasks[16]` mirror, SQL, transactions and rollback; poller is mutated through CRUD helpers.
- Raw TI (unit/value) persisted for exact A2 readback; `has_interval` distinguishes TI 0 (no trigger) from legacy every-round.
- Real reset: M1 resets runtime state; masks 6012 {1,127,128,129,130}, 6013 A2 only.
- Unsupported standard-valid fields refused with DAR 2.

Grafts:
- c1: decode the whole body for shape (DAR 7) before any semantic/product pass (DAR 2); no state change on any refusal.
- c4: `#define DLT698_TAG_TI 0x54` in foundation/include/dlt698_codec.h as single source; `dcu_ti_to_seconds`; 6012 A2 encoded sorted by task_id; staging storage for 16 units must not be a >512B stack array (poller/store member).
- c4: `dcu_poller_set_tasks`-style idempotence is noted but base uses per-id CRUD; the implementation must preserve clock phase for unchanged tasks and re-init only on interval change.

Open (reported to verifier, coding waits): TI byte layout (tag+raw unit+be16) and 6013 element = task_id.

## Cross-judge verdict and final call

Judge (opus, own context) scored C1 27, C3 27, C4 24 and based on C1, tie-broken because C1 uses DAR 3 for permanently unsupported fields while C3 uses DAR 2. The verifier explicitly ruled DAR 2 for that case (a master must not treat a permanent limitation as denial of the class), so the tie-break does not apply. Final base stays C3, with the judge's verified grafts folded in:
- flat TI 0x54|unit|be16 (mandatory; C1/C3 prose had nested TI, the delegate brief already fixes it)
- `_chain_finish_visit` counts only due+enabled tasks (verified bug dcu_poller.c:219-225)
- 6013 elem rule kept per verifier (0 = lowest id)
- task_id 0 reserved invalid to keep addressing unambiguous
- nested-structure handling in the acceptance six-OAD parser

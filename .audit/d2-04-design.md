# D2-04 synthesis (architect arena, final)

Base: candidate-1 (opus). Judge 29 vs 27: C1 parses every standard MS/capture/ROAD shape so malformed bodies are DAR 7, while C2 shortcut-rejected non-1 MS choices at the choice byte (DAR 2 for a shape error). Grafts from C2: reuse the existing adapter getters, and add SQL-trigger rollback tests.

Decisions:
- Classes per A.7: 6014 = 11 (集合接口类), 6015 = 8 (参数变量). OBJECT_MODEL.md:315 class-10 claim is stale; correct it in the same change. No _class_defines_method change: class 11 max method 2 is enough once 127-130 are in the mask; 1/2 -> DAR 5, 131+ -> DAR 4.
- Honored profile: capture type 0 with NULL content, depth 0, storage label 0, MS choice 1 (all user addresses), CSD OAD only. Standard-valid but unhonorable -> DAR 2 with a field-specific log. ROAD -> DAR 2 after full shape parse. Unknown CSD OAD -> DAR 2. H.5.4's literal depth 1/label 1 frame is therefore DAR 2; acceptance uses the adapted frame and says so.
- CSD translation: dcu_driver_adapter_proto_to_oad("DLT698", std_oad) -> internal tag; oad_to_proto for readback. Names are inverted for 698; verified by tests. No new wrappers.
- 6014 A2 ignores element index and returns the full array sorted by scheme id; 6015 A2 uses elem = scheme id (0 = lowest), missing DAR 6. Matches 6012/6013 precedent.
- Persistence: new dcu_scheme_store.{h,c} on storage->db: dcu_scheme_config(id, depth, capture_type, ms_kind, time_label), dcu_scheme_csd(id,pos,oad), dcu_scheme_meta(seed marker). DB-first/RAM-second inside BEGIN/COMMIT with the D2-03 file-static snapshot rollback; seed once from dcu_698_var_poll_tags; reload clears RAM first.
- References: 127 upsert that changes CSD of a scheme referenced by a live task -> DAR 3; 130 Set_CSD on referenced -> DAR 3; 128 delete referenced or missing -> DAR 6 (whole batch); 129 Clear while any scheme referenced -> DAR 3.
- Poller: upsert/remove/clear + referenced/masked predicates; remove compacts schemes (ids are identity); no new scheduler, no timer, no lock.
- Decode scratch lives in dcu_poller_t (scheme_op), never on the stack.
- Masks: 6014 attr {2}, methods {127,128,129,130}; 6015 attr {2}, no methods. ACL 18 -> 20 with a static assert against the driver cap 20. CSD wire 0x5B 0x00 + 4 raw OAD bytes (ROAD 0x5B 0x01 parsed then DAR 2); MS 0x5C + choice.
- Tests: shape DAR 7 for truncated/wrong tag/count/trailing; MS/capture/ROAD malformed bodies DAR 7; policy DAR 2; reference DAR 3/6; add scheme with 2 of 4 columns -> poller step submits exactly those 2 tags; Set_CSD changes them; delete stops; persistence reload; SQL trigger failure rolls back RAM and DB. ACTION 6014 is unreachable over the wire until D1-20a: prove via dcu_698_accept_action_axdr and tag the acceptance case 待接入.

# Computational Substrate Phase 0 Scenario Matrix

Status: active Phase 0 scenario artifact

## Purpose

This document is the checked-in mutation scenario matrix required by
workstream `0.4` of
[COMPUTATIONAL_SUBSTRATE_PHASE0_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE0_PLAN.md).

It defines the representative mutation classes that later shadow phases must
be able to observe, capture, and compare.

For each scenario, it records:

- the representative mutation
- the required captures
- the expected comparison outputs
- the owning validation lane

## Comparison Surface Key

The capture shorthand used below maps to the observable-state model in
[COMPUTATIONAL_SUBSTRATE_PHASE0_OBSERVABLE_STATE_MODEL.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE0_OBSERVABLE_STATE_MODEL.md):

- `FT`: formula-tree snapshot
- `FTr`: formula-track snapshot
- `BC`: normalized broadcaster-state snapshot
- `LR`: derived listener-registration relations
- `DI`: engine dependency snapshot versus live dirty-state comparison
- `RQ`: engine recalc queue versus live queue comparison

## Scenario Matrix

| ID | Mutation class | Representative operations | Required captures | Expected comparison outputs | Owning validation lane |
| --- | --- | --- | --- | --- | --- |
| `S1` | Scalar edit | `SetValue` on a direct precedent with both direct and transitive dependents | `BC`, `LR`, `DI`, `RQ`, `FT` | direct/area listener shape remains exact; invalidation exact or conservative superset; recalc queue exact on safe graphs | `CppunitTest_sc_ucalc_dependency_shadow` |
| `S2` | Formula text edit | `SetString` with formula content replacing scalar or prior formula | `BC`, `LR`, `FT`, `FTr`, `DI`, `RQ` | listener teardown/rebuild exact after edit; queue membership exact after rebuild; no stale listeners retained | `CppunitTest_sc_ucalc_dependency_shadow` |
| `S3` | Formula insertion | `SetFormula` on empty target introducing new precedents and dependents | `BC`, `LR`, `FT`, `FTr` | formula-tree membership exact; broadcaster/listener additions exact | `CppunitTest_sc_ucalc_dependency_shadow` |
| `S4` | Single-cell clear | `ClearCell` on precedent or dependent cell | `BC`, `LR`, `FT`, `FTr`, `DI`, `RQ` | stale listeners removed exactly; dirty/recalc state conservative or exact depending on graph shape | `CppunitTest_sc_ucalc_dependency_shadow` |
| `S5` | Range clear | `ClearRange` over cells containing formulas and precedents | `BC`, `LR`, `FT`, `FTr` | removed formula nodes leave no residual listener registrations or queue entries | `CppunitTest_sc_ucalc_dependency_shadow` |
| `S6` | Row insert/delete | insert or delete rows intersecting referenced ranges | `BC`, `LR`, `FT`, `FTr` | listener registrations normalized-equivalent or exact after reference update; queue state exact after rebuild | `CppunitTest_sc_ucalc_dependency_shadow` |
| `S7` | Column insert/delete | insert or delete columns intersecting referenced ranges | `BC`, `LR`, `FT`, `FTr` | same expectations as row structural edits | `CppunitTest_sc_ucalc_dependency_shadow` |
| `S8` | Named-range edit | rename or retarget a named range used by formulas | `DI`, `RQ`, `FT`, `BC` | invalidation and queue comparisons exact or conservative superset; no orphan listeners | `CppunitTest_sc_ucalc_dependency_shadow` |
| `S9` | Copy/move rebuild | copy/paste or move formulas so listener rebuild is required | `BC`, `LR`, `FT`, `FTr` | listener graph exact after rebuild; moved formulas do not retain old broadcaster anchors | `CppunitTest_sc_ucalc_dependency_shadow` |
| `S10` | Clipboard-driven group/listener recovery | copy/paste affecting grouped or area-listener formulas | `BC`, `LR`, `FT` | formula-group listener capture normalized-equivalent or exact by anchor and length | `CppunitTest_sc_ucalc_shared_cases` |
| `S11` | Load-time listener setup | `CalcAfterLoad` or post-import setup where formulas exist before listeners are attached | `BC`, `LR`, `FT`, `FTr` | listeners and queue state converge to exact steady-state snapshot after load completion | `CppunitTest_sc_ucalc_workbook_facade` plus targeted Calc load lane |
| `S12` | Delayed listener startup | mutations executed while listener setup is intentionally postponed | `BC`, `LR`, `FT`, `FTr` | pre-finalization captures may diverge; post-finalization capture must become exact | `CppunitTest_sc_ucalc_dependency_shadow` |
| `S13` | Delayed broadcaster deletion | empty-broadcaster purge after listener teardown | `BC`, `LR` | pre-purge normalized-equivalent allowed only where explicitly documented; post-purge exact | `CppunitTest_sc_ucalc_dependency_shadow` |

## Phase 0 Implementation Priorities

Not every scenario above needs to be automated in the first validation patch.
Phase 0 should proceed in this order:

1. `S1` scalar edit
2. `S2` formula text edit
3. `S4` single-cell clear
4. `S13` delayed broadcaster deletion
5. `S6` or `S7` structural edit
6. `S8` named-range edit

That ordering is intentional:

- it covers the minimum required mutation set early
- it exercises both listener addition and listener teardown
- it gives Phase 0 a real differential lane before expanding to harder
  structural and load-time cases

## Owning Lane Notes

### `CppunitTest_sc_ucalc_dependency_shadow`

This is the primary Phase 0 owner because it already compares engine
invalidation and recalc plans against live Calc state. It is the right place
for:

- formula tree captures
- formula track captures
- normalized broadcaster-state comparisons
- dependency and recalc correspondence checks

### `CppunitTest_sc_ucalc_workbook_facade`

This remains the secondary owner for scenarios that depend on faithful
document-surface projection but not on deep listener graph assertions.

### `CppunitTest_sc_ucalc_shared_cases`

This is reserved for scenarios where listener or broadcaster effects interact
with already-extracted shared execution semantics, especially formula-group and
copy/paste style cases.

## Exit Implication

Phase 0 should not claim completion until:

- each required mutation class in the plan maps to a checked-in scenario row
- every row has a named capture set
- every row has a named owning lane
- the first implemented automated lane is chosen from this matrix instead of
  being invented ad hoc

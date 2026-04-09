# Computational Substrate Primitive Realization And Rollback Decision Record

Status: completed closeout decision

## Decision

Proceed with engine-authored admitted-slice primitive realization and
rollback migration on the bounded slice.

This is a bounded proceed decision, not a broad `ScDocument` realization or
rollback replacement.

## What The Evidence Justifies

The completed proof cycle now justifies one additional settled boundary
shift on the admitted slice:

- the engine already owns admitted resident cell storage
- the engine already owns admitted resident wiring containers
- the engine already owns admitted formula-cell lifetime decisions
- the engine already owns admitted scalar mutation-entry request shape,
  routing, and after-state decisions
- the engine already owns admitted raw mutation records
- the engine already owns admitted raw document mutation records and
  primitive apply verdicts
- the engine already owns admitted live object-realization records
- the engine already owns admitted rollback records
- the engine already owns admitted live apply plans
- the engine now also owns the explicit admitted primitive realization and
  primitive rollback records and apply verdicts consumed by Calc before the
  retained final verification host shell completes

The admitted exact result is stronger than the earlier host-shell shape
because the live host shell no longer only realizes and restores admitted
state through inline local intent. It now consumes explicit engine-authored
primitive realization and rollback records first.

## What Still Stays In Calc

This closeout does not justify broad host independence. Calc still owns:

- the final verification host shell around admitted primitive realization
  and rollback execution
- all workbook and mutation classes outside the admitted slice

That remaining host role is narrower than before this cycle, but it is still
real.

## Why This Is A Proceed Rather Than Hybrid

The evidence supports proceed because all of the following now hold on the
admitted slice:

- admitted primitive realization identity is built into an explicit
  value-semantic record before live execution
- admitted primitive rollback identity is built into an explicit
  value-semantic record before live restore
- admitted applied lanes close with
  `PrimitiveRealizationObservationKind::Exact`
- admitted dirty-baseline rollback lanes close with
  `PrimitiveRollbackObservationKind::Exact`
- queue, computational, graph, and replay baselines remain green
- the retained host shell is meaningfully smaller without widening the
  admitted workbook or mutation surface

This is strong enough to treat the admitted primitive realization and
primitive rollback records and apply verdicts as part of the settled
engine-authored boundary on the bounded slice.

## Deferred Or Out-Of-Contract Classes

The following remain explicitly out of contract after this closeout:

- shared-group-sensitive primitive realization or rollback classes
- named-range-sensitive primitive realization or rollback classes
- off-sheet or sheet-wide structural primitive realization or rollback
- broad final verification migration
- broad `ScDocument` host independence

## Next Adjacent Concern

The next adjacent concern after this closeout is:

- final verification host-shell reassessment on the admitted slice

The engine now owns the admitted resident, mutation-entry, raw-mutation,
raw-document-mutation, object-realization, rollback, live-apply, primitive
realization, and primitive rollback surfaces that were the main
prerequisites for that question. What remains host-owned on the bounded
slice is no longer mutation intent, realization intent, or rollback intent.
It is the retained final verification host shell around that admitted
engine-authored state.

## Validation Used For This Decision

The closeout stayed green on:

- `CppunitTest_sc_ucalc_dependency_shadow`
- `CppunitTest_sc_ucalc_workbook_facade`
- `CppunitTest_sc_ucalc_compile_diff`
- `spreadsheetengine_computational_graph_tests`
- `spreadsheetengine_computational_substrate_tests`
- `spreadsheetengine_workbook_facade_tests`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

Standing replay baseline remains exact:

- `workbooks=500`
- `formula_cells=50661`
- `parsed_formulas=50652`
- `cached_fallback_cells=0`
- `cached_fallback_rate=0`

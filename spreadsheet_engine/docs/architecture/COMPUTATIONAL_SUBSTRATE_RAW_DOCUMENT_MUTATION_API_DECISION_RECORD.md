# Computational Substrate Raw Document Mutation API Decision Record

Status: completed closeout decision

## Decision

Proceed with engine-authored admitted-slice raw document mutation migration
on the bounded slice.

This is a bounded proceed decision, not a broad `ScDocument` mutation API
replacement.

## What The Evidence Justifies

The completed proof cycle now justifies one additional settled boundary shift
on the admitted slice:

- the engine already owns admitted resident cell storage
- the engine already owns admitted resident wiring containers
- the engine already owns admitted formula-cell lifetime decisions
- the engine already owns admitted scalar mutation-entry request shape,
  routing, and after-state decisions
- the engine already owns admitted live object-realization records
- the engine already owns admitted rollback records
- the engine already owns admitted raw mutation records
- the engine already owns admitted live apply plans
- the engine now also owns the explicit admitted raw document mutation
  record and primitive apply verdict consumed by Calc before the retained
  primitive host shell completes

The admitted exact result is stronger than the earlier host-primitive shape
because the live host shell no longer only executes admitted scalar and
narrow structural mutations through inline local intent. It now consumes one
explicit engine-authored primitive mutation record first.

## What Still Stays In Calc

This closeout does not justify broad host independence. Calc still owns:

- the primitive realization host operations around admitted primitive
  execution
- the primitive rollback host operations around admitted primitive
  execution
- the final verification host shell around those primitive operations
- all workbook and mutation classes outside the admitted slice

That remaining host role is narrower than before this cycle, but it is still
real.

## Why This Is A Proceed Rather Than Hybrid

The evidence supports proceed because all of the following now hold on the
admitted slice:

- admitted primitive mutation identity is built into an explicit
  value-semantic record before live execution
- admitted scalar, formula, clear, and narrow structural lanes close with
  `RawDocumentMutationObservationKind::Exact`
- dirty-baseline rejection and rollback still carry explicit primitive
  mutation identity and close cleanly on the bounded slice
- queue, computational, graph, and replay baselines remain green
- the retained host shell is meaningfully smaller without widening the
  admitted workbook or mutation surface

This is strong enough to treat the admitted raw document mutation record and
primitive apply verdict as part of the settled engine-authored boundary on
the bounded slice.

## Deferred Or Out-Of-Contract Classes

The following remain explicitly out of contract after this closeout:

- shared-group-sensitive primitive mutation classes
- named-range-sensitive primitive mutation classes
- off-sheet or sheet-wide structural primitive mutation
- broad primitive realization or rollback migration
- broad `ScDocument` mutation API replacement
- broad `ScDocument` host independence

## Next Adjacent Concern

The next adjacent concern after this closeout is:

- primitive realization and rollback shell reassessment on the admitted
  slice

The engine now owns the admitted resident, mutation-entry, raw-mutation,
raw-document-mutation, object-realization, rollback-record, and live-apply
surfaces that were the main prerequisites for that question. What remains
host-owned on the bounded slice is no longer mutation identity or primitive
mutation intent. It is the retained primitive realization and rollback shell
around that admitted engine-authored state.

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

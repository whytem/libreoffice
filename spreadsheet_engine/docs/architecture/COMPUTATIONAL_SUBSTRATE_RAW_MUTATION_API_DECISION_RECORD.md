# Computational Substrate Raw Mutation API Decision Record

Status: completed closeout decision

## Decision

Proceed with engine-authored admitted-slice raw mutation migration on the
bounded slice.

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
- the engine now also owns the explicit admitted raw mutation record
  consumed by Calc before live apply

The admitted exact result is stronger than the earlier hybrid raw mutation
shape because the live host shell no longer reconstructs admitted mutation
identity inline at the main mutation-entry call site. It now consumes one
explicit engine-authored record first.

## What Still Stays In Calc

This closeout does not justify broad host independence. Calc still owns:

- the underlying raw document mutation APIs used to execute the admitted
  record
- the final live apply shell that executes engine-authored raw mutation,
  realization, and rollback records
- all workbook and mutation classes outside the admitted slice

That remaining host role is narrower than before this cycle, but it is still
real.

## Why This Is A Proceed Rather Than Hybrid

The evidence supports proceed because all of the following now hold on the
admitted slice:

- admitted raw mutation identity is built into an explicit value-semantic
  record before live apply
- admitted scalar, formula, clear, and narrow structural lanes close with
  `RawMutationObservationKind::Exact`
- dirty-baseline rejection and rollback still close with exact rollback
  observation through the raw mutation path
- queue, computational, graph, and replay baselines remain green
- the retained host shell is meaningfully smaller without widening the
  admitted workbook or mutation surface

This is strong enough to treat the admitted raw mutation record as part of
the settled engine-authored boundary on the bounded slice.

## Deferred Or Out-Of-Contract Classes

The following remain explicitly out of contract after this closeout:

- shared-group-sensitive raw mutation classes
- named-range-sensitive raw mutation classes
- off-sheet or sheet-wide structural raw mutation
- broad `ScDocument` mutation API replacement
- broad live apply-shell migration
- broad `ScDocument` host independence

## Next Adjacent Concern

The next adjacent concern after this closeout is:

- broader live apply-shell reassessment on the admitted slice

The engine now owns the admitted resident, mutation-entry, realization,
rollback, and raw-mutation-record surfaces that were the main prerequisites
for that question. What remains host-owned on the bounded slice is no longer
mutation identity. It is the retained live apply shell around that admitted
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

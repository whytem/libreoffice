# Computational Substrate Primitive Execution Host Decision Record

Status: completed closeout decision

## Decision

Proceed with engine-authored admitted-slice primitive execution migration on
the bounded slice.

This is a bounded proceed decision, not a broad `ScDocument` primitive
execution replacement.

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
- the engine already owns admitted primitive realization and primitive
  rollback records and apply verdicts
- the engine already owns admitted final verification records and
  observations
- the engine now also owns the explicit admitted primitive execution plan
  and observation consumed before the retained primitive host shell closes
  the admitted path

The admitted exact result is stronger than the earlier host-primitive shape
because the live host shell no longer only carries primitive execution
identity through implicit local sequencing between raw document mutation,
primitive realization or rollback, and final verification. It now consumes
one explicit engine-authored primitive execution plan first.

## What Still Stays In Calc

This closeout does not justify broad host independence. Calc still owns:

- the primitive host calls that execute admitted low-level document
  mutation, realization, and rollback work
- all workbook and mutation classes outside the admitted slice

That remaining host role is narrower than before this cycle, but it is still
real.

## Why This Is A Proceed Rather Than Hybrid

The evidence supports proceed because all of the following now hold on the
admitted slice:

- admitted primitive execution identity is built into an explicit
  value-semantic plan before the retained host shell completes
- applied admitted mutation-entry lanes close with
  `PrimitiveExecutionObservationKind::Exact`
- dirty-baseline rollback lanes close with
  `PrimitiveExecutionObservationKind::Exact`
- queue, computational, graph, and replay baselines remain green
- the retained primitive host shell is meaningfully smaller without
  widening the admitted workbook or mutation surface

This is strong enough to treat the admitted primitive execution plan and
observation as part of the settled engine-authored boundary on the bounded
slice.

## Deferred Or Out-Of-Contract Classes

The following remain explicitly out of contract after this closeout:

- shared-group-sensitive primitive execution classes
- named-range-sensitive primitive execution classes
- off-sheet or sheet-wide structural primitive execution
- broad primitive host-call replacement outside the admitted slice
- broad `ScDocument` host independence

## Next Adjacent Concern

The next adjacent concern after this closeout is:

- primitive host-call executor reassessment on the admitted slice

The engine now owns the admitted resident, mutation-entry, raw-mutation,
raw-document-mutation, live-apply, primitive-realization, primitive-
rollback, final-verification, and primitive-execution surfaces that were the
main prerequisites for that question. What remains host-owned on the bounded
slice is no longer primitive execution identity or stage sequencing. It is
the retained primitive host-call executor around that admitted
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

# Computational Substrate Final Verification Host-Shell Decision Record

Status: completed closeout decision

## Decision

Proceed with engine-authored admitted-slice final verification migration on
the bounded slice.

This is a bounded proceed decision, not a broad `ScDocument` verification
replacement.

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
- the engine now also owns the explicit admitted final verification record
  and observation consumed before the retained host shell closes the
  admitted path

The admitted exact result is stronger than the earlier host-verification
shape because the live host shell no longer only accepts admitted state
through implicit local verification sequencing. It now consumes one explicit
engine-authored verification record first.

## What Still Stays In Calc

This closeout does not justify broad host independence. Calc still owns:

- the primitive execution host operations around admitted mutation,
  realization, and rollback execution
- all workbook and mutation classes outside the admitted slice

That remaining host role is narrower than before this cycle, but it is still
real.

## Why This Is A Proceed Rather Than Hybrid

The evidence supports proceed because all of the following now hold on the
admitted slice:

- admitted final verification identity is built into an explicit
  value-semantic record before the retained host shell completes
- applied admitted mutation-entry lanes close with
  `FinalVerificationObservationKind::Exact`
- dirty-baseline rollback lanes close with
  `FinalVerificationObservationKind::Exact`
- queue, computational, graph, and replay baselines remain green
- the retained host shell is meaningfully smaller without widening the
  admitted workbook or mutation surface

This is strong enough to treat the admitted final verification record and
observation as part of the settled engine-authored boundary on the bounded
slice.

## Deferred Or Out-Of-Contract Classes

The following remain explicitly out of contract after this closeout:

- shared-group-sensitive final verification classes
- named-range-sensitive final verification classes
- off-sheet or sheet-wide structural verification
- broad primitive execution migration
- broad `ScDocument` host independence

## Next Adjacent Concern

The next adjacent concern after this closeout is:

- primitive execution host-operation reassessment on the admitted slice

The engine now owns the admitted resident, mutation-entry, raw-mutation,
raw-document-mutation, object-realization, rollback, live-apply, primitive
realization, primitive rollback, and final verification surfaces that were
the main prerequisites for that question. What remains host-owned on the
bounded slice is no longer verification identity. It is the retained
primitive execution shell around that admitted engine-authored state.

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

# Computational Substrate Live Apply-Shell Decision Record

Status: completed closeout decision

## Decision

Proceed with engine-authored admitted-slice live apply-shell migration on the
bounded slice.

This is a bounded proceed decision, not a broad `ScDocument` host
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
- the engine now also owns the explicit admitted live apply plan consumed by
  Calc before exact verification completes

The admitted exact result is stronger than the earlier hybrid apply-shell
shape because the live host shell no longer only sequences raw mutation,
realization, verification, and rollback inline at the main mutation-entry
call site. It now consumes one explicit engine-authored plan first.

## What Still Stays In Calc

This closeout does not justify broad host independence. Calc still owns:

- the underlying raw document mutation APIs used to execute the admitted
  plan stages
- the primitive realization and rollback host operations consumed by that
  plan
- the final verification host shell around those primitive operations
- all workbook and mutation classes outside the admitted slice

That remaining host role is narrower than before this cycle, but it is still
real.

## Why This Is A Proceed Rather Than Hybrid

The evidence supports proceed because all of the following now hold on the
admitted slice:

- admitted stage ordering is built into an explicit value-semantic apply plan
- applied admitted scalar, formula, and narrow structural lanes close with
  `LiveApplyObservationKind::Exact`
- dirty-baseline rejection and rollback still close with exact live-apply
  observation through the apply-plan path
- queue, computational, graph, and replay baselines remain green
- the retained host shell is meaningfully smaller without widening the
  admitted workbook or mutation surface

This is strong enough to treat the admitted live apply plan as part of the
settled engine-authored boundary on the bounded slice.

## Deferred Or Out-Of-Contract Classes

The following remain explicitly out of contract after this closeout:

- shared-group-sensitive live apply classes
- named-range-sensitive live apply classes
- off-sheet or sheet-wide structural live apply
- broad raw document mutation API replacement
- broad `ScDocument` host independence

## Next Adjacent Concern

The next adjacent concern after this closeout is:

- raw document mutation API migration on the admitted slice

The engine now owns the admitted resident, mutation-entry, realization,
rollback, raw-mutation, and live-apply-plan surfaces that were the main
prerequisites for that question. What remains host-owned on the bounded slice
is no longer stage identity or stage ordering. It is the underlying raw
document mutation shell that still executes the admitted plan.

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

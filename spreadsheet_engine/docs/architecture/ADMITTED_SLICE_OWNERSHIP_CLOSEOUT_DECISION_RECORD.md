# Admitted Slice Ownership Closeout Decision Record

Status: completed closeout decision

## Decision

Proceed with substantive ownership completion on the current admitted slice.

This is a bounded completion decision, not a broad `ScDocument`
independence claim.

## What The Evidence Justifies

The completed proof cycle now justifies one additional settled boundary
shift on the admitted slice:

- the engine already owns admitted resident cell storage
- the engine already owns admitted resident wiring containers
- the engine already owns admitted formula-cell lifetime decisions
- the engine already owns admitted mutation-entry request shape, routing,
  and after-state decisions
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
- the engine already owns admitted primitive execution plans and
  observations
- the engine now also owns the explicit admitted primitive host-call
  executor plan and observation consumed by the verified mutation-entry
  result on the bounded slice

That means the current admitted slice is now ownership-complete in the
substantive architectural sense this program has been targeting: the engine
owns the identity of storage, wiring, lifetime, mutation, realization,
rollback, verification, primitive execution, and the retained primitive
host-call shell on that slice.

## What Still Stays In Calc

This closeout does not justify broad host independence. Calc still owns:

- the thin primitive host-call adapter that executes low-level admitted
  document mutation, realization, and rollback calls
- all workbook and mutation classes outside the admitted slice

That remaining role is now a bounded host adapter role, not hidden admitted
authority on the current slice.

## Why This Is Completion Rather Than Hybrid

The evidence supports admitted-slice ownership completion because all of the
following now hold on the bounded slice:

- admitted primitive host-call identity is explicit and engine-authored
- admitted apply lanes and dirty-baseline rollback lanes both carry an
  explicit primitive host-call executor plan
- admitted apply lanes and dirty-baseline rollback lanes both classify the
  primitive host-call executor observation as `Exact`
- the verified admitted mutation-entry result now consumes that executor
  observation rather than treating it as passive diagnostics
- queue, computational, graph, replay, realization, rollback, and final
  verification baselines remain green
- the retained Calc role is meaningfully smaller without widening the
  admitted workbook or mutation surface

This is strong enough to say the remaining admitted-slice work is no longer
"finish ownership on the current slice." The current slice is substantively
complete. The roadmap now shifts to widening that slice safely.

## Deferred Or Out-Of-Contract Classes

The following remain explicitly out of contract after this closeout:

- shared-group-sensitive workbook classes
- named-range-sensitive structural classes
- off-sheet or sheet-wide structural classes
- copy, move, clipboard, load-time, or undo-like flows
- workbook or mutation classes outside the admitted scalar and narrow
  structural slice
- broad `ScDocument` host independence

## Next Roadmap Category

The next roadmap category after this closeout is:

- widening the admitted slice on top of an ownership-complete boundary

The next explicit plans should therefore target broader workbook classes,
not another current-slice ownership seam. Shared-group-sensitive behavior
and other harder workbook-class widening fronts are now the relevant next
questions.

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

Recorded bounded sample:

- `dependency_shadow elapsed=7.01`
- `dependency_shadow rss_kb=248612`

Standing replay baseline remains exact:

- `workbooks=500`
- `formula_cells=50661`
- `parsed_formulas=50652`
- `cached_fallback_cells=0`
- `cached_fallback_rate=0`

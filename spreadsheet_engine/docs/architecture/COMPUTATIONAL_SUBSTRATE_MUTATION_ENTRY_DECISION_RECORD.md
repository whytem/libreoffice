# Computational Substrate Mutation Entry Decision Record

Status: completed closeout decision

## Decision

Keep admitted-slice mutation entry **validation-only**.

This cycle does **not** promote direct mutation entry into the settled
engine-owned boundary.

## Why

The completed proof cycle did justify:

- engine-owned admitted mutation request records
- engine-owned request-to-path classification
- engine-owned admitted after-state decisions for scalar, lifecycle, and
  admitted structural entry
- Calc-side apply, realization, verification, and rollback from that
  engine-owned entry surface
- exact live formula-entry proof on the admitted slice
- exact live admitted structural-entry proof on the admitted slice
- explicit dirty-baseline rejection and rollback

It did **not** justify settled live admission across the whole admitted
mutation-entry surface because the scalar-entry proof still closes with one
remaining live realization gap:

- queue comparison stays exact
- graph comparison stays exact and full-match
- IR comparison stays accepted
- the computational comparison is still not a full match because live
  broadcaster canonicalization does not yet converge exactly

That is materially narrower than a true graph or queue divergence, but it is
still enough to block a clean “proceed” decision for the full admitted
mutation-entry slice.

## Settled Boundary After This Closeout

After this decision, the settled boundary is:

- the engine owns admitted resident cell storage
- the engine owns admitted resident wiring containers
- the engine owns admitted formula-cell lifetime decisions
- the engine owns admitted mutable computational state plus graph, wiring,
  and queue decisions
- the engine owns a validation-only admitted mutation-entry surface
- Calc still owns the settled live mutation-entry boundary
- Calc still owns live realization and final rollback

So the project is stronger than the earlier
engine-owned-state-plus-host-entry split, but it is not yet justified to
claim direct admitted mutation entry as part of the settled live boundary.

## Next Adjacent Concern

The next adjacent concern should be **another newly bounded reassessment**,
not a broad boundary jump.

The most actionable next target is:

- admitted-slice scalar-entry broadcaster-canonicalization convergence

That next reassessment should ask one narrow question:

- can live Calc broadcaster realization for direct scalar mutation entry be
  brought into exact computational alignment with the already-exact queue and
  graph results?

Broader object-realization migration or broader dependency-container
migration should wait until that narrower scalar-entry gap is either closed
or explicitly judged acceptable as a stable comparison boundary.

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

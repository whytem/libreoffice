# Computational Substrate Scalar Mutation Entry Decision Record

Status: completed closeout decision

## Decision

Proceed with admitted-slice scalar mutation entry as part of the settled live
boundary.

This closeout removes the last blocker recorded in
[COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_DECISION_RECORD.md)
for the bounded direct mutation-entry surface.

## Why

The completed scalar convergence cycle now proves, on the admitted scalar
slice:

- exact queue verification
- exact computational verification
- exact graph verification
- exact broadcaster canonicalization after live realization
- retained dirty-baseline rejection
- retained rollback-triggering coverage

The earlier scalar blocker no longer remains:

- the scalar path no longer closes with `missing_expected_broadcasters`
- the authority after-state now carries the dependency-derived broadcaster
  surface that the graph path already proved
- the live scalar mutation-entry comparison is now fully exact

That means the scalar path no longer justifies keeping the bounded direct
mutation-entry surface validation-only.

## Settled Boundary After This Closeout

After this decision, the admitted live boundary on the bounded slice now
includes:

- engine-owned admitted scalar mutation-entry request shape and routing
- engine-owned admitted scalar after-state decisions
- exact direct scalar mutation-entry verification after Calc realization

The already-proven direct formula-entry and admitted structural-entry proof
lanes remain green.

Calc still intentionally owns:

- raw document mutation APIs
- live object realization
- final live verification and rollback
- all workbook and mutation classes outside the admitted slice

So this closeout strengthens the settled live boundary, but it does not claim
broad host independence for mutation application.

## What This Does Not Claim

This decision does not claim:

- named-range-sensitive mutation-entry widening
- shared-group-sensitive mutation-entry widening
- sheet-wide mutation-entry widening
- broad object-realization migration
- rollback migration out of Calc
- token-container or broad storage migration

Those concerns remain separate.

## Next Adjacent Concern

The next adjacent concern should be:

- broader object-realization reassessment

Now that the admitted scalar mutation-entry blocker is closed, the most
actionable remaining host-owned surface on the bounded slice is not request
classification or after-state prediction. It is Calc-hosted live object
realization and final rollback.

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

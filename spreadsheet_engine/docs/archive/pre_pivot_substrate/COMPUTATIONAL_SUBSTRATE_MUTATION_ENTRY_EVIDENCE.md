# Computational Substrate Mutation Entry Evidence

Status: implemented evidence

## Purpose

This note records the bounded proof results for
[COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_PLAN.md)
after the admitted mutation-entry implementation and Calc realization path
landed.

## Proven Live Cases

The admitted mutation-entry wrapper now has green live proof lanes for:

- `testComputationalMutationEntrySetValue`
- `testComputationalMutationEntrySetFormula`
- `testComputationalMutationEntryInsertRows`
- `testComputationalMutationEntryRejectsDirtyBaseline`
- `testComputationalMutationEntryRuntimeExplicitGate`
- `testComputationalMutationEntryClassifiesRepairDetected`

These lanes exercise:

- admitted scalar entry
- admitted formula lifecycle entry
- admitted single-sheet structural entry
- explicit dirty-baseline rejection and rollback
- explicit runtime gating
- explicit repair-detected classification

## Differential Results

### Set Scalar Value

The bounded scalar-entry proof is:

- result kind: `Applied`
- queue comparison: exact
- graph comparison: exact and full-match
- IR comparison: accepted
- computational comparison: not a full match

The only observed computational divergence in this bounded scalar-entry case
is broadcaster canonicalization on the live Calc side. The frozen proof lane
now checks that the scalar-entry comparison shape is:

- cell population match
- formula tree match
- formula track match
- broadcaster match: false
- formula group match
- named-range match

This is materially narrower than a true dependency or queue mismatch: the
engine-owned mutation entry, resident cell state, graph state, and queue state
all agree, while live broadcaster materialization still normalizes
differently.

### Set Formula

The bounded formula-entry proof is:

- result kind: `Applied`
- queue comparison: exact
- computational comparison: full match
- graph comparison: exact and full-match
- IR comparison: accepted

This is the cleanest proof in the current mutation-entry cycle because the
engine-owned entry request, resident after-state, live formula-cell
realization, and post-realization verification all converge exactly.

### Insert Rows

The bounded structural-entry proof is:

- result kind: `Applied`
- queue comparison: exact
- computational comparison: full match
- graph comparison: exact and full-match
- IR comparison: accepted

This keeps the current mutation-entry cycle anchored to the already-admitted
single-sheet structural slice rather than widening into deferred named-range,
shared-group, or sheet-wide classes.

## Rollback And Reject Lanes

The explicit reject/rollback proof is:

- `testComputationalMutationEntryRejectsDirtyBaseline`

That lane verifies:

- the mutation-entry wrapper still refuses dirty baselines
- the raw host mutation does not survive the rejected apply
- the captured formula-state snapshot is restored exactly

Repair-detected behavior is still explicit through
`testComputationalMutationEntryClassifiesRepairDetected`, even though this
mutation-entry cycle did not promote a live repair-detected workbook slice.

## Performance And Memory Observations

Representative single-test timings on the current machine:

- `testComputationalMutationEntrySetValue`: `elapsed=7.50s`, `rss_kb=248696`
- `testComputationalMutationEntrySetFormula`: `elapsed=7.57s`, `rss_kb=248756`
- `testComputationalMutationEntryInsertRows`: `elapsed=5.05s`, `rss_kb=248760`
- `testComputationalMutationEntryRejectsDirtyBaseline`: `elapsed=7.71s`, `rss_kb=250732`

These are proof-lane measurements, not benchmark-grade throughput claims, but
they show that the admitted mutation-entry wrapper remains in the same runtime
and memory envelope as the rest of the admitted computational-substrate test
lanes.

## Cleanliness Assessment

This mutation-entry layer is materially cleaner than the prior
engine-owned-state-plus-host-entry boundary because:

- admitted request normalization now lives in one engine-owned request shape
- request-to-path classification is engine-owned
- admitted after-state decisions are engine-owned before Calc verification
- Calc is reduced to raw apply, live realization, verification, and rollback

The remaining caveat is equally clear:

- scalar-entry still inherits one broadcaster-canonicalization gap at live
  realization time

So the evidence supports a stronger mutation-entry boundary than before, but
not yet a fully exact computational equivalence claim across every admitted
mutation class.

## Standing Validation

The implementation and evidence currently keep the following green:

- `CppunitTest_sc_ucalc_dependency_shadow`
- `CppunitTest_sc_ucalc_workbook_facade`
- `CppunitTest_sc_ucalc_compile_diff`
- `spreadsheetengine_computational_graph_tests`
- `spreadsheetengine_computational_substrate_tests`
- `spreadsheetengine_workbook_facade_tests`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

Replay baseline remains exact:

- `workbooks=500`
- `formula_cells=50661`
- `parsed_formulas=50652`
- `cached_fallback_cells=0`
- `cached_fallback_rate=0`

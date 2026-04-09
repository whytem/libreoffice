# Computational Substrate Raw Mutation API Evidence

Status: frozen raw-mutation evidence

## Purpose

This note records the bounded proof results for
[COMPUTATIONAL_SUBSTRATE_RAW_MUTATION_API_MIGRATION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_RAW_MUTATION_API_MIGRATION_PLAN.md)
after the engine-authored raw mutation record and Calc consumption path
landed.

The question for this cycle is whether the admitted raw mutation shell now
shrinks in a meaningful way without weakening exact proof on the admitted
slice.

## Exact Applied Lanes

The landed exact proof lanes are:

- `testComputationalMutationEntrySetValue`
- `testComputationalMutationEntrySetFormula`
- `testComputationalMutationEntryInsertRows`

On those admitted lanes, the raw mutation surface now proves:

- an admitted raw mutation record is built before live apply
- that record is carried in `moRawMutationRecord`
- live apply through the raw mutation compat layer succeeds
- `moRawMutationObservation` classifies as `Exact`
- queue comparison remains exact
- computational comparison remains a full match
- graph comparison remains exact and full-match

That means the admitted raw mutation shell is no longer implicit at the main
mutation-entry call site. The bounded live host apply now consumes one
engine-authored record first.

## Rollback And Reject Lane

The bounded rollback and reject proof lane is:

- `testComputationalMutationEntryRejectsDirtyBaseline`

That lane verifies:

- the admitted raw mutation shell still refuses a dirty baseline
- the raw mutation does not survive the rejected path
- rollback restores the captured admitted before-state exactly
- `moRawMutationObservation` classifies the rolled-back outcome as `Exact`

This matters because the raw mutation workstream does not succeed by making
apply exact while weakening the admitted restore path.

## Deferred And Out-Of-Contract Reading

The raw mutation cycle still remains intentionally bounded.

The standing classifier coverage from the observation workstream continues to
keep these families explicit:

- ordering-only shell drift
- hidden host mutation reconstruction
- missing realized or rolled-back objects
- queue or state mismatch
- out-of-contract mutation records

No new workbook or mutation classes were admitted in order to make the raw
mutation path look exact.

## Host-Shell Reduction Assessment

This cycle materially shrinks the remaining host shell because:

- admitted mutation identity is now frozen as a value-semantic raw mutation
  record before live apply
- admitted scalar, formula, clear, and narrow structural entry no longer
  rely on inline request reinterpretation at the main mutation-entry call
  site
- the existing realization and rollback path now consumes an already-built
  raw mutation record instead of a hidden host-originated mutation shell

The host shell is not gone. Calc still executes:

- the live apply host for the admitted raw mutation record
- the final realization host
- the final rollback host

But the admitted mutation shell is meaningfully smaller than before this
cycle because mutation identity is now engine-authored and explicit.

## Bounded Runtime Samples

Focused proof-lane samples on the current machine:

- `testComputationalMutationEntrySetValue`: `elapsed=7.69s`, `rss_kb=248760`
- `testComputationalMutationEntryInsertRows`: `elapsed=7.71s`, `rss_kb=248716`

These are bounded proof-lane measurements, not throughput benchmarks. They
show that the explicit raw mutation record does not introduce an obvious
runtime or memory cliff on the exercised admitted slice.

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

## Evidence Reading

The evidence supports one bounded conclusion:

- admitted-slice raw mutation identity is now explicitly engine-authored and
  exact on the exercised admitted lanes

The remaining closeout question is not whether the raw mutation record works.
It is whether that narrower host shell is strong enough to count as a
settled boundary shift or should still be described as a hybrid raw mutation
apply path.

# Computational Substrate Live Apply-Shell Evidence

Status: frozen live-apply evidence

## Purpose

This note records the bounded proof results for
[COMPUTATIONAL_SUBSTRATE_LIVE_APPLY_SHELL_REASSESSMENT_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_LIVE_APPLY_SHELL_REASSESSMENT_PLAN.md)
after the engine-authored live apply-plan path landed.

The question for this cycle is whether the admitted live apply shell now
shrinks in a meaningful way without weakening exact proof on the admitted
slice.

## Exact Applied Lanes

The landed exact proof lanes are:

- `testComputationalMutationEntrySetValue`
- `testComputationalMutationEntrySetFormula`
- `testComputationalMutationEntryInsertRows`

On those admitted lanes, the live apply-shell surface now proves:

- an admitted live apply plan is built before live verification completes
- that plan is carried in `moLiveApplyPlan`
- applied admitted mutation-entry results now carry `moLiveApplyObservation`
- `moLiveApplyObservation` classifies as `Exact`
- the applied plan carries the explicit stage family:
  - raw mutation
  - realization
  - verification
- queue comparison remains exact
- computational comparison remains a full match
- graph comparison remains exact and full-match

That means the admitted live apply shell is no longer just implicit runtime
sequencing at the main mutation-entry call site. The bounded host shell now
consumes one engine-authored plan first.

## Rollback And Reject Lane

The bounded rollback and reject proof lane is:

- `testComputationalMutationEntryRejectsDirtyBaseline`

That lane verifies:

- the admitted live apply shell still refuses a dirty baseline
- the live apply plan still records the rolled-back path explicitly
- the rolled-back plan carries the explicit stage family:
  - raw mutation
  - rollback
- rollback restores the captured admitted before-state exactly
- `moLiveApplyObservation` classifies the rolled-back outcome as `Exact`

This matters because the live apply workstream does not succeed by making
exact apply greener while weakening explicit rollback sequencing.

## Deferred And Out-Of-Contract Reading

The live apply cycle still remains intentionally bounded.

The standing classifier coverage from the observation workstream continues to
keep these families explicit:

- ordering-only host execution drift
- hidden host apply orchestration
- missing realized or rolled-back objects
- queue or state mismatch
- out-of-contract live apply plans

No new workbook or mutation classes were admitted in order to make the live
apply shell look exact.

## Host-Shell Reduction Assessment

This cycle materially shrinks the remaining host shell because:

- admitted stage ordering is now frozen as a value-semantic live apply plan
- admitted mutation-entry callers no longer only walk raw mutation,
  realization, verification, and rollback inline at the main compat site
- applied and rolled-back paths now both carry explicit plan identity and
  observation instead of relying on hidden host orchestration alone

The host shell is not gone. Calc still executes:

- the underlying raw document mutation APIs
- the primitive realization and rollback host operations
- the final verification host shell

But the admitted live apply shell is meaningfully smaller than before this
cycle because stage identity and ordering are now engine-authored and
explicit.

## Bounded Runtime Samples

Focused proof-lane samples on the current machine:

- `testComputationalMutationEntrySetValue`: `elapsed=7.99s`, `rss_kb=248648`
- `testComputationalMutationEntryRejectsDirtyBaseline`: `elapsed=7.95s`,
  `rss_kb=248760`

These are bounded proof-lane measurements, not throughput benchmarks. They
show that the explicit live apply plan does not introduce an obvious runtime
or memory cliff on the exercised admitted slice.

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

- admitted-slice live apply sequencing is now explicitly engine-authored and
  exact on the exercised admitted lanes

The remaining closeout question is not whether the apply plan works. It is
whether that narrower host shell is strong enough to count as a settled
boundary shift or should still be described as a hybrid live apply path.

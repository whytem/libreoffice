# Computational Substrate Phase 5 Differential Surface

Status: active Phase 5 differential reference

## Purpose

This note records the explicit verdict categories exercised by the Phase 5
lifecycle-authority lane.

It exists so the Phase 5 closeout can point to one checked-in surface that
names:

- which lifecycle outcomes are considered successful
- which outcomes remain accepted only as classification-only evidence
- which outcomes are treated as rejection or rollback conditions
- which tests cover each category

## Verdict Categories

### Applied

Meaning:

- the admitted lifecycle mutation was accepted
- the engine-authored lifecycle transition was applied on the narrowed scalar
  formula subset
- queue verification is exact
- computational verification is exact
- graph verification is exact

Coverage:

- `testComputationalLifecycleInsertFormulaPilot`
- `testComputationalLifecycleReplaceFormulaPilot`
- `testComputationalLifecycleRemoveFormulaPilot`

### Applied Normalized Equivalent

Meaning:

- the lifecycle transition is accepted
- exact computational verification still holds
- graph or IR observation may normalize to an equivalent representation that
  the contract explicitly allows

Coverage:

- `testComputationalLifecycleClassifiesNormalizedEquivalent`

Phase 5 note:

- this remains a classification-only proof point in Phase 5
- the admitted live runtime subset is still expected to land as exact on the
  standing insert, replace, and remove pilot cases

### Rejected Dirty Baseline

Meaning:

- lifecycle authority is refused because the baseline formula state is already
  dirty or queued
- the pilot must not proceed on a non-clean baseline

Coverage:

- `testComputationalLifecycleRejectsDirtyBaseline`

### Rejected Out Of Contract

Meaning:

- the mutation is outside the admitted lifecycle subset
- the pilot returns deterministically without drifting into partial authority

Coverage:

- `testComputationalLifecycleRejectsOutOfContractMutation`

### Rolled Back Verification Failure

Meaning:

- the mutation was admitted
- live state diverged from the engine-authored lifecycle answer in queue or
  graph-facing verification
- the runtime bridge restored the pre-apply formula queue state

Coverage:

- `testComputationalLifecycleRollsBackVerificationFailure`

### Repair Detected

Meaning:

- queue and graph verification may still appear comparable
- but the broader computational-after surface does not match the
  engine-authored lifecycle answer
- this is treated as unacceptable silent lifecycle repair rather than as a
  successful apply

Coverage:

- `testComputationalLifecycleClassifiesRepairDetected`

Phase 5 note:

- this remains a classification-only proof point in Phase 5
- the admitted live pilot should not normally hit this path

## Standing Validation Lanes

Phase 5 lifecycle differential validation is expected to stay green alongside:

- `CppunitTest_sc_ucalc_dependency_shadow`
- `CppunitTest_sc_ucalc_workbook_facade`
- `CppunitTest_sc_ucalc_compile_diff`
- `spreadsheetengine_computational_graph_tests`
- `spreadsheetengine_computational_ir_tests`
- `spreadsheetengine_computational_substrate_tests`
- `spreadsheetengine_workbook_facade_tests`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`

## Phase 5 Interpretation

Phase 5 should proceed only if:

- the applied lifecycle path remains the common result on the admitted subset
- dirty and out-of-contract cases reject deterministically
- rollback remains exercised and reliable
- repair-detected stays exceptional rather than becoming normal behavior

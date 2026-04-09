# Computational Substrate Final Verification Host-Shell Observation

Status: completed observation note

## Purpose

This note freezes the observation and classification surface used by
[COMPUTATIONAL_SUBSTRATE_FINAL_VERIFICATION_HOST_SHELL_REASSESSMENT_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FINAL_VERIFICATION_HOST_SHELL_REASSESSMENT_PLAN.md)
to distinguish final-verification drift from earlier admitted primitive
drift.

The observation layer exists so the proof cycle can say whether a mismatch
comes from:

- the retained final verification host shell
- earlier live apply or primitive realization or rollback behavior
- or a true queue, computational, graph, or IR divergence

## Landed Observation Surface

The bounded observation surface now lives in:

- [ComputationalSubstrateFinalVerification.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateFinalVerification.hxx)
- [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)

It introduces one explicit admitted-slice observation type:

- `FinalVerificationObservation`

with these classifier outcomes:

- `Exact`
- `NormalizedEquivalent`
- `OrderingOnly`
- `HiddenHostVerificationOrchestration`
- `MissingVerificationInputs`
- `QueueOrStateMismatch`
- `OutOfContract`

## Inputs Frozen For This Cycle

The final-verification classifier consumes only already-admitted surfaces:

- queue comparison
- computational comparison
- graph comparison
- optional execution-IR comparison
- optional broadcaster-canonicalization comparison
- live apply observation
- primitive realization observation or primitive rollback observation

That keeps the observation layer bounded to the admitted slice and avoids
inventing a second source of truth for state that is already owned
elsewhere in the pipeline.

## Meaning Of The Classifier Outcomes

The observation model now means:

- `Exact`
  the admitted verification shell saw exact queue, computational, graph,
  live-apply, and primitive inputs
- `NormalizedEquivalent`
  the admitted verification shell stayed exact except for an already
  admitted normalized graph or IR equivalence
- `OrderingOnly`
  the remaining drift is limited to queue or broadcaster ordering
  canonicalization
- `HiddenHostVerificationOrchestration`
  the retained host shell is still quietly reconstructing acceptance logic
  beyond the engine-authored surfaces
- `MissingVerificationInputs`
  the final verification shell cannot prove exactness because primitive
  realization or rollback inputs are still incomplete
- `QueueOrStateMismatch`
  there is a real state mismatch that final verification must reject
- `OutOfContract`
  the bounded admitted verification inputs were not present at all

## Why This Observation Layer Matters

Before this step, the retained final verification shell was still mostly an
implicit consequence of earlier comparisons and pilot-specific result
classification.

After this step, the proof cycle has one explicit admitted observation
surface for the remaining verification shell. That makes it possible to:

- write exact classifier tests for the final verification seam itself
- keep earlier primitive drift separate from true verification drift
- build the next implementation step around an explicit verification record
  instead of hidden local sequencing

## Validation Used

The observation layer was validated with:

- `CppunitTest_sc_ucalc_dependency_shadow`
- `git diff --check`

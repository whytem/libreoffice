# Computational Substrate Live Apply-Shell Implementation

Status: implemented live-apply path

## Purpose

This note records the engine-authored live apply-plan path added for
[COMPUTATIONAL_SUBSTRATE_LIVE_APPLY_SHELL_REASSESSMENT_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_LIVE_APPLY_SHELL_REASSESSMENT_PLAN.md).

The implementation goal for this workstream is intentionally narrow:

- introduce one explicit engine-authored apply plan for the admitted slice
- have Calc consume that plan instead of implicitly sequencing admitted raw
  mutation, realization, verification, and rollback at the main call site
- keep the underlying raw mutation APIs in Calc for this cycle

## Landed Engine-Authored Surface

The new compat surface lives in:

- [ComputationalSubstrateLiveApply.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateLiveApply.hxx)

It now defines:

- `LiveApplyStageKind`
- `AdmittedLiveApplyPlan`
- `LiveApplyPlanBuildResultKind`
- `LiveApplyPlanBuildResult`
- `buildAdmittedLiveApplyPlan(...)`
- `LiveApplyObservation`
- `classifyLiveApplyObservation(...)`

The admitted live apply plan is value-semantic and bundles the already-proven
engine-authored surfaces that the host shell was previously sequencing more
implicitly:

- admitted raw mutation record
- admitted object-realization record when applicable
- admitted rollback record
- explicit stage ordering

## Apply-Plan Flow

The landed live apply path is:

1. build an admitted raw mutation record from the admitted mutation-entry
   request
2. build an admitted rollback record from the captured before-state
3. build an admitted live apply plan that fixes stage order for the bounded
   path
4. execute the primitive host stages through that already-built plan
5. classify the resulting live apply observation explicitly

The important change is that the main mutation-entry path no longer only
walks those stages inline. It now carries one explicit engine-authored plan
for the admitted slice.

## Mutation-Entry Integration

The admitted mutation-entry path in
[ComputationalSubstrateMutationEntry.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateMutationEntry.hxx)
now:

- stores the admitted live apply plan in `moLiveApplyPlan`
- stores the resulting live apply observation in `moLiveApplyObservation`
- builds a rolled-back plan for dirty-baseline rejection, transition failure,
  realization failure, and verification-triggered rollback
- builds an applied plan for the exact admitted path before live
  realization and verification complete

That makes the live apply shell visible as its own bounded proof surface
instead of remaining implicit inside the mutation-entry compat wrapper.

## Boundary Kept Intact

This workstream still does not claim:

- broad `ScDocument` mutation API replacement
- shared-group, named-range-sensitive, off-sheet, or sheet-wide apply-plan
  support
- host-independent primitive execution
- broad live apply-shell migration outside the admitted slice

Calc remains the temporary primitive executor and verification host. The
landed change is that admitted live apply sequencing is now more explicitly
engine-authored before that host shell executes the bounded stages.

# Computational Substrate Final Verification Host-Shell Implementation

Status: completed implementation note

## Purpose

This note records the landed implementation for
[COMPUTATIONAL_SUBSTRATE_FINAL_VERIFICATION_HOST_SHELL_REASSESSMENT_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FINAL_VERIFICATION_HOST_SHELL_REASSESSMENT_PLAN.md).

The goal of this step was to make the admitted final verification shell more
explicitly engine-authored without widening beyond the admitted slice or
reopening primitive execution ownership.

## Landed Runtime Surface

The new admitted final-verification surface now lives in:

- [ComputationalSubstrateFinalVerification.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateFinalVerification.hxx)
- [ComputationalSubstrateMutationEntry.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateMutationEntry.hxx)
- [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)

It adds:

- `AdmittedFinalVerificationRecord`
- `FinalVerificationRecordResult`
- `FinalVerificationApplyResult`
- `FinalVerificationObservation`

and uses them inside mutation entry before the retained host shell closes the
admitted path.

## What Changed In Mutation Entry

`MutationEntryResult` now carries two additional admitted-slice surfaces:

- `moFinalVerificationRecord`
- `moFinalVerificationObservation`

The runtime now:

- builds an explicit final verification record from the admitted live apply
  plan plus primitive realization or primitive rollback identity
- applies that record against the already-collected admitted queue,
  computational, graph, broadcaster, live-apply, and primitive observation
  surfaces
- stores the resulting final verification observation explicitly on the
  mutation-entry result

This means the retained verification shell is no longer only the implicit
result of path-specific classifier code. It now consumes one named
engine-authored verification record first.

## Rollback Surface Tightening

The rollback path now carries explicit comparison state through the admitted
verification seam instead of collapsing immediately to a single rollback
observation.

`ScopedComputationalMutationEntry` now preserves, on rollback lanes:

- queue comparison
- computational comparison
- graph comparison
- broadcaster canonicalization comparison
- rollback observation

That change matters because the final-verification record needs the same
comparison surface on rollback lanes that apply lanes already had.

## Compatibility Rule Preserved

One important compatibility rule stayed intact:

- execution-IR drift remains observational on this admitted slice

The earlier admitted mutation-entry boundary did not treat IR mismatch as a
hard verification reject, so the new final-verification record preserves
that behavior. Queue, computational, graph, live-apply, and primitive
realization or rollback state remain the exact acceptance gate here.

## Test Coverage Added

The landed tests now cover:

- direct classifier coverage for the final-verification observation kinds
- admitted applied mutation-entry lanes carrying an explicit final
  verification record and exact observation
- admitted dirty-baseline rollback lanes carrying an explicit final
  verification record and exact observation

This keeps the new verification seam visible in the unit suite instead of
only emerging as a side effect of broader mutation-entry assertions.

## Boundaries Kept On Purpose

This implementation still leaves these host-owned surfaces in Calc:

- primitive execution host operations
- the broader live document shell outside the admitted slice
- the final decision of whether this verification surface should become a
  settled boundary shift

So this step narrows the host shell and makes it explicit, but it does not
yet claim broad host independence.

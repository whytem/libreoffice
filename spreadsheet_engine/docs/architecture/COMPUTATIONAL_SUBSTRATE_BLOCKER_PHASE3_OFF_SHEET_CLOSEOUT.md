# Computational Substrate Blocker Phase 3 Off-Sheet Closeout

Status: completed closeout for blocker-clearance Phase 3

## Scope

This closeout covers Phase 3 of
[COMPUTATIONAL_SUBSTRATE_BLOCKER_CLEARANCE_PLAN.md](COMPUTATIONAL_SUBSTRATE_BLOCKER_CLEARANCE_PLAN.md):
bounded off-sheet dependency closure.

## Result

Phase 3 closed without admitting an off-sheet shared-group family.

The important conclusion is that off-sheet behavior is not currently blocked
by one missing gate flip. It is blocked by the fact that the facade and
authority surfaces still treat cross-sheet shared-group consumers as outside
the exact bounded contract.

## Current Boundary

The current off-sheet boundary is explicit:

- bounded same-sheet named-range-combined cases can stay on the admitted
  surface
- off-sheet named-range-combined cases are classified as `Deferred`
- off-sheet shared-group consumer candidates remain rejected at the narrow
  authority surface
- broader multi-sheet spill or workbook-wide closure stays fenced

## Why No Admission Landed

The blocker is not formula evaluation alone. A credible off-sheet admission
would need exact:

- cross-sheet dependency edges
- cross-sheet broadcaster and listener identity
- queue and invalidation closure across sheets
- rollback and final verification on the same bounded cross-sheet surface

That exact end-to-end closure is not yet present on the current runtime
surface, so admitting a family in this phase would have been synthetic.

## Evidence

The main retained proof buckets for the current off-sheet boundary are:

- `testCalcFacadeSharedGroupNamedRangeBoundaryOffSheetStaysDeferred`
- `testComputationalNarrowRolloutSharedGroupNonStructuralAuthorityOffSheetConsumerStaysRejected`
- `testComputationalNarrowRolloutGlobalNamedRangeOffSheetConsumerCandidate`

## Decision

All off-sheet shared-group families remain deferred after Phase 3.

This is still a useful closeout because the phase narrows the next task:
future off-sheet work must start with canonical cross-sheet dependency and
broadcaster identity, not with candidate-gate widening.

## Follow-On

The next blocker phase should move to host-shell reduction. If a later pass
returns to off-sheet closure, it should do so only after the host shell is
recut tightly enough that cross-sheet verification is comparing one
canonical engine-authored record set.

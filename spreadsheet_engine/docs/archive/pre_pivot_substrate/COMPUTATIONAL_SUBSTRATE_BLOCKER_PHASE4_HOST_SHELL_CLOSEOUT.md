# Computational Substrate Blocker Phase 4 Host-Shell Closeout

Status: completed closeout for blocker-clearance Phase 4

## Scope

This closeout covers Phase 4 of
[COMPUTATIONAL_SUBSTRATE_BLOCKER_CLEARANCE_PLAN.md](COMPUTATIONAL_SUBSTRATE_BLOCKER_CLEARANCE_PLAN.md):
recutting the retained host shell into an explicit, narrow execution
boundary around engine-authored admitted-slice records.

## Result

Phase 4 closed without a new admitted family, but it did remove one of the
original top blocker categories as an opaque concern.

The retained host shell is no longer best described as a broad mixed-ownership
blocker on the admitted slice. The runtime and proof surfaces now already
describe it as a narrow document-service shell that executes and observes
engine-authored records.

## Current Host-Shell Contract

On the admitted slice, the engine is already the canonical source for:

- expected after-state planning
- expected graph and expected IR construction
- object-realization records
- rollback records
- raw-mutation and live-apply plans
- primitive realization and primitive rollback records
- final-verification records and observations

Calc remains responsible for the retained host shell around those records:

- applying raw document mutations to the live document
- materializing and restoring live objects through the document APIs
- executing the recalc queue against the live workbook state
- surfacing the live observations compared against engine-authored records
- hosting workbook and mutation classes outside the admitted slice

That is still a real host boundary, but it is no longer the hidden truth
source for the admitted path.

## Evidence Surface

The main runtime classifier seams already encode this contract explicitly:

- object realization distinguishes `Exact`, `OrderingOnly`,
  `MissingRealizedObjects`, `HostOnlyRepairOrReconstruction`,
  `QueueOrStateMismatch`, and `OutOfContract`
- live apply distinguishes `Exact`, `OrderingOnly`,
  `HiddenHostApplyOrchestration`, `MissingRealizedOrRolledBackObjects`,
  `QueueOrStateMismatch`, and `OutOfContract`
- final verification distinguishes `Exact`, `NormalizedEquivalent`,
  `OrderingOnly`, `HiddenHostVerificationOrchestration`,
  `MissingVerificationInputs`, `QueueOrStateMismatch`, and
  `OutOfContract`
- primitive host execution likewise distinguishes exact, ordering-only,
  hidden-host, missing-input, and queue/state-mismatch outcomes

Those buckets mean the current runtime is no longer treating host-shell
behavior as a generic opaque blocker. It is classifying the retained host
surface into explicit exact, normalized, ordering-only, host-only, missing,
or mismatch outcomes.

## Decision

The retained host shell is no longer a top blocker in the same sense as the
remaining shared-group authoring, repair-sensitive normalization, and
off-sheet dependency-closure frontiers.

Future widening should still respect the retained host shell, but the next
program steps should treat it as an explicit bounded contract rather than as
the main unresolved architecture risk.

## Evidence

The standing proof buckets for this closeout are:

- `testComputationalObjectRealizationClassifierKinds`
- `testComputationalLiveApplyObservationClassifierKinds`
- `testComputationalFinalVerificationObservationClassifierKinds`
- `testComputationalPrimitiveHostExecutorObservationClassifierKinds`

This closeout also rests on the already-settled admitted-slice boundary
recorded in:

- [COMPUTATIONAL_SUBSTRATE_LIVE_APPLY_SHELL_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_LIVE_APPLY_SHELL_DECISION_RECORD.md)
- [COMPUTATIONAL_SUBSTRATE_FINAL_ROLLBACK_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_FINAL_ROLLBACK_DECISION_RECORD.md)
- [COMPUTATIONAL_SUBSTRATE_FINAL_VERIFICATION_HOST_SHELL_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_FINAL_VERIFICATION_HOST_SHELL_DECISION_RECORD.md)

## Follow-On

The final blocker-program closeout should now reassess the roadmap from this
cleaner boundary:

- broader same-sheet after-state authoring remains the main same-surface
  blocker
- repair-sensitive normalization remains explicit but unresolved
- off-sheet dependency closure remains explicitly deferred
- the host shell should move out of the top blocker list and into the
  settled admitted-slice contract

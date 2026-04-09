# Computational Substrate Blocker Phase 2 Normalization Closeout

Status: completed closeout for blocker-clearance Phase 2

## Scope

This closeout covers Phase 2 of
[COMPUTATIONAL_SUBSTRATE_BLOCKER_CLEARANCE_PLAN.md](COMPUTATIONAL_SUBSTRATE_BLOCKER_CLEARANCE_PLAN.md):
making repair-sensitive normalization explicit.

## Normalization Taxonomy

Phase 2 classifies the current repair-sensitive frontier into three buckets.

### 1. Deterministic Repair-Detected Rollback

These are not admitted, but they are no longer opaque:

- structural named-range divergence that resolves to explicit
  `RepairDetected`
- structural shared-group divergence that resolves to explicit
  `RepairDetected`
- lifecycle or mutation-entry verification drift that is intentionally
  surfaced as `RepairDetected`

These cases are acceptable as explicit boundary behavior because the engine
already classifies them and rolls them back deterministically.

### 2. Explicit Reject-By-Rule

These remain out of scope by rule rather than hidden cleanup:

- repair-sensitive shared-group regroup families that do not close on exact
  authored after-state
- repair-sensitive shared-group merge families that still require host-only
  cleanup or regrouping
- any family that still reaches `opaque_dependency_surface`

### 3. Retained Host-Only Cleanup Shapes

These are surfaced by canonicalization and realization diagnostics but are
not yet normalized into admitted behavior:

- duplicate broadcaster materialization
- listener-anchor canonicalization only
- unexpected host listeners
- queue-or-state mismatch observations in realization, rollback, live apply,
  raw mutation, primitive execution, and final verification

## Result

Phase 2 closed without an admitted-slice expansion.

That is still a meaningful result. The repair-sensitive frontier is now
explicit enough that later widening does not have to rediscover what kind of
failure it is looking at. The remaining work is no longer “generic repair.”
It is one of:

- deterministic repair-detected rollback
- explicit reject-by-rule
- retained host-only cleanup drift

## Evidence Surface

The existing proof ladder already covers the main buckets:

- `testComputationalLifecycleClassifiesRepairDetected`
- `testComputationalMutationEntryClassifiesRepairDetected`
- `testComputationalStructuralValidateGlobalNamedRangeRepairDetected`
- `testComputationalStructuralValidateSharedGroupRepairDetected`
- `testComputationalStructuralRepairDetectedRollback`
- `testComputationalStructuralDeleteRowRepairDetectedRollback`
- `testComputationalStructuralInsertColumnRepairDetectedRollback`
- object-realization classifier proof for broadcaster canonicalization kinds

## Decision

Repair-sensitive behavior remains outside the admitted slice, but it is now
an explicit boundary with named subfamilies.

No later pass should use the generic phrase “repair-sensitive” without also
placing the case into one of the three buckets above.

## Follow-On

The next blocker phase should move to bounded off-sheet dependency closure
using the now-explicit repair taxonomy as a fence. Off-sheet work should
admit only families that stay out of the retained cleanup buckets.

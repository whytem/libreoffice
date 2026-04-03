# Computational Substrate Phase 7 Rollout Matrix

Status: active rollout matrix for Phase 7

## Purpose

This matrix translates the Phase 7 ownership map into an explicit rollout
candidate.

It exists to keep the Phase 7 decision grounded in specific workbook classes,
mutation classes, and authority gates rather than broad architectural
aspiration.

## Candidate Rollout Shape

The cleanest candidate after Phase 6 is a narrow rollout, not a broad one.

The recommended rollout posture is:

- opt-in only
- developer or experimental channel first
- exact verification retained on admitted authority surfaces
- immediate rollback or deactivation on divergence

## Workbook-Class Matrix

- synthetic single-sheet scalar workbooks
  - candidate status: ready for bounded rollout
  - reason: closest match to the admitted scalar lifecycle and structural
    authority surface
- promoted FODS-style scalar workbook slices with no shared groups and no
  named-range-sensitive structural behavior
  - candidate status: ready for bounded rollout after targeted bake time
  - reason: broad semantic coverage with cleaner admitted mutation shapes
- ordinary Calc workbooks containing shared groups
  - candidate status: pilot-only
  - reason: shared-group lifecycle and repair remain deferred
- workbooks with named-range-sensitive structural behavior
  - candidate status: pilot-only
  - reason: live named-range structural authority remains deferred
- workbooks using external-reference-sensitive structural behavior
  - candidate status: deferred
  - reason: retained host ownership remains clearer than partial rollout
- load-time, clipboard, undo-like, or import-heavy workbook flows
  - candidate status: deferred
  - reason: these classes were not admitted by the proven authority subset

## Mutation-Class Matrix

- `SetValue`
  - candidate status: ready for bounded rollout
  - authority basis: admitted by Phase 4
- scalar `SetString`
  - candidate status: ready for bounded rollout
  - authority basis: admitted by Phases 4 and 5 where lifecycle shape stays
    in contract
- direct `SetFormula`
  - candidate status: ready for bounded rollout
  - authority basis: admitted scalar lifecycle surface
- `ClearCell`
  - candidate status: ready for bounded rollout
  - authority basis: admitted scalar lifecycle surface
- single-sheet `InsertRows`
  - candidate status: ready for bounded rollout only on the ordinary-scalar
    slice
  - authority basis: admitted by Phase 6
- single-sheet `DeleteColumns`
  - candidate status: ready for bounded rollout only on the ordinary-scalar
    slice
  - authority basis: admitted by Phase 6
- `DeleteRows`
  - candidate status: pilot-only validation
  - authority basis: validation-only in Phase 6
- `InsertColumns`
  - candidate status: pilot-only validation
  - authority basis: validation-only in Phase 6
- sheet insert, delete, rename, or move
  - candidate status: deferred
  - authority basis: not admitted
- copy, move, clipboard, load-time, or undo-like structural behavior
  - candidate status: deferred
  - authority basis: not admitted

## Recommended Authority Gates

If rollout proceeds, the candidate authority gates should be layered rather
than monolithic:

- `scalar_lifecycle_authority`
  - covers the admitted Phase 5 scalar lifecycle subset
- `structural_scalar_slice_authority`
  - covers the admitted Phase 6 single-sheet row-insert and column-delete
    slice
- `exact_verification_required`
  - requires exact queue, computational, and graph verification
- `repair_detected_rollback_required`
  - forces immediate rollback or deactivation on admitted structural repair
    divergence

These are rollout-policy categories, not a claim that the final runtime switch
names have already been implemented.

## Recommended Deactivation Boundaries

Rollout should deactivate immediately when any of the following appear:

- dirty-baseline entry
- mutation classes outside the admitted rollout matrix
- shared-group creation, split, merge, or repair
- named-range-sensitive structural behavior
- structural repair-detected outcomes on the admitted subset
- queue, computational, or graph mismatch after admitted application

## Rollout Summary

The strongest rollout candidate after Phase 6 is:

- a narrow experimental rollout on the already admitted scalar lifecycle plus
  single-sheet row-insert and column-delete slice
- exact verification and rollback retained
- broader structural, shared-group, named-range, and sheet-level behavior
  left out of rollout

That candidate is narrow enough to stay honest and broad enough to answer the
real Phase 7 question: whether the computational substrate program should move
forward as a bounded product-direction boundary rather than remain only an
exploratory architecture effort.

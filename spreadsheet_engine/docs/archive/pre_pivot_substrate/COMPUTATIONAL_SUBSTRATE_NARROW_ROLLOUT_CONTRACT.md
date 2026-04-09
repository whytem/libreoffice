# Computational Substrate Narrow Rollout Contract

Status: complete closeout contract note for the narrow rollout plan

## Purpose

This note freezes the exact rollout contract for the completed narrow
computational substrate rollout surface.

It exists so bounded rollout engineering cannot silently widen beyond what the
completed Phase 7 decision actually admitted.

## Admitted Rollout Surface

The admitted rollout surface is now:

- admitted scalar lifecycle authority
- admitted single-sheet `InsertRows`
- admitted single-sheet `DeleteRows`
- admitted single-sheet `InsertColumns`
- admitted single-sheet `DeleteColumns`
- the ordinary-scalar-formula slice only
- no shared groups
- no named-range-sensitive structural behavior

This is the only surface that may be enabled by the completed narrow rollout.

## Required Authority Gates

The narrow rollout requires:

- one umbrella rollout gate for the admitted slice
- the ability to narrow by disabling lifecycle, structural, or earlier
  authority slices individually
- exact verification retained on admitted authority surfaces
- repair-detected rollback retained on admitted structural divergence

## Rollout Success Conditions

The admitted narrow rollout counts as successful only when:

- admitted authority paths can be enabled without broad default-on behavior
- exact queue, computational, and graph verification remain intact
- repair-detected structural divergence still rolls back
- the standing replay and substrate validation contract remains green

## Immediate Deactivation Conditions

The narrow rollout must deactivate or remain disabled when:

- the mutation class is outside the admitted rollout surface
- the baseline is dirty
- shared-group or named-range-sensitive structural behavior appears
- queue, computational, or graph mismatch appears on the admitted surface
- repair-detected structural divergence appears on the admitted surface

## Explicit Non-Admission

This contract still does not admit:

- sheet insert, delete, rename, or move
- copy, move, clipboard, load-time, or undo-like structural behavior
- broader storage or token-container ownership transfer

Those remain outside the rollout and require a future explicit decision before
any widening beyond this closeout surface.

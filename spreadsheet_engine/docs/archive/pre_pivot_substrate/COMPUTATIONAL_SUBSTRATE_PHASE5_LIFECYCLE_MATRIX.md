# Computational Substrate Phase 5 Lifecycle Matrix

Status: active mutation classification for Phase 5

## Purpose

This matrix classifies the lifecycle-bearing mutation surface for the first
Phase 5 authority pilot.

## Classification

### Admitted

- formula replacement on an existing single-cell scalar formula
- direct formula insertion into a non-formula cell
- `ClearCell` removal of an existing single-cell scalar formula

### Validation-Only

- representative shared-group lifecycle comparisons
- named-range-sensitive lifecycle comparisons

### Rejected

- `SetValue`
- scalar `SetString` that does not create or replace a formula
- shared-group creation, split, merge, or repair
- row insert / delete
- column insert / delete
- copy, move, clipboard, load-time, undo-like, or repair-heavy lifecycle paths

## Preconditions For Admitted Cases

- clean formula-tree and formula-track baseline
- no target-address shared-group membership before capture
- no required structural widening
- no required host-only repair logic

## Expected Pilot Outcomes

Admitted cases may end as:

- `Applied`
- `AppliedNormalizedEquivalent`
- `RolledBackVerificationFailure`

Non-admitted cases must end as:

- `RejectedDirtyBaseline`
- `RejectedOutOfContract`

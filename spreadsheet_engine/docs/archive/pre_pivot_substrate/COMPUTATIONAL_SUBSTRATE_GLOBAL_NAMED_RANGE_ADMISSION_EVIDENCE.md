# Computational Substrate Global Named-Range Admission Evidence

Status: completed evidence note for the global named-range admission plan

## Purpose

This note records the checked-in proof outcomes for the bounded global
single-area named-range promotion candidate.

Its job is to say what was observed, what stayed stable, and what still keeps
the slice from joining the live opt-in rollout.

## Evidence Summary

The current proof surface separates into four classes:

- exact standalone prediction on the bounded global single-area slice
- one live-candidate same-sheet class that reaches the authority bridge but
  still rolls back on verification
- deterministic reject and rollback safety cases
- one off-sheet consumer class that still stays out of contract

That is useful evidence, but it is not yet live-admission evidence.

## Exact-Match Evidence

Exact prediction evidence exists on the standalone structural builder surface
for the bounded global single-area class:

- global single-area target
- ordinary scalar formulas
- same-sheet `InsertColumns`
- exact target rewrite on the computational shadow

The checked-in standalone cases also show:

- the authority builder can classify the bounded slice as `Admitted` when the
  dedicated candidate gate is enabled
- explicit-sheet-prefix target text is preserved exactly on the standalone
  shadow surface

This is the strongest positive result from the admission cycle.

## Normalized-Equivalent Evidence

No useful global named-range-specific normalized-equivalent class was needed
to make the current proof work.

That is a good outcome. It means the bounded candidate either predicts
exactly, rolls back, or rejects cleanly instead of depending on a fuzzy
normalization rule for promotion.

## Live-Candidate Evidence

Calc differential evidence now shows that the bounded same-sheet global
single-area case can enter the live authority bridge when the dedicated
candidate gate is enabled:

- structural rollout gate enabled
- dedicated global named-range gate enabled
- global single-area target
- same-sheet structural edit
- ordinary scalar formulas

However, that case does not yet land as an exact live apply. It currently
reaches the verification bridge and returns:

- `RolledBackVerificationFailure`

That is important. The gating and authority-path plumbing are now present, but
the happy path still does not satisfy the promotion standard.

## Rollback And Repair Evidence

The promotion cycle still preserves the existing safety model:

- dirty-baseline entry rejects deterministically
- deliberate divergence on the bounded global named-range case returns
  `RepairDetected` and rolls back
- the new live-candidate gate does not bypass rollback

This means the candidate slice can be exercised without weakening the current
structural safety contract.

## Deterministic Reject Evidence

The following classes continue to reject deterministically:

- gate-off global named-range live apply
- sheet-local named ranges
- multi-area named ranges
- off-sheet consumer case under the live-candidate gate

The off-sheet result is especially important:

- formulas on another sheet that consume the same global name do not stay
  inside the current promotion surface
- the bounded candidate is therefore not homogeneous enough to treat
  same-sheet and off-sheet consumers as one live-admission class

## What This Evidence Supports

This evidence supports only the following conclusion:

- keep the bounded global single-area named-range slice out of the live
  opt-in rollout for now
- keep the standalone exact-prediction and validation/live-candidate lanes in
  place
- treat off-sheet global-name consumers as a separate question rather than as
  already-covered happy-path behavior

It does not support live rollout admission yet.

## Standing Validation

The evidence above is backed by:

- `CppunitTest_sc_ucalc_dependency_shadow`
- `spreadsheetengine_computational_substrate_tests`
- `git diff --check`

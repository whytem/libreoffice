# Computational Substrate Phase 7 Rollback Strategy

Status: active rollback and deactivation strategy for Phase 7

## Purpose

This note defines how the computational substrate program backs out if the
Phase 7 rollout candidate does not hold up.

It exists so a narrow proceed decision cannot quietly become a one-way
commitment. The candidate boundary must have an explicit rollback and
deactivation story.

## Candidate Authority Gates

If rollout proceeds, the candidate authority should remain gated in layers:

- scalar lifecycle authority gate
- structural scalar-slice authority gate
- exact verification required gate
- repair-detected rollback required gate

These gates should be independently disableable so the project can narrow
authority without disabling all engine-owned computation work.

## Immediate Deactivation Triggers

The candidate rollout should deactivate immediately when any of the following
appear on the admitted surface:

- dirty-baseline entry into an authority path
- mutation classes outside the admitted rollout matrix
- queue mismatch after admitted application
- computational mismatch after admitted application
- graph mismatch after admitted application
- repair-detected structural divergence on the admitted subset
- unexpected shared-group, named-range-sensitive, or broader structural repair
- replay regression away from zero cached fallback

## Narrowing Triggers

The candidate rollout should narrow rather than stop completely when:

- scalar lifecycle authority remains stable but structural authority is not
- synthetic or FODS-style scalar workbook slices remain stable but broader
  workbook classes do not
- performance or memory regressions appear only on the structural slice and
  not on the scalar lifecycle slice

Narrowing is the preferred response when the evidence shows one admitted slice
is sound and another is not.

## Stop Triggers

The computational substrate program should recommend stop rather than rollout
when either of the following becomes true:

- the resulting boundary is harder to explain or operate than the current
  Calc and engine split
- correctness, memory, or recalc behavior is not good enough even for the
  narrow rollout candidate

It should also stop if continued rollout would depend on hidden dual authority
between Calc and `spreadsheet_engine/`.

## Rollback Path

The rollback path for a narrow rollout candidate should be:

- disable the structural scalar-slice authority gate first
- fall back to the admitted scalar lifecycle authority slice if it remains
  healthy
- disable scalar lifecycle authority next if the narrower slice also regresses
- retain the already-completed first-stage extraction, replay, and standalone
  engine capabilities regardless of the Phase 7 outcome

This ensures the project can back out of the computational-substrate rollout
without undoing the successful first-stage extraction work.

## Operational Rule

No Phase 7 proceed decision is valid unless this rollback path remains simpler
than the candidate rollout path itself.

If rollback would be harder to reason about than the current boundary, the
project should narrow or stop instead of rolling forward.

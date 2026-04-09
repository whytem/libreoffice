# Computational Substrate Phase 4 Differential Surface

Status: active Phase 4 differential-validation artifact

## Purpose

This note records the explicit verdict surface exercised by the Phase 4
authoritative pilot.

It exists so the pilot outcome categories are checked in as a deliberate
contract, not left implicit in individual tests or helper code.

## Verdict Categories

Phase 4 uses the following explicit authority-pilot outcomes:

- `Applied`
- `AppliedNormalizedEquivalent`
- `RolledBackVerificationFailure`
- `RejectedDirtyBaseline`
- `RejectedOutOfContract`

## Phase 4 Interpretation

For the narrowed admitted subset, the authoritative pilot is currently gating
application on:

- exact queue correspondence after application
- exact graph-facing verification after application

The pilot also records execution-IR comparison state for the same mutations,
but in Phase 4 that IR comparison remains observation data rather than a hard
rollback gate.

This matches the actual narrowed authority claim of Phase 4:

- the engine is authoritative for dependency-graph update and recalc-queue
  derivation on the admitted pilot subset
- Calc still hosts execution, verification capture, and rollback
- execution-IR evidence is retained for later phases without overstating
  current authority

## Covered Lanes

The checked-in validation surface now covers:

- `Applied`
  - admitted `SetValue`
  - admitted formula text edit
- `AppliedNormalizedEquivalent`
  - explicit verdict classification lane for accepted normalized-equivalent
    post-apply state
- `RolledBackVerificationFailure`
  - admitted mutation with deliberately diverged live graph state
- `RejectedDirtyBaseline`
  - admitted mutation attempted from a dirty baseline
- `RejectedOutOfContract`
  - deferred mutation class attempted through the authoritative bridge

## Deferred Truths

Phase 4 does not yet claim:

- broad structural authority
- named-range authority
- live listener/broadcaster ownership migration
- hard rollback gating on execution-IR mismatch

Those remain Phase 5 or later questions and should stay explicit in the
decision record.

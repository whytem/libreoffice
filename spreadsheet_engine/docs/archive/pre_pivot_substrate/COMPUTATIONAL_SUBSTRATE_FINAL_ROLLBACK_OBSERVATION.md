# Computational Substrate Final Rollback Observation

Status: frozen final-rollback observation model

## Purpose

This note freezes the rollback observation and classification model for
[COMPUTATIONAL_SUBSTRATE_FINAL_ROLLBACK_REASSESSMENT_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FINAL_ROLLBACK_REASSESSMENT_PLAN.md).

The goal of the observation layer is to make admitted-slice rollback gaps
explicit enough that the closeout can distinguish:

- exact restore
- ordering-only restore drift
- missing restored live objects
- host-only rollback reconstruction
- true queue or state mismatch

This observation model does not itself widen the settled boundary. It exists
to make rollback outcomes diagnosable before the engine-authored rollback
record is promoted or deferred.

## Observation Kinds

### `Exact`

Use `Exact` only when all of the following hold after rollback:

- rollback realization applies successfully
- rollback queue comparison is exact
- computational comparison is a full match
- graph comparison is a full match
- broadcaster comparison is exact

This is the only kind that can count as settled rollback authority evidence.

### `OrderingOnly`

Use `OrderingOnly` when:

- rollback queue order differs
- computational and graph state still match exactly
- broadcaster comparison is ordering-equivalent

This may count as validation-only evidence, but not as a proceed signal on
its own.

### `MissingRestoredObjects`

Use `MissingRestoredObjects` when rollback fails to restore some expected
live surface, including:

- missing restored formula cells
- missing restored broadcaster nodes
- missing restored listener participation
- computational cell-population mismatch caused by incomplete restore

This kind signals a real rollback gap, even if the remaining state appears
close to the expected baseline.

### `HostOnlyRollbackReconstruction`

Use `HostOnlyRollbackReconstruction` when rollback ends in a state that is
only diagnosable as host-side cleanup or canonicalization, such as:

- duplicate broadcaster materialization
- empty broadcaster materialization
- listener-anchor-only canonicalization
- mixed host-side repair patterns

This kind is explicit evidence that Calc still reclaimed rollback authority
in a way not represented by the engine-authored surface.

### `QueueOrStateMismatch`

Use `QueueOrStateMismatch` when rollback applies but exact restore still
fails for queue, computational, or graph reasons that are not better
explained by the missing-object or host-reconstruction classes.

This kind is the default “true restore divergence” bucket.

### `OutOfContract`

Use `OutOfContract` when rollback realization itself fails or the restore
path leaves the admitted slice.

This kind is not evidence for proceed. It records a bounded failure or defer
surface.

## Comparison Inputs

The admitted rollback observation model consumes:

- the rollback realization result
- rollback queue comparison against the captured before-state formula queue
- computational comparison against the captured before-state computational
  shadow
- graph comparison against the captured before-state graph shadow
- broadcaster canonicalization comparison against the captured before-state
  wiring expectation

The observation layer intentionally stays scoped to the admitted slice. It
does not attempt to diagnose shared-group, named-range-sensitive, or
sheet-wide rollback behavior.

## Standing Proof Lanes

The rollback proof cycle should keep at least these lanes checked in and
green:

- synthetic classifier coverage for every rollback observation kind
- one exact rollback restore case on the admitted scalar slice
- one missing-restored-object case on the admitted scalar slice
- later closeout evidence tying these classifications back to the full
  mutation-entry rollback path

## Guardrails

This observation model is violated if implementation work:

- silently treats `OrderingOnly` as exact
- folds missing restored objects into generic mismatch
- treats host-only reconstruction as a success case
- widens the observation surface beyond the admitted slice without a new
  plan

# Computational Substrate Live Apply-Shell Observation

Status: frozen live-apply observation model

## Purpose

This note freezes the live apply-shell observation and classification model
for
[COMPUTATIONAL_SUBSTRATE_LIVE_APPLY_SHELL_REASSESSMENT_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_LIVE_APPLY_SHELL_REASSESSMENT_PLAN.md).

The goal of the observation layer is to make admitted-slice apply-shell gaps
explicit enough that the closeout can distinguish:

- exact engine-authored live apply sequencing
- ordering-only host execution drift
- hidden host apply orchestration or repair
- missing realized or rolled-back live objects
- true queue, computational, graph, or replay divergence

This observation model does not itself widen the settled boundary. It exists
to make live apply outcomes diagnosable before the engine-authored apply plan
is promoted or deferred.

## Observation Kinds

### `Exact`

Use `Exact` only when the admitted live apply path closes exactly after the
engine-authored stages are consumed, or when rollback from that same apply
path closes exactly on the admitted slice.

### `OrderingOnly`

Use `OrderingOnly` when the admitted apply plan is consumed and the only
remaining difference is bounded stage or materialization ordering drift that
still preserves exact admitted state.

### `HiddenHostApplyOrchestration`

Use `HiddenHostApplyOrchestration` when the host shell still appears to
reconstruct or repair stage ordering in a way not represented by the
engine-authored apply plan, including:

- host-only realization sequencing
- host-only rollback sequencing
- host-only raw mutation orchestration

### `MissingRealizedOrRolledBackObjects`

Use `MissingRealizedOrRolledBackObjects` when the live apply path fails to
leave or restore the expected admitted live surface, including:

- missing realized formula objects
- missing rolled-back formula objects
- missing listener or broadcaster participation after apply or rollback

### `QueueOrStateMismatch`

Use `QueueOrStateMismatch` when live apply succeeds but exact state still
fails for queue, computational, graph, or replay reasons that are not better
explained by the missing-object or hidden-orchestration classes.

### `OutOfContract`

Use `OutOfContract` when the apply-plan path itself is not admissible,
cannot be classified on the admitted slice, or leaves that slice.

## Comparison Inputs

The admitted live apply observation model consumes:

- admitted raw mutation observation
- admitted object-realization observation when the path applies
- admitted rollback observation when the path is rejected or rolled back

The observation layer intentionally stays scoped to the admitted slice. It
does not attempt to diagnose shared-group, named-range-sensitive, or
sheet-wide apply behavior.

## Standing Proof Lanes

The live apply-shell proof cycle should keep at least these lanes checked in
and green:

- synthetic classifier coverage for every live apply observation kind
- later runtime proof that exact admitted apply closes with `exact`
- later runtime proof that rollback from the apply shell can still be
  classified explicitly

## Guardrails

This observation model is violated if implementation work:

- silently treats `OrderingOnly` as exact
- folds missing realized or rolled-back objects into generic mismatch
- treats hidden host orchestration as success
- widens the observation surface beyond the admitted slice without a new
  plan

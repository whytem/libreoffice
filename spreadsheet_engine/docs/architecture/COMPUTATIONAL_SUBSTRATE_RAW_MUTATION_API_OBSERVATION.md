# Computational Substrate Raw Mutation API Observation

Status: frozen raw-mutation observation model

## Purpose

This note freezes the raw mutation observation and classification model for
[COMPUTATIONAL_SUBSTRATE_RAW_MUTATION_API_MIGRATION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_RAW_MUTATION_API_MIGRATION_PLAN.md).

The goal of the observation layer is to make admitted-slice raw mutation
shell gaps explicit enough that the closeout can distinguish:

- exact engine-authored raw mutation entry
- ordering-only shell drift
- hidden host-originated mutation reconstruction
- missing realized or rolled-back live objects
- true queue, computational, graph, or replay divergence

This observation model does not itself widen the settled boundary. It exists
to make raw mutation outcomes diagnosable before the engine-authored raw
mutation record is promoted or deferred.

## Observation Kinds

### `Exact`

Use `Exact` only when the admitted raw mutation record is applied and the
downstream admitted state closes exactly, or when rollback from the raw
mutation path closes exactly on the admitted slice.

### `OrderingOnly`

Use `OrderingOnly` when the admitted raw mutation record is consumed and the
only remaining difference is bounded ordering drift that still preserves
exact admitted state.

### `HiddenHostMutationReconstruction`

Use `HiddenHostMutationReconstruction` when the host shell still appears to
reconstruct the mutation or its repair path in a way not represented by the
engine-authored record, including:

- host-only object-realization reconstruction
- host-only rollback reconstruction

### `MissingRealizedOrRolledBackObjects`

Use `MissingRealizedOrRolledBackObjects` when the raw mutation path fails to
leave or restore the expected admitted live surface, including:

- missing realized formula objects
- missing rolled-back formula objects
- missing listener or broadcaster participation after apply or rollback

### `QueueOrStateMismatch`

Use `QueueOrStateMismatch` when raw mutation apply succeeds but exact state
still fails for queue, computational, graph, or replay reasons that are not
better explained by the missing-object or hidden-reconstruction classes.

### `OutOfContract`

Use `OutOfContract` when the raw mutation record itself is not admissible,
cannot be applied, or the path leaves the admitted slice.

## Comparison Inputs

The admitted raw mutation observation model consumes:

- whether an admitted raw mutation record was built and applied
- queue comparison after apply
- computational comparison after apply
- graph comparison after apply
- object-realization observation when the mutation is applied
- rollback observation when the mutation is rejected or rolled back

The observation layer intentionally stays scoped to the admitted slice. It
does not attempt to diagnose shared-group, named-range-sensitive, or
sheet-wide mutation behavior.

## Standing Proof Lanes

The raw mutation proof cycle should keep at least these lanes checked in and
green:

- synthetic classifier coverage for every raw mutation observation kind
- later runtime proof that exact admitted apply closes with `exact`
- later runtime proof that rollback from the raw mutation path can still be
  classified explicitly

## Guardrails

This observation model is violated if implementation work:

- silently treats `OrderingOnly` as exact
- folds missing realized or rolled-back objects into generic mismatch
- treats hidden host reconstruction as success
- widens the observation surface beyond the admitted slice without a new
  plan

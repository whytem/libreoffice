# Computational Substrate Primitive Realization And Rollback Observation

Status: frozen primitive-realization-rollback observation model

## Purpose

This note freezes the observation and classification model for
[COMPUTATIONAL_SUBSTRATE_PRIMITIVE_REALIZATION_ROLLBACK_REASSESSMENT_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PRIMITIVE_REALIZATION_ROLLBACK_REASSESSMENT_PLAN.md).

The goal of this observation layer is to make primitive realization and
rollback gaps explicit without reopening the already-settled
object-realization, rollback, raw-document-mutation, or live-apply
vocabularies.

## Observation Surface

The primitive realization and rollback observation layer is a bounded
classifier that sits one level narrower than the existing
object-realization and rollback observations:

- the admitted object-realization record still describes engine-authored
  realized state identity
- the admitted rollback record still describes engine-authored restore
  identity
- primitive realization and primitive rollback observations now classify
  execution of those identities against the live document shell

This keeps the new proof surface focused on primitive host execution rather
than reclassifying every downstream mismatch from scratch.

## Frozen Primitive Realization Observation Kinds

The admitted primitive realization classifier is frozen to these kinds:

- `exact`
- `ordering_only`
- `hidden_host_realization_orchestration`
- `missing_realized_objects`
- `queue_or_state_mismatch`
- `out_of_contract`

## Frozen Primitive Rollback Observation Kinds

The admitted primitive rollback classifier is frozen to these kinds:

- `exact`
- `ordering_only`
- `hidden_host_rollback_orchestration`
- `missing_restored_objects`
- `queue_or_state_mismatch`
- `out_of_contract`

Those kinds deliberately match the existing object-realization and rollback
proof vocabularies closely so that the closeout can say whether the
remaining gap is truly in primitive host execution or somewhere later in
the final verification shell.

## Inputs

The primitive realization observation is driven from:

- whether primitive realization execution actually ran
- the already-classified object-realization observation

The primitive rollback observation is driven from:

- whether primitive rollback execution actually ran
- the already-classified rollback observation

The primitive layer therefore does not invent a second independent
comparison model. It narrows the interpretation of the existing exact,
ordering, host-orchestration, missing-object, and queue-or-state mismatch
signals to the primitive realization and rollback shell.

## Classification Rules

The primitive realization rules are frozen as:

- `exact` when primitive realization executed and the downstream
  object-realization observation is exact
- `ordering_only` when primitive realization executed and the downstream
  object-realization observation is ordering-only
- `hidden_host_realization_orchestration` when primitive realization
  executed but the downstream observation proves hidden host reconstruction
- `missing_realized_objects` when primitive realization drift causes missing
  realized live objects
- `queue_or_state_mismatch` when primitive realization execution leads to
  queue, computational, or graph mismatch
- `out_of_contract` when primitive realization never executed or the
  admitted object-realization layer already rejected the request

The primitive rollback rules are frozen as:

- `exact` when primitive rollback executed and the downstream rollback
  observation is exact
- `ordering_only` when primitive rollback executed and the downstream
  rollback observation is ordering-only
- `hidden_host_rollback_orchestration` when primitive rollback executed but
  the downstream observation proves hidden host rollback reconstruction
- `missing_restored_objects` when primitive rollback drift causes missing
  restored live objects
- `queue_or_state_mismatch` when primitive rollback execution leads to
  queue, computational, or graph mismatch
- `out_of_contract` when primitive rollback never executed or the admitted
  rollback layer already rejected the request

## Diagnostic Expectations

The bounded proof lanes for this cycle must surface:

- whether primitive realization or primitive rollback ran at all
- whether the downstream object-realization or rollback outcome remained
  exact
- whether queue, computational, and graph proof stayed exact downstream
- whether broadcaster exactness was preserved

That is enough to distinguish:

- exact engine-authored primitive realization and rollback execution
- ordering-only host execution differences
- hidden host realization or rollback orchestration or repair
- missing realized or restored live objects
- true queue, computational, graph, or replay divergence

## Guardrails

This observation model is violated if implementation work:

- bypasses the object-realization or rollback classifiers and invents a
  separate conflicting primitive mismatch vocabulary
- widens primitive execution classification outside the admitted slice
- treats final verification success as proof of primitive exactness without
  an explicit primitive realization or primitive rollback observation record

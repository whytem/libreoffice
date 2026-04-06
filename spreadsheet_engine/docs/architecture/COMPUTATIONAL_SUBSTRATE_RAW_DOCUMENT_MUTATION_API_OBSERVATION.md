# Computational Substrate Raw Document Mutation API Observation

Status: frozen raw-document-mutation observation model

## Purpose

This note freezes the observation and classification model for
[COMPUTATIONAL_SUBSTRATE_RAW_DOCUMENT_MUTATION_API_MIGRATION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_RAW_DOCUMENT_MUTATION_API_MIGRATION_PLAN.md).

The goal of this observation layer is to make primitive document-mutation
gaps explicit without reopening the already-settled raw-mutation,
realization, rollback, or live-apply vocabularies.

## Observation Surface

The raw document mutation observation layer is a bounded classifier that
sits one level narrower than the existing raw-mutation observation:

- the admitted raw-mutation record still describes engine-authored mutation
  identity
- the raw document mutation observation now classifies primitive execution
  of that identity against the live document shell
- downstream realization and rollback exactness still flow through the
  settled object-realization and rollback observations

This keeps the new proof surface focused on primitive host execution rather
than reclassifying every downstream mismatch from scratch.

## Frozen Observation Kinds

The admitted primitive execution classifier is frozen to these kinds:

- `exact`
- `ordering_only`
- `hidden_host_mutation_orchestration`
- `missing_realized_or_rolled_back_objects`
- `queue_or_state_mismatch`
- `out_of_contract`

Those kinds deliberately match the existing raw-mutation proof vocabulary
closely so that the closeout can say whether the remaining gap is truly in
primitive mutation execution or somewhere later in the live apply pipeline.

## Inputs

The raw document mutation observation is driven from:

- whether primitive mutation execution actually ran
- the already-classified raw-mutation observation

The primitive layer therefore does not invent a second independent
comparison model. It narrows the interpretation of the existing exact,
ordering, host-orchestration, missing-object, and queue-or-state mismatch
signals to the primitive execution shell.

## Classification Rules

The classification rules are frozen as:

- `exact` when primitive mutation executed and the downstream raw-mutation
  observation is exact
- `ordering_only` when primitive mutation executed and the downstream
  raw-mutation observation is ordering-only
- `hidden_host_mutation_orchestration` when primitive mutation executed but
  the downstream observation proves hidden host reconstruction
- `missing_realized_or_rolled_back_objects` when primitive mutation drift
  causes missing realized or restored live objects
- `queue_or_state_mismatch` when primitive mutation execution leads to
  queue, computational, or graph mismatch
- `out_of_contract` when primitive mutation never executed or the admitted
  raw-mutation layer already rejected the request

## Diagnostic Expectations

The bounded proof lanes for this cycle must surface:

- whether primitive execution ran at all
- whether the raw-mutation outcome remained exact
- whether queue, computational, and graph proof stayed exact downstream
- whether object realization or rollback exactness was preserved

That is enough to distinguish:

- exact engine-authored primitive mutation execution
- ordering-only host execution differences
- hidden host mutation orchestration or repair
- missing realized or rolled-back live objects
- true queue, computational, graph, or replay divergence

## Guardrails

This observation model is violated if implementation work:

- bypasses the raw-mutation classifier and invents a separate conflicting
  primitive mismatch vocabulary
- widens primitive execution classification outside the admitted slice
- treats downstream live-apply success as proof of primitive exactness
  without an explicit primitive observation record

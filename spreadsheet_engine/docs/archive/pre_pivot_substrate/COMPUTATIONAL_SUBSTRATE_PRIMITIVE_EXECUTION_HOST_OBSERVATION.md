# Computational Substrate Primitive Execution Host Observation

Status: frozen primitive-execution observation model

## Purpose

This note freezes the primitive-execution observation and classification
model for
[COMPUTATIONAL_SUBSTRATE_PRIMITIVE_EXECUTION_HOST_REASSESSMENT_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PRIMITIVE_EXECUTION_HOST_REASSESSMENT_PLAN.md).

The goal of the observation layer is to make admitted-slice primitive host
gaps explicit enough that the closeout can distinguish:

- exact engine-authored primitive execution
- normalized-equivalent primitive execution
- ordering-only host execution drift
- hidden host primitive execution orchestration or repair
- missing primitive execution inputs
- true queue, computational, graph, or replay divergence

This observation model does not itself widen the settled boundary. It exists
to make primitive execution outcomes diagnosable before the engine-authored
primitive execution plan is promoted, kept hybrid, or deferred.

## Observation Surface

The primitive execution observation layer is a bounded classifier that sits
between the already-settled raw-document-mutation, primitive
realization/rollback, and final-verification observations:

- the admitted raw document mutation record still describes primitive
  mutation identity
- the primitive realization or primitive rollback record still describes the
  realized or restored live object shell
- the final verification record still describes downstream acceptance
  identity
- the primitive execution observation now classifies the combined primitive
  execution shell that links those settled records together

This keeps the proof surface focused on primitive host sequencing rather than
reclassifying every downstream mismatch from scratch.

## Frozen Observation Kinds

The admitted primitive execution classifier is frozen to these kinds:

- `exact`
- `normalized_equivalent`
- `ordering_only`
- `hidden_host_primitive_execution_orchestration`
- `missing_primitive_execution_inputs`
- `queue_or_state_mismatch`
- `out_of_contract`

Those kinds deliberately stay close to the already-settled raw document
mutation, primitive realization/rollback, and final verification vocabularies
so that the closeout can say whether the remaining gap is truly in primitive
execution sequencing or somewhere later in the retained host shell.

## Inputs

The primitive execution observation is driven from:

- whether primitive execution actually ran
- the already-classified raw document mutation observation
- the already-classified primitive realization or primitive rollback
  observation
- the already-classified final verification observation

The primitive layer therefore does not invent a second independent
comparison model. It narrows the interpretation of the existing exact,
normalized-equivalent, ordering, hidden-host, missing-input, and
queue-or-state mismatch signals to the primitive execution shell.

## Classification Rules

The classification rules are frozen as:

- `exact` when primitive execution ran and the admitted raw document
  mutation, primitive realization or rollback, and final verification
  observations all close exactly
- `normalized_equivalent` when primitive execution ran and the only
  remaining bounded difference is a settled final-verification normalized
  equivalence
- `ordering_only` when primitive execution ran and the only remaining
  bounded difference is stage or materialization ordering drift that still
  preserves exact admitted state
- `hidden_host_primitive_execution_orchestration` when the combined
  primitive execution shell still proves hidden host repair or sequencing
  not represented in the engine-authored plan
- `missing_primitive_execution_inputs` when primitive execution drift leaves
  missing realized, restored, or verification inputs
- `queue_or_state_mismatch` when primitive execution runs but exact queue,
  computational, graph, or replay proof still fails
- `out_of_contract` when primitive execution never ran, leaves the admitted
  slice, or is missing the required settled upstream or downstream
  observation surfaces

## Diagnostic Expectations

The bounded proof lanes for this cycle must surface:

- whether primitive execution ran at all
- whether the raw document mutation outcome remained exact
- whether the primitive realization or rollback outcome remained exact
- whether final verification remained exact or normalized-equivalent
- whether queue, computational, and graph proof stayed exact downstream

That is enough to distinguish:

- exact engine-authored primitive execution
- normalized-equivalent primitive execution that still preserves the
  admitted final-verification contract
- ordering-only host execution differences
- hidden host primitive execution orchestration or repair
- missing primitive execution inputs
- true queue, computational, graph, or replay divergence

## Guardrails

This observation model is violated if implementation work:

- bypasses the settled raw document mutation, primitive realization or
  rollback, or final verification classifiers and invents a conflicting
  primitive mismatch vocabulary
- widens primitive execution classification outside the admitted slice
- treats downstream live success as proof of primitive exactness without an
  explicit primitive execution observation record

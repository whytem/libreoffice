# Admitted Slice Ownership Closeout Observation

Status: frozen ownership-closeout observation model

## Purpose

This note freezes the primitive host-call observation and classification
model for
[ADMITTED_SLICE_OWNERSHIP_CLOSEOUT_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/ADMITTED_SLICE_OWNERSHIP_CLOSEOUT_PLAN.md).

The goal of the observation layer is to make the remaining admitted-slice
host-call gap explicit enough that the closeout can distinguish:

- exact engine-authored primitive host-call execution
- normalized-equivalent bounded outcomes when the settled verification rules
  still allow them
- ordering-only host-call drift
- hidden host execution or repair
- missing execution inputs
- true queue, computational, graph, realization, rollback, or verification
  divergence

This observation model does not widen the settled boundary. It exists to
make the retained executor shell diagnosable before the closeout is marked
complete, hybrid, or deferred.

## Observation Surface

The primitive host-call observation layer is a bounded classifier that sits
between the already-settled raw-document-mutation, primitive-execution,
primitive realization or rollback, and final-verification observations:

- the admitted raw document mutation record still describes primitive
  mutation identity
- the admitted primitive execution plan still describes engine-authored
  stage sequencing identity
- the primitive realization or primitive rollback record still describes the
  realized or restored live-object shell
- the final verification record still describes downstream acceptance
  identity
- the primitive host-call observation now classifies the last retained
  low-level host-call shell that links those settled records together

This keeps the proof surface focused on the retained host executor instead
of inventing another comparison vocabulary.

## Frozen Observation Kinds

The admitted primitive host-call classifier is frozen to these kinds:

- `exact`
- `normalized_equivalent`
- `ordering_only`
- `hidden_host_call_orchestration`
- `missing_host_call_inputs`
- `queue_or_state_mismatch`
- `out_of_contract`

Those kinds deliberately stay close to the already-settled raw document
mutation, primitive execution, realization or rollback, and final
verification vocabularies so the closeout can say whether the remaining gap
is truly in the retained executor shell.

## Inputs

The primitive host-call observation is driven from:

- whether the retained primitive host-call shell actually ran
- the already-classified raw document mutation observation
- the already-classified primitive execution observation
- the already-classified primitive realization or primitive rollback
  observation
- the already-classified final verification observation

The host-call layer therefore does not invent a second independent
comparison model. It narrows the interpretation of the existing exact,
normalized-equivalent, ordering, hidden-host, missing-input, and
queue-or-state mismatch signals to the last retained host executor shell.

## Classification Rules

The classification rules are frozen as:

- `exact` when the retained host-call executor ran and the admitted raw
  document mutation, primitive execution, primitive realization or rollback,
  and final verification observations all close exactly
- `normalized_equivalent` when the retained executor ran and the only
  remaining bounded difference is a settled normalized-equivalent outcome
  already allowed downstream
- `ordering_only` when the retained executor ran and the only remaining
  bounded difference is stage or materialization ordering drift that still
  preserves exact admitted state
- `hidden_host_call_orchestration` when the combined retained host-call
  shell still proves hidden host repair or sequencing not represented in
  the engine-authored executor plan
- `missing_host_call_inputs` when host-call drift leaves missing realized,
  restored, or verification inputs
- `queue_or_state_mismatch` when the executor runs but exact queue,
  computational, graph, realization, rollback, or verification proof still
  fails
- `out_of_contract` when the executor never ran, leaves the admitted slice,
  or is missing the required settled upstream or downstream observation
  surfaces

## Diagnostic Expectations

The bounded proof lanes for this cycle must surface:

- whether the retained host-call executor ran at all
- whether the raw document mutation outcome remained exact
- whether primitive execution remained exact or normalized-equivalent
- whether the primitive realization or rollback outcome remained exact
- whether final verification remained exact or normalized-equivalent
- whether queue, computational, and graph proof stayed exact downstream

That is enough to distinguish:

- exact engine-authored primitive host-call execution
- normalized-equivalent bounded outcomes
- ordering-only host-call drift
- hidden host execution or repair
- missing execution inputs
- true queue, computational, graph, realization, rollback, or verification
  divergence

## Guardrails

This observation model is violated if implementation work:

- bypasses the settled raw document mutation, primitive execution,
  realization or rollback, or final verification classifiers and invents a
  conflicting host-call mismatch vocabulary
- widens host-call classification outside the admitted slice
- treats downstream live success as proof of host-call exactness without an
  explicit primitive host-call observation record

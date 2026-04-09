# Computational Substrate Primitive Execution Host Schema

Status: frozen primitive-execution schema

## Purpose

This note freezes the admitted primitive execution record, plan, and verdict
shape for
[COMPUTATIONAL_SUBSTRATE_PRIMITIVE_EXECUTION_HOST_REASSESSMENT_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PRIMITIVE_EXECUTION_HOST_REASSESSMENT_PLAN.md).

The schema exists to keep the proof cycle focused on one narrow question:

- can the admitted primitive execution shell be expressed as an explicit
  engine-authored record or plan that Calc consumes
- without reclaiming hidden primitive execution authority in the host shell

This schema does not widen the admitted slice. It defines the stable record
shape the later observation and implementation phases must use.

## Admitted Primitive Execution Plan

The admitted primitive execution plan should be value-semantic and stable.

The plan should carry:

- explicit linkage to the admitted raw document mutation record already
  produced upstream
- explicit linkage to the admitted primitive realization record for apply
  lanes
- explicit linkage to the admitted primitive rollback record for rollback
  lanes
- explicit stage ordering for the admitted primitive execution shell
- an explicit marker for whether the lane is an apply lane or a rollback
  lane
- an explicit marker for whether the admitted lane still requires final
  verification after primitive execution completes
- an explicit marker that this plan represents primitive execution on the
  admitted slice

The primitive execution plan must remain fully representable without
Calc-local pointers, live object identity, or implicit host-only execution
context.

## Stable Primitive Execution Identity Rules

The admitted primitive execution identity is frozen as:

- the upstream admitted raw document mutation identity
- the upstream admitted primitive realization identity for apply lanes
- the upstream admitted primitive rollback identity for rollback lanes
- the normalized primitive stage order
- whether the admitted lane rolls forward into realization or back into
  rollback
- whether final verification is still required after primitive execution

This is the identity the proof cycle should treat as the stable
engine-authored primitive execution shell. Calc may execute that plan, but
it must not change the identity of the primitive execution it is consuming.

## Normalized Payload Rules

The following payload rules are frozen for this cycle:

- apply lanes must carry explicit primitive realization linkage
- rollback lanes must carry explicit primitive rollback linkage
- apply and rollback linkage may not both be present in the same plan
- admitted stage order must start from the raw document mutation record
- final verification may remain a downstream stage, but the primitive
  execution plan must explicitly declare whether that stage is required
- missing raw document mutation linkage or missing primitive linkage is out
  of contract

These rules are intentionally narrow because the admitted slice is still
bounded to ordinary scalar formulas and admitted single-sheet structural
edits.

## Primitive Execution Verdict Families

The schema freezes four high-level verdict families for this cycle:

- `applied_exact`
- `applied_normalized_equivalent`
- `applied_hybrid`
- `rejected_or_rolled_back`

More detailed observation and diagnosis can still classify:

- exact engine-authored primitive execution
- ordering-only primitive execution
- hidden host primitive execution orchestration or repair
- missing primitive execution inputs caused by earlier shell drift
- queue, computational, graph, or replay mismatch
- out-of-contract failure

But the record-level schema should keep the top-level primitive execution
verdict families simple and stable.

## Before And After State Linkage

The admitted primitive execution plan should link cleanly to the already
settled engine-owned surfaces:

- the raw mutation record remains the earlier admitted request-identity
  surface
- the raw document mutation record remains the earlier primitive mutation
  surface
- the primitive execution plan becomes the explicit host-shell sequencing
  surface
- the primitive realization and primitive rollback records remain the
  explicit realized and restore execution surfaces
- the final verification record remains the downstream acceptance surface

This keeps the migration bounded: primitive execution authority moves
forward without reopening resident storage, mutation-entry, realization,
rollback, or final verification ownership.

## Forbidden Host Shortcuts

The following remain explicitly forbidden in this cycle:

- Calc-originated primitive execution identity that is not represented in
  the engine-authored primitive execution plan
- Calc-only stage sequencing reconstruction after the engine-authored plan
  is built
- host-only promotion from apply to rollback, or rollback to apply, that is
  not represented in the plan
- reclaiming admitted primitive execution authority by skipping the
  explicit plan and falling back to implicit local sequencing
- silently widening primitive execution into shared-group-sensitive,
  named-range-sensitive, off-sheet, or sheet-wide classes

## Guardrails

This schema is violated if implementation work:

- smuggles Calc-only pointer identity into the primitive execution plan
- treats missing primitive linkage as implicitly recoverable
- uses a different primitive execution identity in the host shell than the
  engine-authored plan
- changes admitted raw document mutation, primitive realization, primitive
  rollback, or final verification identity as a side effect of primitive
  execution

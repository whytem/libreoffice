# Admitted Slice Ownership Closeout Schema

Status: frozen ownership-closeout schema

## Purpose

This note freezes the admitted primitive host-call executor shape for
[ADMITTED_SLICE_OWNERSHIP_CLOSEOUT_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/ADMITTED_SLICE_OWNERSHIP_CLOSEOUT_PLAN.md).

The schema exists to keep the proof cycle focused on one narrow question:

- can the retained primitive host-call executor be expressed as an explicit
  engine-authored plan that Calc only consumes
- without reclaiming hidden executor authority in the host shell

This schema does not widen the admitted slice. It defines the stable
executor shape that the later observation and implementation phases must
use.

## Admitted Primitive Host-Call Executor Plan

The admitted primitive host-call executor plan should be value-semantic and
stable.

The plan should carry:

- explicit linkage to the admitted raw document mutation record already
  produced upstream
- explicit linkage to the admitted primitive realization record for apply
  lanes
- explicit linkage to the admitted primitive rollback record for rollback
  lanes
- explicit linkage to the admitted primitive execution plan and the admitted
  final verification record already built upstream
- explicit stage ordering for the retained primitive host-call shell
- explicit markers for whether the lane is an apply lane or a rollback lane
- explicit markers for whether realization, rollback, and final
  verification host calls are expected
- an explicit marker that this plan represents the retained primitive
  host-call executor on the admitted slice

The executor plan must remain fully representable without Calc-local
pointer identity, live-object identity, or implicit host-only execution
context.

## Stable Primitive Host-Call Identity Rules

The admitted primitive host-call executor identity is frozen as:

- the upstream admitted raw document mutation identity
- the upstream admitted primitive realization identity for apply lanes
- the upstream admitted primitive rollback identity for rollback lanes
- the upstream admitted primitive execution identity
- the upstream admitted final verification identity
- the normalized host-call stage order
- whether the admitted lane rolls forward into realization or back into
  rollback

Calc may execute that plan, but it must not change the identity of the
primitive host-call executor it is consuming.

## Normalized Payload Rules

The following payload rules are frozen for this cycle:

- apply lanes must carry explicit primitive realization linkage
- rollback lanes must carry explicit primitive rollback linkage
- apply and rollback linkage may not both be present in the same executor
  plan
- admitted stage order must start from the raw document mutation linkage
- final verification may remain a downstream stage, but the executor plan
  must explicitly declare whether that stage is expected
- missing raw document mutation linkage, missing primitive linkage, or
  missing final verification linkage is out of contract

These rules stay intentionally narrow because the admitted slice is still
bounded to ordinary scalar formulas and admitted single-sheet structural
edits.

## Primitive Host-Call Verdict Families

The schema freezes four high-level verdict families for this cycle:

- `applied_exact`
- `applied_normalized_equivalent`
- `applied_hybrid`
- `rejected_or_rolled_back`

More detailed observation and diagnosis can still classify:

- exact engine-authored primitive host-call execution
- ordering-only host-call drift
- hidden host execution or repair
- missing execution inputs caused by earlier shell drift
- queue, computational, graph, replay, realization, rollback, or
  verification mismatch
- out-of-contract failure

But the record-level schema should keep the top-level executor verdict
families simple and stable.

## Before And After State Linkage

The admitted primitive host-call executor plan should link cleanly to the
already-settled engine-owned surfaces:

- the raw mutation record remains the earlier admitted request-identity
  surface
- the raw document mutation record remains the earlier primitive mutation
  surface
- the primitive execution plan remains the earlier stage-sequencing surface
- the primitive host-call executor plan becomes the last retained low-level
  host-call surface
- the primitive realization and primitive rollback records remain the
  realized and restore execution surfaces
- the final verification record remains the downstream acceptance surface

This keeps the closeout bounded: executor authority moves forward without
reopening resident storage, mutation-entry, realization, rollback, or final
verification ownership.

## Forbidden Host Shortcuts

The following remain explicitly forbidden in this cycle:

- Calc-originated primitive host-call identity that is not represented in
  the engine-authored executor plan
- Calc-only host-call stage sequencing reconstruction after the
  engine-authored executor plan is built
- host-only promotion from apply to rollback, or rollback to apply, that is
  not represented in the executor plan
- reclaiming admitted executor authority by skipping the explicit plan and
  falling back to implicit local sequencing
- silently widening the retained host-call surface into shared-group,
  named-range, off-sheet, or sheet-wide classes

## Guardrails

This schema is violated if implementation work:

- smuggles Calc-only pointer identity into the executor plan
- treats missing host-call linkage as implicitly recoverable
- uses a different primitive host-call identity in the host shell than the
  engine-authored executor plan
- changes admitted raw document mutation, primitive realization, primitive
  rollback, primitive execution, or final verification identity as a side
  effect of executor migration

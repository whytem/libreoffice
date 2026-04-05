# Computational Substrate Live Apply-Shell Schema

Status: frozen live-apply-shell schema

## Purpose

This note freezes the admitted live apply-plan and verdict shape for
[COMPUTATIONAL_SUBSTRATE_LIVE_APPLY_SHELL_REASSESSMENT_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_LIVE_APPLY_SHELL_REASSESSMENT_PLAN.md).

The schema exists to keep the proof cycle focused on one narrow question:

- can the admitted live apply shell be expressed as one explicit
  engine-authored sequencing record that Calc consumes
- without reclaiming hidden orchestration authority in the host shell

This schema does not itself widen the admitted slice. It defines the stable
record shape the later observation and implementation phases must use.

## Admitted Live Apply Plan

The admitted live apply plan should be value-semantic and stable.

The plan should carry:

- normalized mutation identity through the admitted raw mutation record
- normalized admitted realization identity
- normalized admitted rollback identity
- explicit stage ordering for raw mutation, realization, verification, and
  rollback
- explicit stage-presence rules for applied versus rolled-back outcomes

The plan must remain fully representable without Calc-local pointers or live
object identity.

## Stable Stage Identity Rules

The admitted live apply-plan identity is frozen as:

- the admitted raw mutation record identity
- the admitted realization record generation and resident payload identity
- the admitted rollback record generation and resident payload identity
- the ordered stage list consumed by the host shell

This is the identity the proof cycle should treat as the stable engine-owned
apply shell. Calc may execute that plan, but it must not change the identity
or order of the admitted stages it is consuming.

## Normalized Stage Ordering Rules

The following stage-order rules are frozen for this cycle:

- applied outcomes must consume raw mutation before realization
- applied outcomes must perform verification after realization
- rollback-capable outcomes must link the admitted rollback record to the
  same raw mutation and realization context
- rolled-back outcomes may omit a successful realization result, but they
  must still preserve explicit rollback identity
- missing raw mutation identity, missing admitted stage linkage, or
  impossible ordering are out of contract

These rules are intentionally narrow because the admitted slice is still
bounded to ordinary scalar formulas and admitted single-sheet structural
edits.

## Before And After State Linkage

The admitted live apply plan should link cleanly to the already-settled
engine-owned surfaces:

- the raw mutation record remains the entry stage
- the mutation-entry transition remains the after-state decision stage
- the object-realization record remains the live realization stage
- verification remains the explicit comparison stage
- the rollback record remains the restore stage

This keeps the migration bounded: live apply authority moves forward without
reopening resident storage, realization, or rollback ownership.

## Verdict Families

The schema freezes three high-level verdict families for this cycle:

- `applied`
- `applied_normalized_equivalent`
- `rejected_or_rolled_back`

More detailed observation and diagnosis can still classify:

- exact apply sequencing
- ordering-only host execution
- hidden host apply orchestration
- missing realized or rolled-back objects
- queue, computational, graph, or replay mismatch
- out-of-contract failure

But the plan-level schema should keep the top-level verdict families simple
and stable.

## Forbidden Host Shortcuts

The following remain explicitly forbidden in this cycle:

- Calc-originated stage ordering that is not represented in the admitted
  apply plan
- Calc-only re-linking between raw mutation, realization, and rollback
  records before execution
- host-only verification staging not represented in the plan
- reclaiming admitted apply-shell sequencing in the live host shell

## Guardrails

This schema is violated if implementation work:

- smuggles Calc-only pointer identity into the admitted apply plan
- treats missing stage linkage as implicitly recoverable
- widens to shared-group, named-range-sensitive, or sheet-wide classes
- uses a different stage order in the host shell than the engine-authored
  plan

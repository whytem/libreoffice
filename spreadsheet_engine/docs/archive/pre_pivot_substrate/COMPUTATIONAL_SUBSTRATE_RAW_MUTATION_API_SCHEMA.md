# Computational Substrate Raw Mutation API Schema

Status: frozen raw-mutation schema

## Purpose

This note freezes the admitted raw mutation request and verdict shape for
[COMPUTATIONAL_SUBSTRATE_RAW_MUTATION_API_MIGRATION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_RAW_MUTATION_API_MIGRATION_PLAN.md).

The schema exists to keep the proof cycle focused on one narrow question:

- can the admitted raw mutation shell be expressed as an explicit
  engine-authored record that Calc consumes
- without reclaiming hidden mutation authority in the host shell

This schema does not itself widen the admitted slice. It defines the stable
record shape the later observation and implementation phases must use.

## Admitted Raw Mutation Record

The admitted raw mutation record should be value-semantic and stable.

The record should carry:

- normalized mutation kind
- normalized target address or structural anchor
- normalized structural count when relevant
- explicit scalar payload for admitted scalar overwrite
- explicit formula source for admitted formula replace
- optional linkage back to admitted cached formula value when already carried
  by the mutation-entry request

The record must remain fully representable without Calc-local pointers or
live object identity.

## Stable Identity Rules

The admitted raw mutation record identity is frozen as:

- mutation kind
- sheet
- anchor column and row
- structural count when relevant
- scalar or formula payload when relevant

This is the identity the proof cycle should treat as the stable engine-owned
mutation shell. Calc may execute that record, but it must not change the
identity of the mutation it is consuming.

## Normalized Payload Rules

The following payload rules are frozen for this cycle:

- scalar overwrite must carry an explicit admitted `CellValue`
- formula replace must carry an explicit admitted formula source string
- `ClearCell` carries no after payload
- row and column structural mutations must carry a positive explicit count
- empty formula source, missing scalar payload, or non-positive structural
  count are out of contract

These rules are intentionally narrow because the admitted slice is still
bounded to ordinary scalar formulas and admitted single-sheet structural
edits.

## Before And After State Linkage

The admitted raw mutation record should link cleanly to the already-settled
engine-owned surfaces:

- the raw mutation record is the entry record
- the mutation-entry transition remains the after-state decision surface
- the object-realization record remains the live apply surface
- the rollback record remains the restore surface

This keeps the migration bounded: raw mutation authority moves forward
without reopening after-state, realization, or rollback ownership.

## Verdict Families

The schema freezes three high-level verdict families for this cycle:

- `applied`
- `applied_normalized_equivalent`
- `rejected_or_rolled_back`

More detailed observation and diagnosis can still classify:

- exact apply
- ordering-only apply
- hidden host reconstruction
- missing realized or rolled-back live objects
- queue, computational, graph, or replay mismatch
- out-of-contract failure

But the record-level schema should keep the top-level verdict families simple
and stable.

## Forbidden Host Shortcuts

The following remain explicitly forbidden in this cycle:

- Calc-originated mutation identity that is not represented in the admitted
  raw mutation record
- Calc-only scalar or formula payload reconstruction before the engine-owned
  record is built
- host-only structural count normalization not represented in the record
- reclaiming admitted mutation intent in the live apply shell

## Guardrails

This schema is violated if implementation work:

- smuggles Calc-only pointer identity into the raw mutation record
- treats missing scalar or formula payload as implicitly recoverable
- widens to shared-group, named-range-sensitive, or sheet-wide mutation
  classes
- uses a different mutation identity in the host shell than the engine-owned
  record

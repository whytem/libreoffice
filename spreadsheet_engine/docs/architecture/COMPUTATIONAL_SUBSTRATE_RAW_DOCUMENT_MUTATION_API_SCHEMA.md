# Computational Substrate Raw Document Mutation API Schema

Status: frozen raw-document-mutation schema

## Purpose

This note freezes the admitted primitive mutation record and verdict shape
for
[COMPUTATIONAL_SUBSTRATE_RAW_DOCUMENT_MUTATION_API_MIGRATION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_RAW_DOCUMENT_MUTATION_API_MIGRATION_PLAN.md).

The schema exists to keep the proof cycle focused on one narrow question:

- can the admitted primitive raw document mutation shell be expressed as an
  explicit engine-authored record that Calc consumes
- without reclaiming hidden primitive mutation authority in the host shell

This schema does not widen the admitted slice. It defines the stable record
shape the later observation and implementation phases must use.

## Admitted Primitive Mutation Record

The admitted primitive mutation record should be value-semantic and stable.

The record should carry:

- normalized mutation kind
- normalized target address or structural anchor
- normalized structural count when relevant
- explicit scalar payload for admitted scalar overwrite
- explicit formula source for admitted formula replace
- optional linkage back to admitted cached formula value when already
  carried by the settled raw-mutation record
- optional explicit link back to the admitted raw mutation record identity
  already produced earlier in the pipeline

The primitive mutation record must remain fully representable without
Calc-local pointers, live object identity, or implicit host-only mutation
context.

## Stable Primitive Identity Rules

The admitted primitive mutation record identity is frozen as:

- mutation kind
- sheet
- anchor column and row
- structural count when relevant
- scalar or formula payload when relevant
- optional cached formula value identity when already present in the
  admitted raw-mutation record

This is the identity the proof cycle should treat as the stable engine-owned
primitive mutation shell. Calc may execute that record, but it must not
change the identity of the primitive mutation it is consuming.

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

## Primitive Apply Result And Verdict Families

The schema freezes two related result surfaces for this cycle:

- primitive record build verdict
- primitive mutation apply verdict

The top-level verdict families remain:

- `applied`
- `applied_normalized_equivalent`
- `rejected_or_rolled_back`

More detailed observation and diagnosis can still classify:

- exact primitive mutation execution
- ordering-only host execution
- hidden host mutation orchestration or repair
- missing realized or rolled-back live objects caused by primitive mutation
  drift
- queue, computational, graph, or replay mismatch
- out-of-contract failure

But the record-level schema should keep the top-level primitive verdict
families simple and stable.

## Before And After State Linkage

The admitted primitive mutation record should link cleanly to the already
settled engine-owned surfaces:

- the admitted raw mutation record remains the earlier identity surface
- the primitive mutation record becomes the explicit execution surface
- the mutation-entry transition remains the after-state decision surface
- the object-realization record remains the live apply surface
- the rollback record remains the restore surface
- the live apply plan remains the sequencing surface

This keeps the migration bounded: primitive mutation authority moves
forward without reopening after-state, realization, rollback, or live apply
ownership.

## Forbidden Host Shortcuts

The following remain explicitly forbidden in this cycle:

- Calc-originated primitive mutation identity that is not represented in
  the engine-authored primitive mutation record
- Calc-only scalar or formula payload reconstruction before the
  engine-authored primitive mutation record is built
- host-only structural count normalization not represented in the record
- reclaiming admitted primitive mutation authority inside the live apply
  shell
- silently widening primitive execution into shared-group-sensitive,
  named-range-sensitive, or sheet-wide classes

## Guardrails

This schema is violated if implementation work:

- smuggles Calc-only pointer identity into the primitive mutation record
- treats missing scalar or formula payload as implicitly recoverable
- uses a different primitive mutation identity in the host shell than the
  engine-authored record
- changes admitted live apply, realization, or rollback identity as a side
  effect of primitive mutation execution

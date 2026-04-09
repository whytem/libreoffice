# Computational Substrate Primitive Realization And Rollback Schema

Status: frozen primitive-realization-rollback schema

## Purpose

This note freezes the admitted primitive realization and rollback record and
verdict shape for
[COMPUTATIONAL_SUBSTRATE_PRIMITIVE_REALIZATION_ROLLBACK_REASSESSMENT_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PRIMITIVE_REALIZATION_ROLLBACK_REASSESSMENT_PLAN.md).

The schema exists to keep the proof cycle focused on one narrow question:

- can the admitted primitive realization and rollback shell be expressed as
  explicit engine-authored records that Calc consumes
- without reclaiming hidden realization or rollback authority in the host
  shell

This schema does not widen the admitted slice. It defines the stable record
shape the later observation and implementation phases must use.

## Admitted Primitive Realization Record

The admitted primitive realization record should be value-semantic and
stable.

The record should carry:

- explicit linkage to the admitted object-realization record already
  produced upstream
- stable realization generation
- explicit formula-cell lifetime realization payload
- explicit resident cell-storage realization payload
- explicit resident wiring realization payload
- an explicit marker that this record represents primitive realization on
  the admitted slice

The primitive realization record must remain fully representable without
Calc-local pointers, live object identity, or implicit host-only execution
context.

## Admitted Primitive Rollback Record

The admitted primitive rollback record should also be value-semantic and
stable.

The record should carry:

- explicit linkage to the admitted rollback record already produced
  upstream
- stable rollback generation
- explicit restore payload for realized objects and resident state
- explicit formula-tree and formula-track restore payload carried through
  the admitted rollback record
- an explicit marker that this record represents primitive rollback on the
  admitted slice

The primitive rollback record must remain fully representable without
Calc-local pointers or hidden host-only restore context.

## Stable Identity Rules

The admitted primitive realization and rollback identity is frozen as:

- the upstream admitted object-realization or rollback record identity
- generation
- formula-cell lifetime payload identity
- resident cell-storage payload identity
- resident wiring payload identity
- formula-state restore identity when rollback is involved

This is the identity the proof cycle should treat as the stable engine-owned
primitive realization and rollback shell. Calc may execute those records,
but it must not change the identity of the primitive shell it is consuming.

## Normalized Payload Rules

The following payload rules are frozen for this cycle:

- primitive realization must carry all three admitted realized payloads:
  formula-cell lifetime, resident cell storage, and resident wiring
- primitive rollback must carry a full admitted rollback record, including
  object-realization restore payload and formula-state restore payload
- missing realized payload linkage or missing rollback linkage is out of
  contract
- empty or partial realized payload recovery by Calc is out of contract

These rules are intentionally narrow because the admitted slice is still
bounded to ordinary scalar formulas and admitted single-sheet structural
edits.

## Verdict Families

The schema freezes three high-level verdict families for this cycle:

- `applied`
- `applied_normalized_equivalent`
- `rejected_or_rolled_back`

More detailed observation and diagnosis can still classify:

- exact primitive realization or rollback execution
- ordering-only host execution
- hidden host realization or rollback orchestration
- missing realized or restored live objects
- queue, computational, graph, or replay mismatch
- out-of-contract failure

But the record-level schema should keep the top-level primitive verdict
families simple and stable.

## Before And After State Linkage

The admitted primitive realization and rollback records should link cleanly
to the already-settled engine-owned surfaces:

- raw mutation and raw document mutation remain the earlier execution-entry
  surfaces
- the live apply plan remains the sequencing surface
- the object-realization record remains the upstream realization identity
  surface
- the rollback record remains the upstream restore identity surface
- primitive realization and rollback records become the explicit host-shell
  execution surface

This keeps the migration bounded: primitive realization and rollback
authority move forward without reopening resident storage, mutation-entry,
or live-apply ownership.

## Forbidden Host Shortcuts

The following remain explicitly forbidden in this cycle:

- Calc-originated realization or rollback identity that is not represented
  in the engine-authored primitive records
- Calc-only payload reconstruction before the engine-authored primitive
  realization or rollback record is built
- host-only generation normalization not represented in the record
- reclaiming admitted realization or rollback authority inside the final
  verification host shell

## Guardrails

This schema is violated if implementation work:

- smuggles Calc-only pointer identity into the primitive realization or
  rollback record
- treats missing realized or rollback payload as implicitly recoverable
- uses a different primitive realization or rollback identity in the host
  shell than the engine-authored records
- changes admitted raw document mutation or live-apply identity as a side
  effect of primitive realization or rollback execution

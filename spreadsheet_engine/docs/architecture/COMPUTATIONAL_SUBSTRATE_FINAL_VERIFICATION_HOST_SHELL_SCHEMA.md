# Computational Substrate Final Verification Host-Shell Schema

Status: frozen final-verification schema

## Purpose

This note freezes the admitted final verification record and verdict shape
for
[COMPUTATIONAL_SUBSTRATE_FINAL_VERIFICATION_HOST_SHELL_REASSESSMENT_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FINAL_VERIFICATION_HOST_SHELL_REASSESSMENT_PLAN.md).

The schema exists to keep the proof cycle focused on one narrow question:

- can the admitted final verification shell be expressed as an explicit
  engine-authored record or plan that Calc consumes
- without reclaiming hidden verification authority in the host shell

This schema does not widen the admitted slice. It defines the stable record
shape the later observation and implementation phases must use.

## Admitted Final Verification Record

The admitted final verification record should be value-semantic and stable.

The record should carry:

- explicit linkage to the admitted live apply plan already produced
  upstream
- explicit linkage to the admitted primitive realization or primitive
  rollback record consumed before verification
- normalized expected queue outcome
- normalized expected computational outcome
- normalized expected graph outcome
- normalized expected execution-IR outcome when it is part of the admitted
  path
- normalized expected broadcaster-canonicalization outcome when it is part
  of the admitted path
- an explicit marker that this record represents final verification on the
  admitted slice

The final verification record must remain fully representable without
Calc-local pointers, live object identity, or implicit host-only
verification context.

## Stable Verification Identity Rules

The admitted final verification identity is frozen as:

- the upstream admitted live apply plan identity
- the upstream admitted primitive realization or primitive rollback
  identity
- verification mode expectations for queue, computational, graph, and
  execution-IR comparison
- normalized broadcaster-canonicalization expectations when relevant
- whether the admitted lane is an apply lane or a rollback lane

This is the identity the proof cycle should treat as the stable
engine-authored verification shell. Calc may execute that verification, but
it must not change the identity of the verification it is consuming.

## Normalized Payload Rules

The following payload rules are frozen for this cycle:

- apply lanes must carry explicit primitive realization linkage
- rollback lanes must carry explicit primitive rollback linkage
- queue, computational, and graph expectations are mandatory
- execution-IR expectations may be omitted only when the admitted path does
  not produce an IR comparison
- missing live-apply linkage or missing primitive linkage is out of
  contract
- missing mandatory comparison expectations are out of contract

These rules are intentionally narrow because the admitted slice is still
bounded to ordinary scalar formulas and admitted single-sheet structural
edits.

## Verification Verdict Families

The schema freezes three high-level verdict families for this cycle:

- `verified_exact`
- `verified_normalized_equivalent`
- `rejected_or_rolled_back`

More detailed observation and diagnosis can still classify:

- exact engine-authored final verification
- ordering-only host verification
- hidden host verification orchestration or repair
- missing verification inputs caused by primitive shell drift
- queue, computational, graph, or replay mismatch
- out-of-contract failure

But the record-level schema should keep the top-level final verification
verdict families simple and stable.

## Before And After State Linkage

The admitted final verification record should link cleanly to the
already-settled engine-owned surfaces:

- the raw mutation and raw document mutation records remain the earlier
  primitive execution identity surfaces
- the live apply plan remains the sequencing surface
- the object-realization and rollback records remain the broader realized
  and restore identity surfaces
- the primitive realization and primitive rollback records remain the
  explicit host-shell execution surfaces
- the final verification record becomes the explicit acceptance surface

This keeps the migration bounded: verification authority moves forward
without reopening resident storage, mutation-entry, realization, or
primitive execution ownership.

## Forbidden Host Shortcuts

The following remain explicitly forbidden in this cycle:

- Calc-originated verification identity that is not represented in the
  engine-authored final verification record
- Calc-only acceptance criteria reconstruction after the engine-authored
  verification record is built
- host-only normalization not represented in the verification record
- reclaiming admitted verification authority by skipping the explicit
  verification record and falling back to implicit local acceptance rules

## Guardrails

This schema is violated if implementation work:

- smuggles Calc-only pointer identity into the final verification record
- treats missing comparison expectations as implicitly recoverable
- uses a different verification identity in the host shell than the
  engine-authored record
- changes admitted live apply, primitive realization, or primitive rollback
  identity as a side effect of final verification

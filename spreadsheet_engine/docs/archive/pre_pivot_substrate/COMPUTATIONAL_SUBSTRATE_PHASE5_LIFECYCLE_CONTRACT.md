# Computational Substrate Phase 5 Lifecycle Contract

Status: active contract note for Phase 5

## Purpose

This note freezes the admitted lifecycle-authority surface for Phase 5.

It exists to keep the first lifecycle pilot narrow and to make it explicit
which formula-bearing mutations count as engine-owned lifecycle in this phase.

## Phase 5 Meaning Of Lifecycle Authority

For Phase 5, "engine-owned formula lifecycle" means:

- the engine decides whether an admitted mutation inserts, replaces, or removes
  a formula-bearing cell
- the engine owns the post-mutation computational shape for that cell
- the engine owns the admitted formula-tree and formula-track membership answer
  for the same mutation
- Calc is limited to applying the resulting host synchronization work,
  capturing verification state, and rolling back on divergence

Phase 5 does not yet mean:

- full formula-cell lifecycle ownership across all mutation classes
- shared-group lifecycle ownership
- listener or broadcaster storage ownership
- structural-edit lifecycle authority
- broad repair authority during load, clipboard, or undo-like flows

## Admitted Mutation Surface

The admitted live lifecycle subset is:

- formula replacement on an existing single-cell scalar formula via
  `SetString("="...)`
- formula replacement on an existing single-cell scalar formula via
  `SetFormula`
- direct formula insertion into a non-formula cell via `SetFormula`
- `ClearCell` removal of an existing single-cell scalar formula

These admitted cases require:

- a clean formula-tree and formula-track baseline before the mutation
- no pre-existing dirty authority debt
- no shared-group membership on the target formula before or after the
  lifecycle transition
- no structural side effects beyond the single target address

## Validation-Only And Rejected Mutation Classes

Validation-only in Phase 5:

- named-range-sensitive lifecycle cases
- representative shared-group cases in differential-only lanes

Rejected in the live lifecycle pilot:

- `SetValue`
- scalar `SetString` that does not create or replace a formula
- shared-group creation, split, merge, or repair
- row or column insert/delete
- copy, move, clipboard, load-time, undo-like, and repair-heavy mutations

## Accepted Verification Semantics

Phase 5 requires exact lifecycle verification for:

- formula-bearing cell population at the target address
- formula-tree membership
- formula-track membership
- graph-facing state on the admitted subset

Execution-IR comparison remains observation data in Phase 5 unless the checked-
in evidence later proves it should become a harder gate.

## Hard Stop Conditions

The lifecycle pilot must reject or roll back when:

- the baseline is dirty
- the mutation drifts outside the admitted lifecycle subset
- the post-apply computational shape diverges from the engine-owned answer
- Calc silently repairs or widens the lifecycle beyond the engine-owned answer
- queue or graph verification fails

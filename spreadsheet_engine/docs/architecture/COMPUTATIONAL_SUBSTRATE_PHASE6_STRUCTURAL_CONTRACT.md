# Computational Substrate Phase 6 Structural Contract

Status: active contract note for Phase 6

## Purpose

This note freezes the admitted structural-authority surface for Phase 6.

It exists to keep the first structural and reference-update pilot narrow and
to make it explicit which structural mutations count as engine-owned in this
phase.

## Phase 6 Meaning Of Structural Authority

For Phase 6, "engine-owned structural authority" means:

- the engine classifies whether an admitted structural mutation is eligible
  for the pilot
- the engine owns the admitted post-mutation cell population and formula-tree
  membership answer on the narrowed subset
- the engine owns the admitted reference-update expectation for shifted scalar
  formulas on that subset
- Calc is limited to hosting the live mutation, exposing the resulting
  workbook state for comparison, and rolling back on divergence

Phase 6 does not yet mean:

- full structural-edit ownership for Calc documents
- shared-group structural lifecycle ownership
- named-range structural authority in the live path
- sheet insert, delete, rename, or move authority
- listener or broadcaster storage ownership
- `ScTokenArray` ownership or broad repair authority

## Admitted Structural Mutation Surface

The admitted live structural subset is:

- `InsertRows` on a single sheet
- `DeleteColumns` on a single sheet

These admitted cases require:

- a clean formula-tree and formula-track baseline before the mutation
- a workbook slice containing only ordinary scalar formulas
- no shared-group membership before or after the mutation
- no named ranges in the admitted sheet slice
- no external-reference, copy, move, clipboard, load-time, or undo-like
  behavior
- local reference updates only for formulas that remain ordinary single-cell
  formulas after the mutation

## Validation-Only And Rejected Mutation Classes

Validation-only in Phase 6:

- `DeleteRows`
- `InsertColumns`
- representative named-range-sensitive structural cases in differential-only
  lanes

Rejected in the live structural pilot:

- `SetValue`
- scalar `SetString`
- `SetFormula`
- `ClearCell`
- `ClearRange`
- `MoveRange`
- `CopyRange`
- `RenameSheet`
- `AddNamedRange`
- `RemoveNamedRange`
- `RenameNamedRange`
- shared-group creation, split, merge, or repair
- sheet insert, delete, or move

## Accepted Verification Semantics

Phase 6 requires exact verification for:

- cell population on the admitted structural slice
- formula-tree membership on the admitted structural slice
- queue comparison
- graph comparison on the admitted subset

Execution-IR comparison remains observation data in Phase 6, but the pilot is
still required to produce an explicit reference-update expectation for the
admitted formulas. A mismatch between that expectation and the live after-state
is treated as unacceptable structural repair.

## Hard Stop Conditions

The structural pilot must reject or roll back when:

- the baseline is dirty
- the mutation drifts outside the admitted structural subset
- the post-apply cell population diverges from the engine-owned answer
- the post-apply queue or graph state diverges from the admitted answer
- Calc silently repairs or widens the admitted reference updates
- the mutation requires retained Calc ownership for shared groups, named
  ranges, or broader repair semantics

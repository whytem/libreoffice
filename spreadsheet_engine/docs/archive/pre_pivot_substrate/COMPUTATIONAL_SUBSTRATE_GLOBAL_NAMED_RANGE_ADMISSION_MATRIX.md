# Computational Substrate Global Named-Range Admission Matrix

Status: completed workbook and mutation matrix for the global named-range
admission plan

## Purpose

This matrix freezes the representative workbook and mutation classes that the
global named-range admission proof cycle must cover.

It keeps the promotion question precise by separating:

- candidate live-admission scenarios
- validation-only proof scenarios
- explicit reject or defer scenarios

## Classification Rules

`candidate live admission` means the scenario is allowed to argue for adding
the bounded global single-area named-range slice to the opt-in rollout if the
later evidence is exact and deterministic.

`validation-only proof surface` means the scenario must still produce checked-
in evidence, but does not by itself justify live admission in this cycle.

`explicit reject or defer` means the scenario stays out of contract and should
continue to reject or roll back deterministically.

## Scenario Matrix

### Candidate live-admission scenarios

`GN-C1`
- named-range class: global single-cell target
- formula location: edited sheet
- structural edit: `InsertRows` before the target
- expected behavior: target and dependent formula addresses shift together
- classification: candidate live admission

`GN-C2`
- named-range class: global single-area target
- formula location: edited sheet
- structural edit: `DeleteRows` crossing the top edge of the target area
- expected behavior: target narrows exactly without leaving the scalar slice
- classification: candidate live admission

`GN-C3`
- named-range class: global single-area target
- formula location: edited sheet
- structural edit: `InsertColumns` before the target area
- expected behavior: target shifts right with exact queue, graph, and
  computational agreement
- classification: candidate live admission

`GN-C4`
- named-range class: global single-area target
- formula location: edited sheet
- structural edit: `DeleteColumns` crossing one edge of the target area
- expected behavior: target narrows deterministically and formulas remain
  ordinary scalar formulas
- classification: candidate live admission

`GN-C5`
- named-range class: global single-area target
- formula location: another sheet
- structural edit: same-sheet edit on the target sheet
- expected behavior: off-sheet formulas keep the same formula text while the
  global target rewrite and resulting graph/queue state remain exact
- classification: candidate live admission

`GN-C6`
- named-range class: global single-area target
- formula location: edited sheet
- structural edit: starts adjacent to, but not inside, the target
- expected behavior: no semantic shift beyond the admitted structural update
  and exact no-op target preservation where applicable
- classification: candidate live admission

### Validation-only proof scenarios

`GN-V1`
- named-range class: global single-area target with explicit sheet prefix in
  the target source text
- formula location: edited sheet
- structural edit: `InsertColumns` before the target area
- expected behavior: exact target rewrite preserves sheet-prefix shape
- classification: validation-only proof surface

`GN-V2`
- named-range class: global single-area target
- formula location: another sheet
- structural edit: `DeleteRows` crossing the target area
- expected behavior: proves whether off-sheet consumers behave homogeneously
  enough to stay inside the live-admission surface
- classification: validation-only proof surface

`GN-V3`
- named-range class: global single-area target
- formula location: edited sheet
- structural edit: same mutation classes as `GN-C1` through `GN-C4`, but with
  deliberate dirty-baseline entry
- expected behavior: deterministic `RejectedDirtyBaseline`
- classification: validation-only proof surface

`GN-V4`
- named-range class: global single-area target
- formula location: edited sheet
- structural edit: same mutation classes as `GN-C1` through `GN-C4`, but with
  deliberate post-edit divergence injection
- expected behavior: deterministic `RepairDetected` with rollback
- classification: validation-only proof surface

### Explicit reject or defer scenarios

`GN-D1`
- named-range class: sheet-local named range
- formula location: any
- structural edit: any admitted row or column edit
- expected behavior: clean reject because local scope is outside the contract
- classification: explicit reject or defer

`GN-D2`
- named-range class: multi-area named range
- formula location: any
- structural edit: any admitted row or column edit
- expected behavior: clean reject because union rewriting is outside the
  candidate slice
- classification: explicit reject or defer

`GN-D3`
- named-range class: overlapping global and sheet-local names with identical
  lookup text
- formula location: any
- structural edit: any admitted row or column edit
- expected behavior: clean reject because descriptor ambiguity remains outside
  contract
- classification: explicit reject or defer

`GN-D4`
- named-range class: any
- formula location: any
- structural edit: case requiring shared-group repair, matrix behavior, or
  another non-scalar structural path
- expected behavior: clean reject or repair-detected rollback
- classification: explicit reject or defer

## Minimum Test Coverage Mapping

The proof cycle must cover at least:

- one same-sheet happy-path case from `GN-C1` through `GN-C4`
- one off-sheet consumer case from `GN-C5`
- one adjacency or no-shift case from `GN-C6`
- one explicit-sheet-prefix case from `GN-V1`
- one dirty-baseline case from `GN-V3`
- one deliberate divergence case from `GN-V4`
- one clean reject for `GN-D1`
- one clean reject for `GN-D2` or `GN-D3`

If the evidence closes without this shape, the admission note is incomplete.

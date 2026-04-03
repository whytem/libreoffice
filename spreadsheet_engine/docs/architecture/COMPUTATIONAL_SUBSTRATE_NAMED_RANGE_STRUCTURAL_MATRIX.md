# Computational Substrate Named-Range Structural Matrix

Status: complete workbook and mutation matrix for the named-range widening plan

## Purpose

This matrix freezes the representative workbook and mutation classes that the
named-range-sensitive structural proof cycle must cover.

It keeps the implementation honest by separating:

- candidate live-promotion cases
- validation-only proof cases
- explicit defer cases

## Classification Rules

`candidate live promotion` means the scenario is allowed to argue for widening
the opt-in rollout if the later evidence is exact and deterministic.

`validation-only proof surface` means the scenario must run through the pilot
and produce evidence, but does not argue for live rollout promotion in this
cycle.

`explicit defer` means the scenario is out of scope for the current proof
cycle and should continue to reject cleanly.

## Scenario Matrix

### Candidate live-promotion scenarios

`NR-C1`
- named range class: global single-cell target
- formula location: edited sheet
- structural edit: `InsertRows` before the named target
- expected behavior: target and dependent formula addresses shift together
- classification: candidate live promotion

`NR-C2`
- named range class: global single-area target
- formula location: edited sheet
- structural edit: `DeleteRows` crossing the top edge of the target area
- expected behavior: target narrows deterministically and dependent formulas
  stay ordinary scalar formulas
- classification: candidate live promotion

`NR-C3`
- named range class: global single-area target
- formula location: edited sheet
- structural edit: `InsertColumns` before the target area
- expected behavior: target shifts right with exact queue, graph, and
  computational agreement
- classification: candidate live promotion

`NR-C4`
- named range class: global single-area target
- formula location: edited sheet
- structural edit: `DeleteColumns` crossing the target area
- expected behavior: target narrows deterministically without hidden repair
- classification: candidate live promotion

`NR-C5`
- named range class: global single-area target
- formula location: edited sheet
- structural edit: row or column edit adjacent to, but not crossing, the
  target
- expected behavior: target stays semantically unchanged and the pilot proves
  exact non-shift behavior
- classification: candidate live promotion

### Validation-only proof scenarios

`NR-V1`
- named range class: global single-area target
- formula location: another sheet
- structural edit: `InsertRows` or `DeleteRows` on the target sheet
- expected behavior: cross-sheet formulas continue to resolve through the
  named range after the edit
- classification: validation-only proof surface

`NR-V2`
- named range class: sheet-local single-cell target
- formula location: same scoped sheet
- structural edit: `InsertColumns` before the target
- expected behavior: scope remains stable and target shifts exactly
- classification: validation-only proof surface

`NR-V3`
- named range class: sheet-local single-area target
- formula location: another sheet referencing the local name through an
  unambiguous scoped formula
- structural edit: `DeleteColumns` crossing the target
- expected behavior: scope remains sheet-local and the target rewrite is exact
- classification: validation-only proof surface

`NR-V4`
- named range class: global single-area target
- formula location: edited sheet
- structural edit: edit starts inside the target and widens or narrows only
  one edge
- expected behavior: pilot records whether exact target rewriting stays stable
- classification: validation-only proof surface

`NR-V5`
- named range class: global single-area target
- formula location: edited sheet
- structural edit: same edit class as `NR-C1` through `NR-C4`, but with a
  dirty baseline or deliberate divergence injection
- expected behavior: deterministic rejection or rollback
- classification: validation-only proof surface

### Explicit defer scenarios

`NR-D1`
- named range class: multi-area target
- formula location: any
- structural edit: any admitted row or column insert/delete
- expected behavior: clean reject because the proof cycle does not model union
  rewriting
- classification: explicit defer

`NR-D2`
- named range class: overlapping global and sheet-local names with the same
  lookup text
- formula location: any
- structural edit: any admitted row or column insert/delete
- expected behavior: clean reject because scope ambiguity is outside the
  current comparison contract
- classification: explicit defer

`NR-D3`
- named range class: any
- formula location: any
- structural edit: causes shared-group repair, matrix behavior, or another
  non-scalar structural repair path
- expected behavior: clean reject or repair-detected rollback
- classification: explicit defer

`NR-D4`
- named range class: any
- formula location: any
- structural edit: would require sheet create/delete/rename/move or
  scope-transfer semantics
- expected behavior: clean reject because the edit is outside the row/column
  structural surface
- classification: explicit defer

## Minimum Test Coverage Mapping

The later proof cycle must cover at least:

- one happy-path Calc differential case from `NR-C1` through `NR-C4`
- one adjacency or no-shift case from `NR-C5`
- one cross-sheet validation-only case from `NR-V1`
- one sheet-local validation-only case from `NR-V2` or `NR-V3`
- one deliberate divergence or dirty-baseline case from `NR-V5`
- one clean reject for `NR-D1`
- one clean reject for `NR-D2` or `NR-D3`

If the proof cycle closes without this shape, the evidence note is incomplete.

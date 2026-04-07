# Computational Substrate Shared-Group Non-Structural Replacement-Merge Scenario Matrix

Status: frozen scenario matrix for the exact replacement-merge closeout

## Admitted Scenarios

- Anchor-edge upward replacement merge:
  - before `B1:B2` is one shareable group, `B3:B4` is the touched shareable
    group
  - `SetFormula(B3, "=A3*3")`
  - after `B1:B3` is one exact shareable group and `B4` is left outside the
    merged group
  - expected handling: admitted
- Tail-edge downward replacement merge:
  - before `B1:B2` is the touched shareable group, `B3:B4` is one adjacent
    shareable group
  - `SetFormula(B2, "=A2*3")`
  - after `B2:B4` is one exact shareable group and `B1` is left outside the
    merged group
  - expected handling: admitted

## Retained Reject Scenarios

- One-sided adjacent insert next to only one prior shared group:
  - expected handling: reject
- Three-group collapse driven from one touched edge:
  - expected handling: reject
- Named-range-combined replacement merge:
  - expected handling: reject
- Repair-sensitive host normalization:
  - expected handling: reject or repair-detected
- Off-sheet replacement-merge consumers:
  - expected handling: reject

## Distinguishing Lines

- Gap-closing merge inserts into a blank touched address; replacement merge
  edits an already-shared touched address.
- Edge-regroup absorbs only adjacent ordinary formulas; replacement merge
  absorbs one adjacent prior shared group.
- Broader merge or regroup classes remain outside the admitted matrix even
  if live Calc later produces a grouped after-state.

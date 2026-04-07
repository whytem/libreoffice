# Computational Substrate Shared-Group Non-Structural Merge Completion Scenario Matrix

Status: frozen scenario matrix for the same-sheet merge-completion closeout

## Admitted Scenarios

- Downward one-sided insert:
  - before `B1:B2` is one shareable group and `B3` is blank
  - `SetFormula(B3, "=A3*2")`
  - after `B1:B3` is one exact shareable group
  - expected handling: admitted
- Upward one-sided insert:
  - before `B2:B3` is one shareable group and `B1` is blank
  - `SetFormula(B1, "=A1*2")`
  - after `B1:B3` is one exact shareable group
  - expected handling: admitted

## Retained Reject Scenarios

- Three-group collapse into one merged after-group:
  - expected handling: reject
- Named-range-combined merge:
  - expected handling: reject
- Repair-sensitive host normalization:
  - expected handling: reject or repair-detected
- Off-sheet merge consumer widening:
  - expected handling: reject

## Distinguishing Lines

- Gap-closing merge inserts between two prior shared groups; one-sided
  insert extends exactly one prior shared group.
- Replacement merge edits an already-shared touched cell; one-sided insert
  edits a blank touched address.
- Broader multi-group collapse remains outside the admitted matrix even if
  live Calc later produces a grouped after-state.

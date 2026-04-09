# Computational Substrate Shared-Group Non-Structural Multi-Group Collapse Scenario Matrix

Status: frozen scenario matrix for the exact multi-group collapse closeout

## Exact Standalone Scenario

- Synthetic three-participant full-span collapse:
  - before `B1:B2` shared `*3`, `B3:B4` shared `*2`, `B5:B6` shared `*3`
  - `SetFormula(B3, "=A3*3")`
  - synthetic after `B1:B6` one exact shareable group
  - expected handling: exact standalone model closes

## Live Host Scenario

- Live three-group attempt:
  - before `B1:B2` shared `*3`, `B3:B4` shared `*2`, `B5:B6` shared `*3`
  - `SetFormula(B3, "=A3*3")`
  - live after `B1:B3` grouped and `B5:B6` remains a separate group
  - expected handling: no multi-group-collapse admission

## Retained Reject Scenarios

- Four-plus-group collapse into one after-group:
  - expected handling: reject
- Named-range-combined multi-group collapse:
  - expected handling: reject
- Repair-sensitive host normalization:
  - expected handling: reject or repair-detected
- Off-sheet collapse consumer widening:
  - expected handling: reject

## Distinguishing Line

- A family counts as admitted multi-group collapse only if the live host
  after-state itself is one exact full-span group.
- If the far participant group remains separate, the cycle stays outside the
  admitted slice even if a synthetic exact model can be authored.

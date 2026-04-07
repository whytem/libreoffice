# Computational Substrate Shared-Group Non-Structural Multi-Group Collapse Contract

Status: frozen contract for the exact multi-group collapse closeout

## Candidate Family

The bounded candidate family for this cycle was:

- same-sheet shareable shared-group `SetFormula`
- touched address shared before and after
- exactly three adjacent same-column prior shareable groups
- touched address inside the middle participant group
- engine-authored rebuild window spanning all three prior groups
- exact one-group after-topology covering that full span
- no named-range drift
- no off-sheet dependency expansion
- clean baseline only

## Admission Standard

This family could admit only if all of the following closed together:

- participant discovery was engine-authored
- the full-span after-group was engine-authored
- live Calc exposed the same one-group after-topology
- lifecycle and mutation-entry closed exact computational, queue, graph, and
  IR verification on that same live topology

## Closeout Decision

This contract is now closed as deferred.

The blocked condition was:

- live Calc did not expose the required one-group full-span after-topology

Instead, the live three-group attempt kept the far participant group
separate.

## Retained Boundary

The contract remains validation-only or deferred for:

- any three-participant collapse whose live after-state is not one full-span
  shareable group
- four-plus-group collapse
- named-range-combined collapse
- repair-sensitive normalization
- off-sheet collapse

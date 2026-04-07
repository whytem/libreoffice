# Computational Substrate Shared-Group Non-Structural Frontier Mapping Rules

Status: frozen mapping rules for the broader non-structural shared-group frontier closeout

## Purpose

This note fixes the identity and equivalence rules used by the frontier
closeout.

## Same-Text Preserve Identity

The newly admitted preserve family uses these rules:

- the touched address must already belong to a shareable shared group before
  the mutation
- the touched address must still belong to that same shareable group after
  the mutation
- equality is group identity equality, not merely "still shared somewhere"
- the touched formula text must be identical before and after the mutation

For this family, exactness means:

- same anchor
- same length
- same shareable flag
- same member set

## Prediction Window

The admitted non-structural frontier prediction window is bounded to the
pre-existing touched group window.

That means:

- the engine rebuilds group bindings only inside the touched group's before
  extent
- adjacent ordinary formulas outside that window are not eligible to join
  the admitted preserve family
- the preserve family does not infer new cross-run membership

## Member-Exit Carry-Forward Rule

The earlier admitted member-exit mapping is unchanged:

- remove the touched member from the before-group
- repartition surviving members into contiguous vertical runs
- materialize only runs longer than one cell as shared groups
- leave lone survivors ordinary

## Deferred Frontier Shapes

The following shapes are explicitly outside the admitted mapping rules:

- formula insertion into a blank cell adjacent to a shared group
- formula replacement that keeps the touched cell shared but changes group
  identity
- multi-group collapse into one merged after-group
- named-range-sensitive shared-group closure
- off-sheet dependency-driven shared-group closure

These shapes are not normalized into the admitted preserve or member-exit
families.

## Validation Rule

Observed-after topology remains a verification check only.

It may be used to:

- confirm same-text preserve keeps the same group identity
- confirm member-exit does not reintroduce the touched cell into a group

It may not be used to:

- author regroup identity
- author merge identity
- author off-sheet or named-range-combined after-state topology

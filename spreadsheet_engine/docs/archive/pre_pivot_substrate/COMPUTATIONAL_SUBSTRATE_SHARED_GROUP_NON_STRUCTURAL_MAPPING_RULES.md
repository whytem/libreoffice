# Computational Substrate Shared-Group Non-Structural Mapping Rules

Status: frozen mapping rules for the non-structural shared-group admission
cycle

## Purpose

This note defines the identity and equivalence rules for the bounded
non-structural shared-group candidate slice frozen in
[COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_ADMISSION_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_ADMISSION_CONTRACT.md).

## Stable Before-State Identity

The before-state identity of the touched shared group is:

- the group anchor address
- the group length
- the ordered member-address set implied by that anchor and length
- the shareable flag

The before-state identity of the touched cell is:

- its absolute address
- whether it is the anchor, an interior member, or the tail member of the
  touched group

## Engine-Authored After-State Rule

For the admitted candidate slice, after-topology is predicted by one rule
family only:

1. identify the touched pre-existing shareable group
2. remove the touched cell from that group's member set
3. keep the surviving pre-existing members at the same addresses
4. repartition the surviving members into contiguous vertical runs
5. materialize only runs longer than one cell as shared groups
6. leave singleton survivors as ordinary non-grouped formula cells
7. represent the touched cell as scalar, empty, or ordinary non-grouped
   formula according to the mutation kind

That rule is the whole admitted topology authority. No host-observed
after-state group topology may be copied into the admitted result.

## Outcome Mapping

The rule family maps to observable outcomes as follows:

- `SetScalarValue`: touched cell becomes scalar, surviving members repartition
- `ClearCell`: touched cell disappears, surviving members repartition
- `SetFormula`: touched cell remains formula-bearing but becomes an ordinary
  non-grouped formula cell, surviving members repartition

Those outcomes may produce:

- no surviving shared groups
- one rebuilt shared group
- multiple surviving shared groups if the touched cell splits a longer group

## Exactness Rules

An outcome is exact only if:

- the predicted member population matches the observed after-state
- the predicted shared-group descriptors match the observed after-state
- the touched cell payload kind matches the mutation contract
- queue, computational, graph, and IR verification all close on the same
  after-state

## Normalization Policy

This cycle does not admit normalized-equivalent shared-group topology.

For the candidate slice:

- exact predicted topology is eligible for admission
- host-observed topology that differs from the predicted member-exit rule is
  out of contract for live admission in this cycle

## Forbidden Shortcuts

The following are forbidden:

- copying host-observed after-state shared-group anchors or lengths into the
  predicted shadow
- allowing same-text `SetFormula` preserve behavior to slip into the admitted
  slice through a member-exit rule
- inferring new shared-group creation from arbitrary replacement formula text
- collapsing merge, regroup, or repair-sensitive host behavior into the same
  admitted equivalence class as the bounded member-exit rule family

## Deferred Equivalence Questions

The following remain explicitly outside this mapping note:

- preserve-only same-text replacement semantics
- exact equivalence for regroup or merge across prior groups
- named-range-combined shared-group behavior
- repair-sensitive host-only normalization beyond rollback-capable detection

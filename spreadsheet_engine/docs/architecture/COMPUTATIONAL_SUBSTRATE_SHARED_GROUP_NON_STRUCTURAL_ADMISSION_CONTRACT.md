# Computational Substrate Shared-Group Non-Structural Admission Contract

Status: frozen candidate boundary for the non-structural shared-group
admission cycle

## Purpose

This note freezes the exact candidate slice evaluated by
[COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_ADMISSION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_ADMISSION_PLAN.md).

The goal is not broad shared-group lifecycle admission. The goal is one
bounded same-sheet shareable member-exit family on the already-admitted
non-structural mutation vocabulary.

## Carried-Forward Boundary

The cycle begins on top of the current settled rollout surface:

- admitted scalar mutation entry
- admitted scalar lifecycle authority
- admitted same-sheet structural `InsertRows`, `DeleteRows`, `InsertColumns`,
  and `DeleteColumns`
- admitted exact same-sheet shareable shared-group structural `Preserve`,
  `Split`, and `Rebuild`
- clean baseline only
- no named-range-sensitive structural behavior

This cycle does not reopen storage residency, listener container residency,
or broader workbook-class widening.

## Candidate Mutation Classes

The only mutation kinds under reassessment are:

- `SetScalarValue`
- `SetFormula`
- `ClearCell`

Those mutations are candidate-only when they touch a pre-existing
same-sheet shareable shared-formula group member.

## Candidate Workbook Shape

The candidate slice is intentionally narrow:

- the touched cell belongs to a pre-existing shared formula group
- the touched group is vertical, same-sheet, and shareable
- the touched mutation affects one member of that group only
- all formulas involved stay within ordinary or shared-group-member formula
  kinds
- the workbook has no named ranges
- the baseline is clean

The candidate does not require proving broad regroup or merge behavior across
multiple prior groups.

## Candidate Outcome Family

The only outcomes eligible for live admission in this cycle are exact
member-exit outcomes:

- the touched cell leaves the shared group
- surviving pre-existing group members are repartitioned into contiguous
  vertical runs
- runs longer than one cell stay shared
- singleton survivors become ordinary non-grouped formula cells
- the touched cell becomes one of:
  - scalar
  - empty
  - ordinary non-grouped formula

This exact rule family covers:

- non-structural `Split`
- non-structural `Rebuild`
- collapse from one prior shared group to no surviving shared groups

## Admitted Candidate Subset

The first admissible subset is:

- `SetScalarValue` on a shared-group anchor, interior member, or tail member
- `ClearCell` on a shared-group anchor, interior member, or tail member
- `SetFormula` on a shared-group anchor, interior member, or tail member only
  when the replacement formula is treated as a non-grouped formula cell

The candidate is exact only when after-state topology can be predicted from
the before-group membership plus the mutation without copying host-observed
group topology.

## Explicitly Excluded Classes

The following remain out of scope for this cycle:

- named-range-combined shared-group behavior
- off-sheet shared-group behavior
- non-shareable shared-group behavior
- matrix formulas
- broad regroup or merge across prior groups
- host-only repair-sensitive shared-group normalization
- sheet insert, delete, rename, or move
- copy, move, clipboard, import, load-time, or undo-like flows

## Verification Standard

Promotion is justified only if the candidate slice satisfies the same
standard as the admitted structural family:

- exact engine-authored after-topology for the shared-group portion of the
  after-state
- exact queue verification
- exact computational verification
- exact graph verification
- exact IR verification
- exact object realization on the admitted live path
- explicit rollback or repair-detected behavior for perturbed or out-of-scope
  cases

## Forbidden Shortcuts

The following are explicitly forbidden on the admitted candidate slice:

- copying host-observed after-state shared-group topology
- inferring admission from host regroup or repair heuristics that are not
  representable in the engine shadow
- silently widening into preserve-only replacement behavior, merge behavior,
  or named-range-combined behavior

## Success Condition

This cycle succeeds only if one bounded non-structural shared-group
member-exit family becomes a live admitted rollout slice with the same exact
verification standard already required on the admitted authority surface.

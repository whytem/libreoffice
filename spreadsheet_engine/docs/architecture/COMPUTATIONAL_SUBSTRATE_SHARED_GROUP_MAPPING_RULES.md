# Computational Substrate Shared-Group Mapping Rules

Status: frozen mapping and equivalence rules

## Purpose

This note defines the stable identity and equivalence rules for the bounded
shared-group widening cycle.

The goal is to prevent the shared-group pilot from "passing" through hidden
Calc-local shortcuts that reclaim authority while still allowing explicit
classification of preserve, split, rebuild, and repair outcomes.

## Stable Shared-Group Identity

Within this cycle, shared-group identity is carried by the tuple:

- group anchor sheet
- group anchor column
- group anchor row
- group length

This is the same identity already reflected in:

- workbook-facade `FormulaGroupDescriptor`
- computational-shadow `ShadowFormulaGroupId`
- dependency-graph formula-group listener anchors

No other implicit host identity is part of the widening contract.

## Membership Rules

For a shared group to count as the same logical group after mutation:

- the anchor must remain on the same sheet
- the member population must cover the same contiguous vertical run implied
  by the anchor and length
- every member must still be observable as a shared-group member through the
  workbook facade

If any of those stop being true, the outcome is not "preserved." It is
either a split, rebuild, repair-detected result, or a deferred case.

## Shareable Versus Non-Shareable Classification

This cycle treats the facade's `mbShareable` flag as the stable exposed
classification for whether a surviving group is shareable.

The widening pilot may observe:

- shareable preserved groups
- shareable rebuilt groups
- a loss of shareable status

A loss of shareable status is never silently normalized away. It must be
reported as either:

- a rebuild outcome that remains validation-only
- a repair-detected outcome
- or a deferred case

## Equivalence Rules

The widening cycle recognizes only the following equivalence families.

### Exact Preserve

An outcome is exact preserve only if:

- the shared-group identity tuple matches exactly after address shifting
- the member population matches exactly after address shifting
- the group remains observable through the same workbook-facade surfaces
- computational, graph, queue, realization, rollback, and verification
  remain exact

### Normalized Rebuild

A rebuild may count as normalized-equivalent only in validation or hybrid
analysis, never by silent admission, and only if:

- the mutation semantics clearly imply a rebuilt same-sheet group
- the rebuilt group is still observable as one contiguous shared group
- the rebuilt anchor and length are explicit and stable after normalization
- all non-group computational, graph, queue, realization, rollback, and
  verification comparisons still close

Normalized rebuild is not sufficient on its own for admission. It is an
explicit evidence category.

### Split

A split is not equivalent to preserve.

If a shared group splits into non-group cells or into multiple distinct
groups after the mutation, the outcome must be classified as:

- validation-only split
- repair-detected
- or deferred

### Host Repair

Any case where Calc performs host-local re-grouping, repair, or
re-canonicalization that the current engine seams do not express must be
classified as repair-detected or deferred. It cannot count as equivalent.

## Forbidden Host Shortcuts

The following shortcuts are forbidden for this cycle:

- treating host-observed post-mutation group topology as admitted preserve
  without recording that it was host-observed
- treating group disappearance as equivalent to preserve
- treating a changed anchor or changed length as equivalent to preserve
  without explicit rebuild classification
- dropping shareable-status changes from comparison
- letting hidden host regrouping re-enter the admitted slice by implication

## What The Validation-Only Pilot May Use

The validation-only pilot may still use host-observed shared-group state for
classification so long as it does so explicitly and only to answer the
bounded widening question.

That means the pilot may:

- observe before and after group descriptors from the workbook facade
- compare engine-predicted non-group state with host-observed group
  topology
- classify preserve, split, rebuild, repair, and deferred outcomes

But the pilot may not:

- relabel host-observed group topology as admitted engine-owned authority
- widen the live rollout without an explicit decision record

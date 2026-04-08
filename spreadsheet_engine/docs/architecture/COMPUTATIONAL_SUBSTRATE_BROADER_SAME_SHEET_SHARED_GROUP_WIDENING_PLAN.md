# Computational Substrate Broader Same-Sheet Shared-Group Widening Plan

Status: completed closeout for the broader same-sheet shared-group widening
frontier

## Purpose

This plan focuses only on the remaining same-sheet shared-group widening
frontier called out in
[COMPUTATIONAL_SUBSTRATE_MASTER.md](COMPUTATIONAL_SUBSTRATE_MASTER.md):

- broader named-range-combined regroup, merge, and collapse
- broader non-edge regroup and merge

The goal is to close this frontier with live-backed exact outcomes rather
than with more synthetic widen-first modeling.

## Current Boundary

The admitted same-sheet shared-group slice already covers:

- structural `Preserve`, `Split`, and `Rebuild`
- non-structural member-exit
- same-text preserve
- bounded edge-regroup
- bounded gap merge
- bounded replacement merge
- bounded one-sided insert
- bounded named-range-combined same-text preserve
- bounded named-range-combined member-exit

This pass closed the planned same-sheet frontier as follows:

- named-range-combined three-group-collapse attempts remain explicitly
  deferred because live Calc keeps the far group separate
- broader non-edge regroup attempts normalize onto the already-admitted
  member-exit path
- broader non-edge merge attempts normalize onto the already-admitted
  member-exit path
- corrected live facade proof also exposed bounded named-range-combined
  `Regroup` and bounded named-range-combined `OneSidedInsert` as real live
  families, and this pass admitted both on the bounded
  `GlobalSingleAreaSameSheet` surface

## Working Rule

This plan succeeds only if every candidate family closes as one of:

- `admitted_as_distinct_family`
- `normalized_to_existing_admitted_family`
- `explicitly_deferred_after_live_proof`

Synthetic standalone exactness is not enough for admission.

## Scope

This plan covers:

- same-sheet only
- shareable shared-formula groups only
- single-cell `SetFormula` mutation shapes only
- named-range-combined and non-named-range same-sheet widening attempts

This plan does not cover:

- off-sheet consumers
- repair-sensitive normalization
- structural widening
- broad listener or storage ownership changes

## Target Families

### 1. Named-Range-Combined Three-Group Collapse Attempt

Question:

- does live Calc actually collapse three named-range-combined prior groups
  into one after-group
- or does it normalize to a smaller already-admitted shape
- or does it stay deferred

### 2. Broader Non-Edge Regroup Attempt

Question:

- can an interior touched shared-group member produce a distinct regroup
  family under live same-sheet single-cell mutation
- or does the live host normalize it to member-exit or split behavior

### 3. Broader Non-Edge Merge Attempt

Question:

- can an interior touched shared-group member produce a distinct broader
  merge family under live same-sheet single-cell mutation
- or does the live host normalize it to member-exit, replacement-merge, or
  split behavior

## Deliverables

This plan is complete only when all of the following exist:

1. live facade proof for the three unresolved same-sheet families
2. exact lifecycle and mutation-entry proof for any family that normalizes
   onto an already-admitted shape
3. authority or gate widening only if a family closes exactly as a distinct
   live family
4. a closeout decision record stating which same-sheet families are still
   real widening targets and which are now resolved as normalization or
   explicit defer outcomes
5. updated master and project-status current-state summaries

## Execution Phases

### Phase 1: Freeze The Same-Sheet Matrix

- define the exact unresolved family list
- record the intended proof ladder
- keep the success rule explicit: live shape first, widening second

### Phase 2: Probe Live Facade Shape

Add live Calc facade proof for:

- named-range-combined three-group-collapse attempt
- non-edge regroup attempt
- non-edge merge attempt

Record whether each attempt is:

- a real distinct live family
- a normalization onto an existing admitted family
- or a retained defer outcome

### Phase 3: Carry Through The Authority Path

For any family that is a real distinct live family:

- widen the authority gate only if exact after-state authoring closes
- add standalone, lifecycle, and mutation-entry proof

For any family that normalizes onto an existing admitted family:

- add lifecycle and mutation-entry proof that the live path already closes
  on that existing admitted family
- do not widen the admitted slice dishonestly

### Phase 4: Close Out

- record the exact current-state outcome
- update the master and project-status docs
- state clearly whether broader same-sheet widening still contains any real
  unresolved same-sheet single-cell family after this pass

## Success Criteria

This plan counts as complete only if:

- the three unresolved same-sheet families are each closed with an explicit
  live-backed outcome
- any real widening is admitted only after exact proof
- any family that is not a real widening target is documented as such
- replay and diff hygiene remain clean

## Closeout

The plan is complete.

The exact outcomes are:

- named-range-combined three-group-collapse stays deferred
- broader non-edge regroup is resolved as normalization to member-exit
- broader non-edge merge is resolved as normalization to member-exit
- bounded named-range-combined `Regroup` is admitted
- bounded named-range-combined `OneSidedInsert` is admitted

That means the broader same-sheet single-cell widening frontier no longer
contains another immediate admitted-slice expansion target on the current
surface. The roadmap should now return to repair-sensitive normalization and
bounded off-sheet dependency closure, with the retained multi-group collapse
shape kept as an explicit deferred boundary rather than as the next
assumed widening pass.

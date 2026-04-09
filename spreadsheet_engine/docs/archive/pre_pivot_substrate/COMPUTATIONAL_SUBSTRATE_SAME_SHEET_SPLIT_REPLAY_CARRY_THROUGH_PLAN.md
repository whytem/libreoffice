# Computational Substrate Same-Sheet Split Replay Carry-Through Plan

Status: completed closeout for direct same-sheet split-backed named-range
replay

Phase status:

- Phase 1 completed: freeze the direct replay target and proof ladder
- Phase 2 completed: identify the exact direct authority and lifecycle replay
  mismatch
- Phase 3 completed: patch the smallest exact replay seam and rerun proof
- Phase 4 completed: close the pass as admission

## Purpose

The prior split-outcome pass closed the user-facing mutation-entry lane but
left one narrower blocker:

- direct authority replay of the exact split-backed named-range host shape
- direct lifecycle replay of that same host shape

This pass exists to clear that blocker directly.

## Exact Target

The target family is:

- same-sheet
- shareable
- named-range-combined
- `SetFormula`
- bounded `GlobalSingleAreaSameSheet`
- three adjacent participant groups before the mutation
- touched address inside the middle participant group
- live after-state with two groups:
  - one regrouped near-side group that includes the touched address
  - one far-side participant group that remains separate

The frozen host contract is:

- shared-formula mutation family: `Regroup`
- shared-formula transition kind: `Split`

## Desired Outcome

This pass succeeds only if the exact live split-backed host shape becomes
directly applicable on:

- authority replay
- lifecycle replay

Mutation-entry already closes exactly and is not the blocker.

## Primary Engineering Surfaces

- `AuthorityPilotBuilder.hxx`
- `LifecyclePilotBuilder.hxx`
- `ComputationalShadowComparison.hxx`
- `DependencyGraphShadowComparison.hxx`
- `ucalc_dependency_shadow.cxx`
- `computational_substrate_tests.cxx`

## Working Hypothesis

The current blocker is not predicted topology. Standalone exactness already
closes there.

The likely blocker is direct replay observation shaping on the named-range
surface, specifically the broadcaster/listener form used for the split-backed
after-state when more than one after-group survives.

## Execution Plan

### 1. Freeze The Target

- keep the exact split-backed host contract explicit
- keep mutation-entry out of scope except as a comparison baseline

### 2. Identify The Direct Replay Mismatch

- rerun the direct authority and lifecycle split-outcome proof buckets
- identify whether the mismatch is:
  - computational broadcaster shape only
  - graph listener-anchor or edge shape
  - some wider dependency-snapshot or queue mismatch

### 3. Patch The Smallest Exact Seam

- update the direct replay builder only on the bounded
  `GlobalSingleAreaSameSheet` split-backed named-range surface
- do not widen broader same-sheet or off-sheet families speculatively
- keep the change exact enough that standalone, authority, lifecycle, and
  mutation-entry proof all agree on the same host shape

### 4. Close Out

- if authority and lifecycle both verify exactly, admit the direct replay
  lane and update current-state docs
- if either one still fails, record the narrower retained mismatch and keep
  the boundary deferred explicitly

## Exit Criteria

This pass is complete when one of the following is true:

- direct authority replay and direct lifecycle replay both apply exactly for
  the bounded same-sheet split-backed named-range host shape
- or the pass closes with an explicit retained-defer result that identifies
  the remaining exact mismatch after the targeted replay-shaping change

## Closeout

The pass is complete.

The direct replay blocker is cleared.

The exact bounded same-sheet named-range split-backed host shape now closes
on:

- authority replay
- lifecycle replay
- mutation-entry
- standalone exactness

The runtime fix was a bounded named-range replay-shaping update on the
split-backed same-sheet `Regroup` surface:

- the touched surviving after-group now replays as top-cell plus
  touched-cell area listeners
- the far surviving after-group now replays as the existing group listener
  plus top-cell augmentation

That means the same-sheet split-backed host shape is now a real admitted
slice on the bounded `GlobalSingleAreaSameSheet` named-range surface.

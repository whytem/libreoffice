# Computational Substrate Same-Sheet Named-Range Split-Outcome Plan

Status: completed closeout for the retained same-sheet named-range
split-outcome pass

Phase status:

- Phase 1 completed: freeze the exact live host shape and narrow target
- Phase 2 completed: prove standalone exact carry-through for that host shape
- Phase 3 completed: prove live authority, lifecycle, and mutation-entry
  carry-through and record the exact live split outcome
- Phase 4 completed: record the hybrid widening-and-retained-defer outcome
  in the master status surface

## Purpose

The remaining same-sheet widening blocker is no longer the older synthetic
three-group-collapse model.

Live Calc now shows a narrower host shape for the retained same-sheet
named-range-combined three-group attempt:

- the mutation stays on the bounded `GlobalSingleAreaSameSheet` surface
- the touched middle participant changes shared-group identity
- the after-topology is a `Split`, not a full-span one-group collapse
- the far participant group remains separate

This pass exists to clear that blocker by targeting the exact live host
shape instead of the superseded synthetic one-group collapse model.

## Narrow Target

The target admitted family for this pass is:

- same-sheet
- shareable
- named-range-combined
- `SetFormula`
- bounded `GlobalSingleAreaSameSheet`
- before-state with three adjacent participant groups on the touched column
- touched address inside the middle participant group
- live after-state with two groups:
  - one regrouped near-side after-group that includes the touched address
  - one far-side participant group that remains separate

The expected live semantic surface is:

- shared-formula transition kind: `Split`
- shared-formula mutation family: `Regroup`
- named-range boundary: `GlobalSingleAreaSameSheet`

## Why This Is The Right Next Pass

This is the biggest remaining same-sheet widening opportunity because it:

- clears the top retained same-sheet blocker in the master roadmap
- reuses the already-admitted named-range `Regroup` lane if the exact live
  split-backed shape already closes there
- avoids widening into off-sheet, repair-sensitive, or synthetic host-shape
  assumptions

## Primary Engineering Surfaces

- `FacadeConsumers.hxx`
- `AuthorityPilotBuilder.hxx`
- `LifecyclePilotBuilder.hxx`
- `computational_substrate_tests.cxx`
- `workbook_facade_tests.cxx`
- `ucalc_workbook_facade.cxx`
- `ucalc_dependency_shadow.cxx`

## Execution Plan

### 1. Freeze The Live Host Contract

- add or tighten Calc-facade proof for the named-range three-group attempt
- make the split-backed far-group-separate host shape explicit
- treat the old synthetic one-group collapse model as historical only

### 2. Prove Standalone Exactness

- add a standalone computational-substrate proof bucket for the exact
  split-backed after-topology
- confirm that predicted after-state, graph, and IR close on the two-group
  live host shape

### 3. Prove Live Carry-Through

- add authority proof for the bounded named-range split-backed lane
- add lifecycle proof for the same lane
- add mutation-entry proof for the same lane
- if any proof fails, patch the runtime on the smallest exact seam needed
  to match the live host shape

### 4. Close Out The Blocker

- if the lane closes exactly, promote it into the admitted slice
- if it does not, record the precise retained mismatch and keep the family
  deferred by explicit rule
- update the master document, project status, and architecture README to
  reflect the final result

## Exit Criteria

This pass is complete when one of the following is true:

- the exact same-sheet named-range split-backed three-group attempt is
  admitted live on the bounded `GlobalSingleAreaSameSheet` surface, with
  standalone and live proof
- or the pass closes with an explicit retained-defer result backed by exact
  proof of the live mismatch that still prevents admission

## Closeout

The pass is complete.

The old synthetic one-group-collapse model is now closed as historical only.
The exact live host shape is:

- named-range boundary `GlobalSingleAreaSameSheet`
- shared-formula mutation family `Regroup`
- shared-formula transition kind `Split`
- two after-groups, with the far participant group remaining separate

The final outcome is hybrid:

- standalone exactness for that live split-backed after-topology closes
- live mutation-entry closes exactly and normalizes onto the already-admitted
  bounded named-range `Regroup` lane
- direct authority and lifecycle replay of that split-backed host shape still
  roll back on exact verification and remain deferred

That means this pass did widen the practical admitted slice on the user-facing
mutation-entry surface, but it did not admit a new distinct same-sheet
multi-group-collapse family and it did not clear the direct authority or
lifecycle split-backed replay boundary.

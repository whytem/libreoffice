# Computational Substrate Shared-Group Non-Structural Named-Range-Combined Implementation

Status: bounded named-range-combined closeout implemented without live admission

## What Landed

This cycle added bounded named-range-combined groundwork, but it did not
widen the admitted non-structural shared-group slice.

The landed narrowing is:

- explicit facade-side named-range-combined boundary classification
- preserve-only shared-group named-range candidate gating
- retained member-exit reject handling on that same bounded surface

## Runtime Changes

The main runtime work landed in:

- [FacadeConsumers.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/workbook/FacadeConsumers.hxx)
- [AuthorityPilotBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/AuthorityPilotBuilder.hxx)

The closeout behavior is:

- the facade consumer layer now classifies named-range-combined shared-group
  mutations as `None`, bounded `GlobalSingleAreaSameSheet`, or `Deferred`
- that classification is derived from stable descriptor lookup and relevant
  same-sheet consumer discovery around the touched shared-group neighborhood
- the authority builder now evaluates both shared-group mutation family and
  named-range boundary together
- the authority candidate gate now treats only bounded
  named-range-combined `SameTextPreserve` as in-bounds for prediction
- the same named-range boundary still rejects member-exit and broader
  families with `shared_group_named_range_out_of_contract`

## Carry-Through Shape

The preserve candidate uses the same engine-authored after-state shape as
the earlier same-text-preserve slice:

- keep the exact shared-group after-topology unchanged
- build dependency, recalc, graph, and IR state from the predicted
  after-shadow
- compare against the observed after-state for exact verification,
  including named-range match

That closes in standalone proof, but the live carry-through still stops
short of admission.

## What Did Not Land

This implementation intentionally still does not admit:

- named-range-combined member-exit
- named-range-combined regroup
- named-range-combined merge
- named-range-combined multi-group collapse
- sheet-local, multi-area, or ambiguous named ranges
- off-sheet consumers
- repair-sensitive normalization

The concrete live blockers proven originally in this cycle were:

- lifecycle still rejects the bounded preserve family out of contract
- mutation entry reaches live-apply/final-verification surfaces that still
  report `listener_anchor_out_of_contract`

A later listener-anchor support closeout removed that specific blocker.

The current concrete live blockers are now:

- lifecycle `opaque_dependency_surface`
- mutation-entry `rollback_queue_or_state_mismatch`

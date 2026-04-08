# Computational Substrate Broader Off-Sheet Formula-Retained Carry-Through Plan

Status: active execution plan for the next off-sheet widening pass

## Objective

Resolve the next off-sheet blocker by widening the admitted slice from
bounded direct off-sheet shared-group `MemberExit` to the smallest exact
formula-retained families that stay on the same authority surface.

This pass is intentionally scoped to:

- same-workbook direct off-sheet shared-group consumers
- exactly one consumer sheet outside the source sheet
- exact queue, computational, graph, and IR verification
- exact authority, lifecycle, and mutation-entry carry-through

This pass is not about workbook-wide off-sheet authority.

## Target Admission Slice

Promote the bounded direct one-consumer-sheet off-sheet shared-group
formula-retained families that are adjacent to the already-admitted
member-exit lane:

- `SameTextPreserve` `SetFormula`
- `Regroup` `SetFormula`

The goal is to close those families without widening into broader merge or
named-range-combined off-sheet ownership.

## Explicitly Out Of Scope

This pass does not attempt to admit:

- off-sheet named-range-combined families
- off-sheet merge, replacement-merge, or multi-group collapse
- multi-consumer-sheet off-sheet behavior
- workbook-wide off-sheet authority
- structural off-sheet named-range rollout

If the live proof shows that the bounded target families collapse onto a
smaller admitted shape or remain deferred, the pass should close with that
explicit result instead of stretching scope.

## Why This Next

The last off-sheet pass removed the raw-reference opacity blocker and proved
the bounded direct off-sheet `MemberExit` lane.

What remains is the adjacent formula-retained surface where:

- the touched shared-group formula stays live after mutation, and
- off-sheet direct consumers must still carry through exact queue,
  broadcaster, rollback, and verification state.

That makes bounded direct off-sheet formula-retained carry-through the
highest-value next pass before any off-sheet named-range work.

## Phase 1: Freeze The Bounded Contract

Define the bounded surface precisely in the docs and keep the target lane
small enough to prove exactly.

Deliverables:

- this plan
- master/status/readme links that make this the active off-sheet roadmap item

Exit criteria:

- the target families are frozen as direct one-consumer-sheet off-sheet
  `SameTextPreserve` and `Regroup`
- off-sheet named-range-combined families remain explicitly deferred

## Phase 2: Prove The Host Shape And Standalone Exactness

Add the proof buckets needed to show whether the bounded families are real
live candidates.

Deliverables:

- facade classification proof for direct off-sheet `SameTextPreserve` and
  `Regroup`
- standalone exactness proof for authority planning on those families
- retained reject or defer proof for any broader off-sheet shape that still
  falls outside this pass

Exit criteria:

- the facade exposes the target families distinctly enough to reason about
- standalone authority planning is either exact or fails with a narrower
  explicit blocker

## Phase 3: Carry The Families Through Live Authority

Close the exact authority/lifecycle/mutation-entry path for the bounded
off-sheet formula-retained families.

Deliverables:

- runtime changes only if the new proof exposes a real carry-through gap
- live authority proof
- live lifecycle proof
- live mutation-entry proof

Exit criteria:

- the bounded families are either admitted through live carry-through or
  rejected with a narrower explicit blocker than the current generic
  off-sheet frontier

## Phase 4: Close Out The Pass

Record the final boundary and the next remaining off-sheet blocker in the
consolidated docs.

Deliverables:

- updated master/status/readme
- pass closeout decision summary in this plan or a companion decision record

Exit criteria:

- the admitted slice and the retained off-sheet frontier are described in
  one place
- the next logical blocker is explicit

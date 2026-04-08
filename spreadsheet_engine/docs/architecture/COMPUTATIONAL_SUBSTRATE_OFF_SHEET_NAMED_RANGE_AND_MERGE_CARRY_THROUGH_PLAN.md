# Computational Substrate Off-Sheet Named-Range And Merge Carry-Through Plan

Status: completed closeout for the bounded off-sheet named-range and merge pass

## Objective

Resolve the next off-sheet blocker by promoting the smallest exact families
that remain after direct off-sheet `MemberExit`, `SameTextPreserve`, and
`Regroup` were admitted.

This pass has two adjacent targets:

- bounded off-sheet named-range-combined carry-through
- bounded direct off-sheet merge-shaped carry-through

The work stays explicitly fenced to:

- same workbook
- one mutation sheet
- at most one additional consumer sheet
- exact queue, computational, graph, and IR verification
- exact authority, lifecycle, and mutation-entry carry-through

## Primary Target Families

The first target is bounded off-sheet named-range-combined shared-group
behavior on a global single-area named range with one additional consumer
sheet:

- `SameTextPreserve` `SetFormula`
- `Regroup` `SetFormula`
- `OneSidedInsert` `SetFormula`

The second target is bounded direct off-sheet merge-shaped shared-group
behavior without named-range-combined widening:

- `OneSidedInsert` `SetFormula`
- `Merge` `SetFormula`
- `ReplacementMerge` `SetFormula`

## Explicitly Out Of Scope

This pass does not attempt to admit:

- multi-consumer-sheet off-sheet behavior
- workbook-wide off-sheet authority
- off-sheet named-range-combined multi-group collapse
- off-sheet named-range-combined descriptor drift
- off-sheet structural named-range rollout
- off-sheet repair-sensitive normalization

If some candidates do not close exactly, the pass should close them with an
explicit retained blocker rather than widening scope.

## Why This Next

The direct off-sheet formula-retained pass already proved that bounded
cross-sheet dependency closure works for a one-consumer-sheet direct lane.

What remains is now narrower and more specific:

- named-range-combined off-sheet families are still collapsed into a generic
  deferred boundary instead of a bounded candidate lane
- direct off-sheet merge-shaped families still lack explicit proof even
  though they sit on the same already-admitted dependency surface

That makes this the highest-value next pass before any broader workbook-wide
off-sheet reconsideration.

## Phase 1: Freeze The Contract

Deliverables:

- this plan
- master/status/readme links
- explicit bounded target families

Success criteria:

- the plan distinguishes off-sheet named-range-combined from direct
  off-sheet merge-shaped work
- multi-consumer-sheet and workbook-wide behavior remain out of scope

## Phase 2: Extend The Facade Boundary

Work:

- extend the shared-group named-range boundary classifier so bounded
  off-sheet named-range-combined candidates are distinct from broad deferred
  off-sheet cases
- keep exact same-sheet boundary behavior unchanged
- keep multi-consumer-sheet or descriptor-unstable cases deferred

Success criteria:

- facade proof can distinguish bounded off-sheet named-range-combined
  candidates
- retained broad off-sheet named-range cases still classify as deferred

## Phase 3: Carry The Bounded Families Through Authority

Work:

- admit the new bounded off-sheet named-range boundary in the authority gate
- reuse existing named-range observation-build adjustments when they remain
  exact on the off-sheet surface
- prove direct off-sheet merge-shaped families and only add runtime changes
  if the exact carry-through fails

Success criteria:

- bounded off-sheet named-range-combined families either apply exactly or
  fail with a narrower explicit blocker
- bounded direct off-sheet merge-shaped families either apply exactly or
  fail with a narrower explicit blocker

## Phase 4: Close Out The Pass

Deliverables:

- updated consolidated docs
- final decision summary
- explicit next blocker if any retained family remains

Success criteria:

- the admitted slice and retained off-sheet boundary are described in one
  place
- the next off-sheet blocker is narrower than the current combined blocker

## Closeout Result

This pass closed as a material but bounded admitted-slice widening.

Admitted:

- off-sheet named-range-combined `SameTextPreserve`
- off-sheet named-range-combined `Regroup`
- off-sheet named-range-combined `OneSidedInsert`

Retained deferred:

- off-sheet named-range-combined `MemberExit`
- direct off-sheet gap merge
- direct off-sheet replacement-merge
- off-sheet multi-group collapse
- multi-consumer-sheet and workbook-wide off-sheet authority

The companion closeout is in
[COMPUTATIONAL_SUBSTRATE_OFF_SHEET_NAMED_RANGE_AND_MERGE_CARRY_THROUGH_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_OFF_SHEET_NAMED_RANGE_AND_MERGE_CARRY_THROUGH_DECISION_RECORD.md).

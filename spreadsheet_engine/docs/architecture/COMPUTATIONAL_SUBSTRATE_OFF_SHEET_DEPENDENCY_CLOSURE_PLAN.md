# Computational Substrate Off-Sheet Dependency Closure Plan

Status: completed closeout for the bounded off-sheet widening pass

## Outcome

The pass closed with a real admitted-slice expansion, but on a narrower
family than the initial candidate list suggested.

Admitted:

- same-workbook one-consumer-sheet direct off-sheet shared-group
  non-structural `MemberExit`
- `SetScalarValue`, `SetFormula`, and `ClearCell`

Deferred:

- off-sheet named-range-combined families as a distinct live boundary
- structural off-sheet named-range rollout
- broader off-sheet regroup, merge, and multi-consumer-sheet behavior

The closeout decision is recorded in
[COMPUTATIONAL_SUBSTRATE_OFF_SHEET_DEPENDENCY_CLOSURE_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_OFF_SHEET_DEPENDENCY_CLOSURE_DECISION_RECORD.md).
Supporting proof is in
[COMPUTATIONAL_SUBSTRATE_OFF_SHEET_DEPENDENCY_CLOSURE_EVIDENCE.md](COMPUTATIONAL_SUBSTRATE_OFF_SHEET_DEPENDENCY_CLOSURE_EVIDENCE.md).

## Goal

Resolve the off-sheet dependency closure blocker by promoting the smallest
real cross-sheet shared-group families that the engine can carry exactly
through dependency snapshot, recalc queue, broadcaster projection, live
apply, rollback, graph verification, and IR verification.

This pass is explicitly about bounded off-sheet authority, not workbook-wide
authority transfer.

## Bounded Target Surface

The primary target surface is:

- same workbook
- one mutation sheet plus one consumer sheet
- clean baseline only
- exact shared-group non-structural families only
- no repair-sensitive normalization
- no broad structural off-sheet rollout
- no workbook-wide or multi-consumer-sheet promotion

The first candidate families are:

- off-sheet direct-consumer shared-group member-exit
- off-sheet named-range-combined `SameTextPreserve`
- off-sheet named-range-combined `MemberExit`

Follow-on families are in scope only if they close on the same exact model:

- off-sheet named-range-combined `Regroup`
- off-sheet named-range-combined `OneSidedInsert`

The following remain out of scope unless they close without widening the
surface:

- structural off-sheet named-range rollout
- off-sheet multi-group collapse
- off-sheet repair-sensitive normalization
- multiple consumer sheets
- off-sheet merge families beyond the bounded one-sheet carry-through

## Why This Is The Right Next Pass

The runtime already builds workbook-wide dependency snapshots, recalc plans,
and observation-state broadcaster projections. The main remaining blocker is
that the current admission boundary still treats off-sheet shared-group
consumers as deferred or rejected even where the underlying machinery may
already be exact.

That makes off-sheet dependency closure the highest-value next widening pass:

- it directly attacks the top blocker in the master roadmap
- it stays adjacent to the already-admitted same-sheet shared-group lane
- it can expand the admitted slice without opening a workbook-wide surface

## Phase 1: Freeze The Bounded Off-Sheet Contract

Deliverables:

- this plan
- master/status/readme links
- explicit bounded target definition

Success criteria:

- the pass is framed around one mutation sheet plus one consumer sheet
- structural off-sheet remains explicitly out of scope
- the first execution target is concrete enough to prove or reject

## Phase 2: Extend Bounded Off-Sheet Classification

Work:

- extend the shared-group named-range boundary classifier so bounded
  off-sheet consumers are distinguishable from broad deferred off-sheet
  cases
- preserve the current exact same-sheet boundary as-is
- keep multi-consumer-sheet or descriptor-unstable cases deferred

Success criteria:

- the facade can identify bounded off-sheet candidate families
- retained broad off-sheet cases still classify as deferred

## Phase 3: Carry The Bounded Off-Sheet Families Through Authority

Work:

- admit the bounded off-sheet families in the authority gate
- reuse the existing generic cross-sheet dependency snapshot, queue, and
  broadcaster machinery wherever it is already exact
- add any bounded observation-build adjustments required for off-sheet
  named-range-combined families

Success criteria:

- applicable authority plans are built for the bounded target families
- predicted computational, graph, and IR after-state comparisons remain
  exact

## Phase 4: Prove Live Lifecycle And Mutation Entry

Work:

- add standalone proof for the bounded off-sheet families
- add live Calc lifecycle and mutation-entry proof
- flip existing reject buckets when they become real admitted families
- retain explicit reject proof for broader off-sheet shapes that remain out
  of scope

Success criteria:

- live proofs apply on the bounded target surface
- rollback and verification stay exact
- broader off-sheet cases remain explicitly deferred or rejected

## Phase 5: Close Out The Blocker

Work:

- record what is newly admitted
- record what remains deferred
- update the master status/roadmap to reflect the new boundary

Success criteria:

- the admitted slice grows by at least one bounded off-sheet family, or the
  pass closes with a narrower explicit blocker than “off-sheet dependency
  closure”
- replay baseline remains exact

## Exit Conditions

This pass is complete when all of the following are true:

- the bounded off-sheet surface has been either admitted or explicitly
  reduced to narrower retained blockers
- proof exists in standalone and live tests
- roadmap docs describe the current off-sheet boundary in one place

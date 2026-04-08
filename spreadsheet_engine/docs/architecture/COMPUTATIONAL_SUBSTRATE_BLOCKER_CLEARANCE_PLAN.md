# Computational Substrate Blocker Clearance Plan

Status: ambitious program plan for clearing the four biggest remaining
computational-substrate blockers

Phase status:

- Phase 0 complete: boundary, matrix, and mapping rules frozen
- Phase 1 complete: broader same-sheet named-range-combined attempt families
  were live-proven not to close as distinct regroup or merge admissions
- Phase 2 pending
- Phase 3 pending
- Phase 4 pending
- Phase 5 pending

## Purpose

This plan turns the blocker summary in
[COMPUTATIONAL_SUBSTRATE_MASTER.md](COMPUTATIONAL_SUBSTRATE_MASTER.md)
into one aggressive, program-level execution plan.

The goal is not another narrow one-family widening pass. The goal is to
clear the four biggest blockers that still cap admitted-slice expansion:

1. exact after-state authoring for broader shared-group families
2. repair-sensitive host normalization
3. off-sheet dependency closure
4. retained host shell boundaries

## Desired End State

At closeout, the program should be able to say all of the following:

- broader same-sheet shared-group regroup, merge, and collapse families are
  either admitted exactly or rejected by explicit engine-owned rules instead
  of opaque host behavior
- repair-sensitive cases no longer depend on hidden Calc cleanup
- at least one bounded off-sheet shared-group family is admitted with exact
  dependency, queue, broadcaster, rollback, and final-verification closure
- the retained host shell is reduced to an explicit execution shell around
  engine-authored records on the admitted slice rather than an opaque source
  of truth

## Scope

This plan covers:

- same-sheet named-range-combined regroup, merge, and collapse
- broader same-sheet non-edge regroup and merge
- repair-sensitive shared-group normalization families
- bounded off-sheet shared-group consumers
- host-shell recut work required to keep the above exact end to end

This plan does not attempt:

- broad default-on rollout
- workbook-wide authority transfer
- wholesale `ScDocument` ownership migration
- UI, UNO, import/export, or persistence migration

## Program Strategy

The four blockers are coupled. They should be attacked as one staged program
with shared infrastructure rather than four isolated doc-and-patch cycles.

The strategy is:

1. build one exact after-state authoring substrate that can handle broader
   same-sheet and off-sheet families
2. make host normalization explicit so repair-sensitive cases stop depending
   on hidden Calc cleanup
3. extend dependency closure across a tightly bounded off-sheet surface
4. recut the host shell so the engine-authored records remain authoritative
   all the way through realization, rollback, and final verification

## Primary Engineering Surfaces

The core implementation seams for this program are expected to be:

- `AuthorityPilotBuilder.hxx`
- `StructuralPilotBuilder.hxx`
- `LifecyclePilotBuilder.hxx`
- `DependencySnapshot.hxx`
- `ComputationalSubstrateAuthority.hxx`
- `ComputationalSubstrateMutationEntry.hxx`
- `ComputationalSubstrateLiveApply.hxx`
- `ComputationalSubstrateWiring.hxx`
- `ComputationalShadowComparison.hxx`
- `computational_substrate_tests.cxx`
- `ucalc_workbook_facade.cxx`
- `ucalc_dependency_shadow.cxx`

## Workstream 1: Exact After-State Authoring

### Objective

Make the engine author the exact live after-state for broader shared-group
families instead of relying on bounded special cases.

### Target Families

- same-sheet named-range-combined regroup
- same-sheet named-range-combined merge and collapse
- same-sheet non-edge regroup
- same-sheet non-edge merge and collapse
- named-range-sensitive structural cases that can close on the same exact
  authored model

### Required Changes

- generalize shared-group participant discovery away from current narrow
  edge-only or bounded-gap assumptions
- make authored after-topology first class across
  `AuthorityPilotBuilder.hxx`, `StructuralPilotBuilder.hxx`, and
  `LifecyclePilotBuilder.hxx`
- unify the predicted-after graph, queue, and IR surfaces so they are all
  derived from the same engine-authored after-shadow
- eliminate remaining cases where the host-observed after-state still acts
  as the real topology oracle

### Exit Criteria

- broader same-sheet regroup and merge families are admitted or rejected by
  explicit authored rules
- retained rejects are live-proven rejects, not planner unknowns
- the admitted slice expands materially on the same-sheet shared-group
  surface

## Workstream 2: Repair-Sensitive Normalization

### Objective

Replace hidden Calc cleanup behavior with explicit engine-owned
normalization rules or narrow exact fences.

### Required Changes

- inventory every repair-sensitive divergence class currently hidden behind
  reject, rollback, or validation-only outcomes
- classify each family as `exact-normalizable`,
  `bounded-reject-by-rule`, or `retained-host-only`
- add normalization stages to the engine-authored after-state pipeline
  rather than patching late verification
- make regroup, listener, broadcaster, and queue normalization explicit and
  testable

### Exit Criteria

- every repair-sensitive family on the current frontier is either:
  - admitted with exact engine-owned normalization
  - rejected by an explicit documented rule
  - retained as a deliberate host-only boundary
- no admitted family depends on hidden host cleanup to pass

## Workstream 3: Off-Sheet Dependency Closure

### Objective

Admit a bounded cross-sheet shared-group surface without accidentally
promoting workbook-wide authority.

### Target Slice

Start with the narrowest useful family:

- same-workbook
- single external consumer sheet
- bounded named-range-free and repair-free shared-group consumers first
- then bounded named-range-combined off-sheet consumers if the same exact
  dependency model closes

### Required Changes

- extend dependency snapshots and broadcaster/listener modeling so off-sheet
  edges are first-class in the same canonical graph surface
- ensure queue construction, invalidation, and rollback are stable across
  cross-sheet dependency closure
- teach final verification to compare bounded off-sheet consequences without
  broadening into workbook-wide opaque validation
- keep admission fenced by sheet count, dependency shape, and consumer class

### Exit Criteria

- at least one bounded off-sheet family is admitted live
- rollback and final verification remain exact on that family
- broader off-sheet surfaces remain explicitly fenced rather than being
  silently widened

## Workstream 4: Retained Host Shell Boundary Reduction

### Objective

Turn the current retained host shell from a broad blocker into an explicit,
small, stable shell around engine-authored computational records.

### Required Changes

- formalize the host-shell contract for realization, rollback, queue
  execution, and final verification
- remove any remaining places where Calc-host state is the de facto source
  of truth on the admitted slice
- make live apply, mutation entry, final verification, and rollback operate
  on one canonical engine-authored record set
- isolate the truly retained host responsibilities as narrow document
  services instead of mixed computational ownership

### Exit Criteria

- the retained host shell is explicit, stable, and small on the admitted
  slice
- broader admitted-slice widening no longer stalls on opaque host-shell
  mismatches
- the program can reassess broader ownership expansion from a cleaner
  boundary than today

## Shared Enablers

These foundations should be built once and reused across all four
workstreams:

- one canonical predicted-after shadow surface
- one canonical graph and listener-anchor identity model
- one observation-build option system shared by authority, lifecycle, live
  apply, rollback, and final verification
- one blocker matrix that records admit, reject, rollback, or retained-host
  outcomes per family
- one proof ladder covering standalone synthetic proof, Calc facade proof,
  narrow-rollout lifecycle proof, mutation-entry proof, and replay-baseline
  proof

## Execution Order

### Phase 0: Freeze The Program Boundary

- freeze the four-blocker contract, scenario matrix, and mapping rules
- create one blocker matrix covering same-sheet, repair-sensitive, off-sheet,
  and host-shell seams
- identify the exact live proof buckets that currently fail for each blocker

### Phase 1: Build The Shared Authoring Substrate

- unify after-state authoring across structural and non-structural shared
  group paths
- move predicted-after graph, queue, and IR generation onto the same
  authored-after surface
- close the next wave of same-sheet regroup, merge, and collapse families

Phase 1 closeout:
[COMPUTATIONAL_SUBSTRATE_BLOCKER_PHASE1_SHARED_AUTHORING_CLOSEOUT.md](COMPUTATIONAL_SUBSTRATE_BLOCKER_PHASE1_SHARED_AUTHORING_CLOSEOUT.md)

### Phase 2: Make Normalization Explicit

- land repair-sensitive normalization taxonomy and engine-owned cleanup rules
- convert current hidden-host-cleanup cases into explicit normalize-or-reject
  outcomes
- re-run same-sheet frontier families that were previously blocked by repair
  sensitivity

### Phase 3: Extend To Bounded Off-Sheet Closure

- add bounded off-sheet dependency and broadcaster closure
- admit one bounded off-sheet family
- retain explicit fences against workbook-wide spillover

### Phase 4: Recut The Host Shell

- formalize engine-authored live records as the canonical source for apply,
  rollback, and verification on the admitted slice
- contract the retained host shell to narrow document-service duties
- re-evaluate whether any currently deferred surface is blocked only by the
  old shell boundary

### Phase 5: Close Out And Reassess

- update the master document and project status with the new boundary
- record what is newly admitted, what is explicitly retained, and what still
  remains unproven
- decide whether a follow-on broad-ownership program is justified

## Deliverables

The program should produce:

- one blocker contract document:
  [COMPUTATIONAL_SUBSTRATE_BLOCKER_CONTRACT.md](COMPUTATIONAL_SUBSTRATE_BLOCKER_CONTRACT.md)
- one blocker scenario matrix:
  [COMPUTATIONAL_SUBSTRATE_BLOCKER_SCENARIO_MATRIX.md](COMPUTATIONAL_SUBSTRATE_BLOCKER_SCENARIO_MATRIX.md)
- one blocker mapping-rules reference:
  [COMPUTATIONAL_SUBSTRATE_BLOCKER_MAPPING_RULES.md](COMPUTATIONAL_SUBSTRATE_BLOCKER_MAPPING_RULES.md)
- per-workstream implementation and evidence notes only where genuinely
  needed
- one final blocker-clearance decision record
- updates to
  [COMPUTATIONAL_SUBSTRATE_MASTER.md](COMPUTATIONAL_SUBSTRATE_MASTER.md)
  and [../PROJECT_STATUS.md](../PROJECT_STATUS.md)

## Success Criteria

This plan counts as successful only if all of the following are true:

- the admitted slice expands materially on the same-sheet frontier
- at least one bounded off-sheet family is admitted
- repair-sensitive cases stop depending on hidden host cleanup
- the retained host shell is narrowed enough that it no longer appears in
  the top blocker list in the master document
- the replay baseline remains exact at zero cached fallback

## Risks

- broader shared-group families may reveal more live Calc asymmetry than the
  current bounded slices do
- repair-sensitive normalization may split into multiple incompatible
  cleanup families instead of one reusable rule set
- off-sheet closure may expose workbook-wide invalidation or listener churn
  that forces tighter fences than expected
- reducing the host shell may require more explicit host-service adapters
  rather than less code

## Working Rule For This Program

The program should stay ambitious in scope but conservative in admission.

That means:

- build broad proof infrastructure
- try to clear all four blockers within one staged roadmap
- but admit only families that close exactly under live proof

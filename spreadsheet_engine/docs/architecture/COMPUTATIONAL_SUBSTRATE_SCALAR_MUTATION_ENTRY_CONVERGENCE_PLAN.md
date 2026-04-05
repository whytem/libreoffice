# Computational Substrate Scalar Mutation Entry Convergence Plan

Status: implementation-ready plan

## Purpose

This document defines the next bounded proof cycle after the completed
[COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_DECISION_RECORD.md).

That closeout left one narrow blocker between the current validation-only
mutation-entry surface and a settled live mutation-entry boundary:

- admitted-slice scalar mutation entry still closes with a broadcaster-only
  computational mismatch after live realization
- queue comparison already remains exact
- graph comparison already remains exact and full-match
- IR comparison already remains accepted

The next adjacent concern is therefore not broader mutation authority. It is
scalar-entry broadcaster-canonicalization convergence.

This plan is not a general mutation-entry rewrite. It is a narrowly bounded
reassessment of whether direct admitted-slice `SetScalarValue` can be brought
into full computational alignment with the already-exact queue and graph
results.

## Why This Is The Best Next Path

The project has already proven, on the admitted slice:

- engine-owned resident cell storage
- engine-owned resident wiring containers
- engine-owned formula-cell lifetime decisions
- engine-owned mutable computational state plus graph, wiring, and queue
  decisions
- exact live formula-entry proof
- exact live admitted structural-entry proof
- validation-only scalar mutation entry with a single remaining broadcaster
  canonicalization gap

That means the shortest path toward settled engine-owned live mutation entry
is:

1. isolate the scalar-entry broadcaster mismatch
2. determine whether it is a pure realization-order/canonicalization issue
3. converge it without widening the admitted mutation surface
4. rerun the mutation-entry decision only after exact computational
   convergence is proven

## Plan Goal

Determine whether admitted-slice direct scalar mutation entry can reach exact
computational equivalence after live realization, so that the current
validation-only scalar mutation-entry path can either:

- proceed into the settled live boundary
- remain validation-only with a clearly bounded caveat
- or be deferred again if the broadcaster canonicalization gap proves to be a
  deeper host-boundary issue

## Entry Boundary

This plan begins from the completed mutation-entry closeout:

- the admitted mutation-entry contract is already frozen
- the engine already owns admitted scalar mutation request shape and routing
- the engine already owns admitted scalar after-state decisions
- Calc can already host raw admitted scalar mutation apply and engine-owned
  resident-state realization
- exact queue verification already holds
- exact graph verification already holds
- the only known remaining scalar-entry mismatch is broadcaster-related
  computational canonicalization after live realization

This plan must therefore treat the current scalar-entry gap as a bounded
equivalence problem, not as a reason to widen the authority surface.

## Non-Goals

This plan should not attempt to:

- widen beyond admitted `SetScalarValue`
- widen into direct formula-entry or structural-entry work, which are already
  proven separately
- move rollback out of Calc
- widen into named-range-sensitive, shared-group-sensitive, off-sheet, or
  sheet-wide classes
- move token-container ownership
- reopen resident storage, resident wiring-container, or formula-cell
  lifetime migration
- weaken exact queue or graph requirements
- treat a non-exact computational comparison as “good enough”

## Required Deliverables

This plan is complete only when all of the following exist:

1. a checked-in contract note freezing the scalar-entry convergence surface
2. a checked-in scenario/equivalence note for broadcaster canonicalization on
   the admitted scalar slice
3. an implementation note for the scalar-entry canonicalization observation
   and classification path
4. an implementation note for the scalar-entry convergence changes in Calc
   realization and/or engine-owned wiring replay
5. a checked-in evidence note covering exact scalar-entry proof and retained
   rollback-triggering cases
6. a checked-in decision record saying whether admitted scalar mutation
   entry:
   - proceeds
   - remains validation-only
   - or is deferred again

## Workstreams

### 1. Freeze The Scalar-Entry Convergence Contract

Freeze the exact admitted scalar-entry surface before implementation work
begins.

The contract should name:

- the admitted workbook and mutation class
- the exact scalar-entry cases covered by this convergence cycle
- retained Calc-owned host surfaces that stay out of scope
- exact computational equivalence requirements
- what counts as convergence versus immediate defer

Required artifact:

- one checked-in scalar-entry convergence contract note

### 2. Freeze The Broadcaster Canonicalization Matrix

Define the representative scalar-entry broadcaster scenarios and the
equivalence rules used to judge convergence.

This note should define:

- direct scalar-to-formula listener cases
- transitive scalar invalidation chains
- stable ordering and canonicalization expectations for admitted live
  broadcasters and listeners
- forbidden host-only shortcuts
- any normalization that is allowed during diagnosis but not at closeout

Required artifact:

- one checked-in scalar-entry canonicalization matrix/equivalence note

### 3. Build The Scalar-Entry Observation And Classification Path

Make the scalar-entry mismatch observable enough to distinguish:

- pure ordering differences
- duplicate/empty broadcaster materialization
- listener-anchor canonicalization differences
- true graph or dependency errors

This workstream should:

- add bounded scalar-entry proof lanes
- make the remaining scalar-entry broadcaster mismatch explicit in test output
- keep the observation layer narrowly scoped to admitted scalar entry

Required artifact:

- one checked-in implementation note for scalar-entry observation and
  classification

### 4. Build The Scalar-Entry Convergence Path

Implement the narrowest change set needed to bring admitted scalar mutation
entry into exact computational alignment if the mismatch proves fixable.

This workstream should:

- tighten Calc live wiring realization and/or mutation-entry realization
  ordering as needed
- preserve engine-owned admitted resident storage, wiring, and lifetime
  decisions
- keep queue and graph exactness intact
- keep rollback behavior explicit and green

Required artifact:

- one checked-in implementation note for scalar-entry convergence

### 5. Freeze Scalar-Entry Differential Evidence

Run the bounded scalar-entry proof cycle and record the results.

This evidence note should summarize:

- exact scalar-entry comparisons between engine-owned after-state and live
  Calc realization
- before/after broadcaster canonicalization results
- retained dirty-baseline rejection and rollback coverage
- memory and performance observations
- whether the scalar-entry gap is actually closed

Required artifact:

- one checked-in scalar-entry convergence evidence note

### 6. Freeze The Scalar-Entry Admission Decision

Close the plan with an explicit decision record.

The closeout must say one of:

- proceed with admitted scalar mutation entry as part of the settled live
  boundary
- keep scalar mutation entry validation-only
- defer scalar mutation entry again

The decision record must also name the next adjacent concern after this
closeout:

- broader mutation-entry reassessment
- broader object-realization reassessment
- or another newly bounded equivalence gap if scalar convergence still does
  not hold

Required artifact:

- one checked-in scalar-entry admission decision record

## Target Surfaces

The most likely implementation surfaces for this plan are:

- [ComputationalSubstrateMutationEntry.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateMutationEntry.hxx)
- [ComputationalSubstrateWiring.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateWiring.hxx)
- [ComputationalSubstrateCellStorage.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateCellStorage.hxx)
- [ComputationalSubstrateFormulaCellLifetime.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateFormulaCellLifetime.hxx)
- [MutableComputationalSubstrate.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/MutableComputationalSubstrate.hxx)
- [MutationEntry.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/MutationEntry.hxx)
- [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)
- [computational_substrate_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/computational_substrate_tests.cxx)

## Recommended Execution Order

The recommended order is:

1. freeze the scalar-entry convergence contract
2. freeze the broadcaster canonicalization matrix
3. build the scalar-entry observation/classification path
4. build the scalar-entry convergence implementation
5. freeze the differential evidence
6. close with an explicit proceed / validation-only / defer decision

## Validation Contract

At minimum, each bounded implementation step should keep the following green:

- `CppunitTest_sc_ucalc_dependency_shadow`
- `CppunitTest_sc_ucalc_workbook_facade`
- `CppunitTest_sc_ucalc_compile_diff`
- `spreadsheetengine_computational_graph_tests`
- `spreadsheetengine_computational_substrate_tests`
- `spreadsheetengine_workbook_facade_tests`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

If new scalar-entry-specific proof lanes are added, they should be made part
of this standing contract before closeout.

## Exit Criteria

This plan is complete only if all of the following are true:

- the scalar-entry convergence contract is frozen and respected
- the broadcaster canonicalization scenarios and equivalence rules are
  explicit
- the scalar-entry mismatch is diagnosable in bounded proof lanes
- the admitted scalar mutation-entry path is either brought to exact
  computational convergence or explicitly shown not to converge
- queue and graph exactness remain intact throughout
- rollback and dirty-baseline rejection remain explicit and green
- the closeout decision says whether admitted scalar mutation entry:
  - proceeds
  - remains validation-only
  - or is deferred again

The plan is not complete merely because the mismatch is better described. It
is complete only when the scalar mutation-entry gap is either closed or
explicitly re-deferred with evidence.

# Computational Substrate Final Rollback Reassessment Plan

Status: implementation-ready plan

## Purpose

This document defines the next explicit proof cycle after the completed
[COMPUTATIONAL_SUBSTRATE_OBJECT_REALIZATION_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_OBJECT_REALIZATION_DECISION_RECORD.md).

That closeout moved the admitted slice to its strongest proven position so
far:

- the engine owns admitted resident cell storage
- the engine owns admitted resident wiring containers
- the engine owns admitted formula-cell lifetime decisions
- the engine owns admitted scalar mutation-entry request shape, routing, and
  after-state decisions
- the engine owns the admitted live object-realization record consumed by
  Calc

What still remains host-owned on that same slice is narrower:

- raw document mutation APIs
- final rollback after failed verification

This plan is not a broad document-host transplant. It is the next bounded
reassessment of whether admitted-slice rollback can move further toward
engine-authored authority without reopening broader mutation, workbook-scope,
or host-service migration.

## Why This Is The Best Next Path

The project has already proven nearly all of the bounded admitted-slice live
state surfaces that were the original blockers:

- resident cell storage
- resident wiring containers
- formula-cell lifetime decisions
- scalar mutation-entry decisions
- live object-realization records

That means the most actionable remaining host-owned choke point on the
bounded slice is no longer storage, wiring, mutation intent, or live object
realization. It is the rollback boundary around those already-engine-authored
surfaces.

The shortest path toward broader engine-owned live authority is therefore:

1. make admitted rollback more explicitly engine-authored
2. keep Calc as the temporary raw mutation API host
3. preserve exact queue, computational, graph, and replay verification
4. decide only after that proof whether broader mutation-shell migration is
   justified

## Plan Goal

Determine whether `spreadsheet_engine` can safely own more of the admitted
rollback shape on the admitted slice, so that Calc no longer quietly
reconstructs rollback state with hidden local authority.

The goal is to decide one explicit question:

- proceed with engine-authored admitted-slice rollback
- keep rollback as a hybrid host path
- or defer broader rollback migration again

## Entry Boundary

This plan begins from the current settled admitted-slice boundary:

- the first-stage extraction boundary is complete and stable
- the computational-substrate authority program closed with a narrow proceed
  result
- the narrow opt-in rollout remains bounded to the admitted scalar and
  single-sheet structural slice
- the engine already owns:
  - admitted resident cell storage
  - admitted resident wiring containers
  - admitted formula-cell lifetime decisions
  - admitted scalar mutation-entry request shape, routing, and after-state
    decisions
  - admitted live object-realization records
- exact queue, computational, graph, broadcaster, and replay verification
  already holds on the admitted scalar mutation-entry slice
- Calc still owns:
  - raw document mutation APIs
  - final rollback after failed verification

This plan must therefore treat rollback as the next proof surface, not as an
already-admitted extension of the current boundary.

## Non-Goals

This plan should not attempt to:

- move raw document mutation APIs out of Calc in the same cycle
- widen beyond the admitted scalar lifecycle and admitted narrow structural
  slice
- widen into shared-group-sensitive or named-range-sensitive behavior
- widen into sheet insert, delete, rename, or move
- move token-container ownership
- reopen resident storage, resident wiring, mutation-entry, or
  object-realization migration
- weaken exact queue, computational, graph, replay, or rollback
  requirements
- claim broad `ScDocument` host independence

## Required Deliverables

This plan is complete only when all of the following exist:

1. a checked-in contract note freezing the admitted rollback surface
2. a checked-in scenario matrix for admitted rollback cases
3. a checked-in observation and classification note for rollback divergence
4. a checked-in implementation note for the engine-authored rollback path
5. a checked-in evidence note covering exact, rollback, and deferred
   outcomes
6. a checked-in decision record saying whether admitted-slice rollback:
   - proceeds
   - remains hybrid
   - or is deferred again

## Workstreams

### 1. Freeze The Rollback Contract

Freeze the exact admitted rollback surface before implementation work begins.

This contract should name:

- the admitted workbook and mutation classes
- the exact rollback classes under reassessment:
  - rollback of admitted scalar mutation entry
  - rollback of admitted formula replace/remove realization
  - rollback of admitted row and column structural mutations
  - rollback of admitted listener/broadcaster, formula-tree, and
    formula-track live participation
- retained Calc-owned host surfaces that stay out of scope in this cycle
- exact success criteria versus immediate defer triggers

Required artifact:

- one checked-in rollback contract note

### 2. Freeze The Rollback Scenario Matrix

Define the representative admitted-slice rollback scenarios that the proof
cycle must cover.

This matrix should classify:

- rollback after failed scalar overwrite verification
- rollback after failed formula replace or clear verification
- rollback after admitted row and column structural divergence
- rollback of direct listener cases versus transitive invalidation chains
- rollback cases that must remain explicitly rejected or deferred

Each scenario should be marked as one of:

- candidate for settled rollback authority
- validation-only evidence
- explicit reject or defer

Required artifact:

- one checked-in rollback scenario matrix

### 3. Build The Rollback Observation And Classification Path

Make remaining rollback gaps explicit enough to distinguish:

- exact rollback restore
- ordering-only rollback restore differences
- missing restored live objects
- host-only rollback reconstruction behavior
- true queue, computational, graph, or replay divergence

This workstream should:

- add bounded admitted-slice rollback proof lanes
- surface rollback differences explicitly in test output
- keep the observation layer scoped only to the admitted slice

Required artifact:

- one checked-in rollback observation and classification note

### 4. Build The Engine-Authored Rollback Path

Implement the narrowest change set needed to make admitted rollback more
explicitly engine-authored on the bounded slice.

This workstream should:

- add stable engine-authored rollback records or equivalent admitted rollback
  instructions
- teach Calc to restore admitted live state from that engine-authored
  rollback surface without reclaiming hidden after-state authority
- preserve engine-owned resident storage, resident wiring, mutation-entry,
  and object-realization decisions
- keep the raw mutation shell in Calc

Required artifact:

- one checked-in implementation note for the engine-authored rollback path

### 5. Freeze Differential Rollback Evidence

Run the bounded rollback proof cycle and record the results.

This evidence note should summarize:

- exact admitted-slice comparisons between engine-authored rollback output
  and live Calc rollback restore
- whether cell storage, formula-cell lifetime, wiring, formula-tree, and
  formula-track rollback all close exactly
- repair-detected and reject cases
- memory and performance observations
- whether the host-owned rollback layer actually shrinks in a meaningful way

Required artifact:

- one checked-in rollback evidence note

### 6. Freeze The Rollback Decision

Close the plan with an explicit decision record.

The closeout must say one of:

- proceed with engine-authored admitted-slice rollback
- keep rollback hybrid on the admitted slice
- defer broader rollback migration again

The decision record must also state the next adjacent concern after this
closeout:

- broader raw mutation API migration
- broader admitted-slice host-shell reassessment
- or another newly bounded rollback gap if this proof does not hold

Required artifact:

- one checked-in rollback decision record

## Target Surfaces

The most likely implementation surfaces for this plan are:

- [ComputationalSubstrateMutationEntry.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateMutationEntry.hxx)
- [ComputationalSubstrateObjectRealization.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateObjectRealization.hxx)
- [ComputationalSubstrateCellStorage.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateCellStorage.hxx)
- [ComputationalSubstrateFormulaCellLifetime.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateFormulaCellLifetime.hxx)
- [ComputationalSubstrateWiring.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateWiring.hxx)
- [MutableComputationalSubstrate.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/MutableComputationalSubstrate.hxx)
- a new admitted-slice rollback compat surface under
  `spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/`
- [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)
- [computational_substrate_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/computational_substrate_tests.cxx)
- [workbook_facade_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/workbook_facade_tests.cxx)

## Recommended Execution Order

The recommended order is:

1. freeze the rollback contract
2. freeze the rollback scenario matrix
3. build the observation and classification path
4. build the engine-authored rollback path
5. freeze the differential evidence
6. close with an explicit proceed / hybrid / defer decision

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

If new rollback-specific proof lanes are added, they should become part of
this standing contract before closeout.

## Exit Criteria

This plan is complete only if all of the following are true:

- the admitted rollback contract is frozen and respected
- the scenario matrix and observation model are explicit and stable
- the engine-authored admitted rollback path is implemented or explicitly
  limited
- Calc can restore admitted live state from engine-authored rollback records
  without reclaiming hidden after-state authority
- exact queue, computational, graph, replay, and rollback verification still
  hold on the admitted slice
- repair-detected and reject behavior remain explicit and green
- the closeout decision says whether admitted-slice rollback:
  - proceeds
  - remains hybrid
  - or is deferred again

The plan is not complete merely because the engine stores more before-state
metadata. It is complete only when the admitted rollback boundary is either
proven, explicitly limited, or explicitly deferred.

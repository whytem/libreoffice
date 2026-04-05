# Computational Substrate Object Realization Reassessment Plan

Status: implementation-ready plan

## Purpose

This document defines the next explicit proof cycle after the completed
[COMPUTATIONAL_SUBSTRATE_SCALAR_MUTATION_ENTRY_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SCALAR_MUTATION_ENTRY_DECISION_RECORD.md).

That closeout moved the bounded admitted slice to its strongest proven
position so far:

- the engine owns admitted resident cell storage
- the engine owns admitted resident wiring containers
- the engine owns admitted formula-cell lifetime decisions
- the engine owns admitted scalar mutation-entry request shape, routing, and
  after-state decisions
- direct scalar mutation entry now closes with exact queue, computational,
  graph, and broadcaster verification after Calc realization

What still remains host-owned on that same slice is narrower:

- raw document mutation APIs
- live object realization
- final rollback

This plan is not a broad document-host transplant. It is the next bounded
reassessment of whether admitted-slice live object realization can move
further toward engine-authored authority without reopening broader storage,
mutation, or workbook-scope migration.

## Why This Is The Best Next Path

The current project has already proven the engine-owned state surfaces that
were previously the biggest blockers:

- admitted resident cell storage
- admitted resident wiring containers
- admitted formula-cell lifetime decisions
- admitted scalar mutation-entry decisions

That means the most actionable remaining host-owned choke point on the
bounded slice is no longer storage or wiring modeling. It is the live
realization layer where Calc still turns engine-owned resident state into
live formula objects, listener/broadcaster participation, and final host
objects.

The shortest path toward broader engine-owned live authority is therefore:

1. make admitted live realization more explicitly engine-authored
2. keep Calc as the temporary mutation API and rollback host
3. preserve exact queue, computational, and graph verification
4. decide only after that proof whether further rollback or host-shell
   migration is justified

## Plan Goal

Determine whether `spreadsheet_engine` can safely own more of the admitted
live object-realization shape on the admitted slice, so that Calc no longer
quietly reconstructs the same live objects with hidden local authority.

The goal is to decide one explicit question:

- proceed with engine-authored admitted-slice object realization
- keep object realization as a validation-only or hybrid host path
- or defer broader object-realization migration again

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
- exact queue, computational, graph, and broadcaster verification already
  holds on the admitted scalar mutation-entry slice
- Calc still owns:
  - raw document mutation APIs
  - live object realization
  - final rollback

This plan must therefore treat object realization as the next proof surface,
not as an already-admitted extension of the current boundary.

## Non-Goals

This plan should not attempt to:

- move rollback out of Calc in the same cycle
- widen beyond the admitted scalar lifecycle and admitted narrow structural
  slice
- widen into shared-group-sensitive or named-range-sensitive behavior
- widen into sheet insert, delete, rename, or move
- move token-container ownership
- reopen resident cell-storage or resident wiring-container migration
- weaken exact queue, computational, graph, or rollback requirements
- claim broad `ScDocument` host independence

## Required Deliverables

This plan is complete only when all of the following exist:

1. a checked-in contract note freezing the admitted object-realization
   surface
2. a checked-in scenario matrix for admitted live object-realization cases
3. a checked-in observation and classification note for object-realization
   divergence
4. a checked-in implementation note for the engine-authored
   object-realization path
5. a checked-in evidence note covering exact, rollback, and deferred
   object-realization outcomes
6. a checked-in decision record saying whether admitted-slice object
   realization:
   - proceeds
   - remains validation-only or hybrid
   - or is deferred again

## Workstreams

### 1. Freeze The Object-Realization Contract

Freeze the exact admitted object-realization surface before any new
implementation work begins.

This contract should name:

- the admitted workbook and mutation classes
- the exact live object classes under reassessment:
  - admitted `ScFormulaCell` create, replace, and remove realization
  - admitted listener and broadcaster realization from engine-owned resident
    wiring state
  - admitted formula-tree and formula-track live participation
- retained Calc-owned host surfaces that stay out of scope in this cycle
- exact success criteria versus immediate defer triggers

Required artifact:

- one checked-in object-realization contract note

### 2. Freeze The Object-Realization Scenario Matrix

Define the representative admitted-slice scenarios that the proof cycle must
cover.

This matrix should classify:

- scalar overwrite, formula replace, and clear cases on the admitted slice
- admitted row and column structural cases that already participate in the
  narrow rollout
- create versus replace versus remove realization paths
- realization cases with direct listeners versus transitive invalidation
  chains
- realization cases that must remain explicitly rejected or deferred

Each scenario should be marked as one of:

- candidate for settled object-realization authority
- validation-only evidence
- explicit reject or defer

Required artifact:

- one checked-in object-realization scenario matrix

### 3. Build The Object-Realization Observation And Classification Path

Make remaining live object-realization gaps explicit enough to distinguish:

- ordering-only realization differences
- missing realized object materialization
- host-only repair or reconstruction behavior
- true graph, queue, or computational divergence

This workstream should:

- add bounded admitted-slice realization proof lanes
- surface realized-object differences explicitly in test output
- keep the observation layer scoped only to the admitted slice

Required artifact:

- one checked-in object-realization observation and classification note

### 4. Build The Engine-Authored Object-Realization Path

Implement the narrowest change set needed to make admitted live realization
more explicitly engine-authored on the bounded slice.

This workstream should:

- add stable engine-authored realization records or equivalent admitted live
  realization instructions
- teach Calc to realize admitted live objects from that engine-authored
  surface without reclaiming hidden after-state authority
- preserve engine-owned resident storage, resident wiring, lifetime, and
  mutation-entry decisions
- keep rollback explicit and green

Required artifact:

- one checked-in implementation note for the engine-authored
  object-realization path

### 5. Freeze Differential Object-Realization Evidence

Run the bounded proof cycle and record the results.

This evidence note should summarize:

- exact admitted-slice comparisons between engine-authored realization output
  and live Calc realization
- whether formula-cell, listener/broadcaster, formula-tree, and formula-track
  realization all close exactly
- rollback-triggering and repair-detected cases
- memory and performance observations
- whether the host-owned realization layer actually shrinks in a meaningful
  way

Required artifact:

- one checked-in object-realization evidence note

### 6. Freeze The Object-Realization Decision

Close the plan with an explicit decision record.

The closeout must say one of:

- proceed with engine-authored admitted-slice object realization
- keep object realization validation-only or hybrid on the admitted slice
- defer broader object-realization migration again

The decision record must also state the next adjacent concern after this
closeout:

- final rollback reassessment
- broader mutation API migration
- or another newly bounded host-realization gap if this proof does not hold

Required artifact:

- one checked-in object-realization decision record

## Target Surfaces

The most likely implementation surfaces for this plan are:

- [MutableComputationalSubstrate.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/MutableComputationalSubstrate.hxx)
- [ComputationalSubstrateCellStorage.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateCellStorage.hxx)
- [ComputationalSubstrateFormulaCellLifetime.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateFormulaCellLifetime.hxx)
- [ComputationalSubstrateWiring.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateWiring.hxx)
- [ComputationalSubstrateMutationEntry.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateMutationEntry.hxx)
- a new admitted-slice object-realization compat surface under
  `spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/`
- [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)
- [computational_substrate_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/computational_substrate_tests.cxx)
- [workbook_facade_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/workbook_facade_tests.cxx)

## Recommended Execution Order

The recommended order is:

1. freeze the object-realization contract
2. freeze the object-realization scenario matrix
3. build the observation and classification path
4. build the engine-authored object-realization path
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

If new object-realization-specific proof lanes are added, they should become
part of this standing contract before closeout.

## Exit Criteria

This plan is complete only if all of the following are true:

- the admitted object-realization contract is frozen and respected
- the scenario matrix and observation model are explicit and stable
- the engine-authored admitted object-realization path is implemented or
  explicitly limited
- Calc can realize admitted live objects from engine-authored realization
  records without reclaiming hidden after-state authority
- exact queue, computational, and graph verification still hold on the
  admitted slice
- rollback and repair-detected behavior remain explicit and green
- the closeout decision says whether admitted-slice object realization:
  - proceeds
  - remains validation-only or hybrid
  - or is deferred again

The plan is not complete merely because the engine stores more live-object
metadata. It is complete only when the admitted object-realization boundary
is either proven, explicitly limited, or explicitly deferred.

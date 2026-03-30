# Dependency And Invalidation Extraction Plan

## Purpose

This document turns the next roadmap step after the Calc-backed workbook facade
into an implementation-ready plan:

- extract dependency analysis out of Calc-owned internals
- extract invalidation planning out of Calc-owned dirty-state side effects
- keep Calc as the storage owner and mutation executor for now
- make `spreadsheet_engine` the place where dependency relationships and
  mutation-to-dirty planning live

This is intentionally a separate plan because "extract the dependency graph"
can easily sprawl into a premature scheduler rewrite or an accidental storage
rewrite. That is not the goal of this milestone.

## Executive Summary

The right next move is an **engine-owned dependency and invalidation planner**
that runs against the workbook facade and canonical compiler/token model.

The milestone should:

1. build dependency snapshots from the engine workbook facade
2. model reverse dependencies and dirty causes in engine-owned types
3. translate normalized mutation events into invalidation plans
4. run in shadow mode against Calc and compare predicted dirty sets with Calc's
   current behavior
5. stay conservative where fidelity is incomplete, preferring safe
   over-invalidation to under-invalidation

The milestone should **not**:

- move `ScDocument` storage ownership
- replace Calc listeners/broadcasters yet
- replace recalculation scheduling yet
- rewrite `ScInterpreter`
- migrate rendering, persistence, UNO, or UI state

The milestone is complete when `spreadsheet_engine` owns a stable dependency
snapshot and invalidation-planning layer that is validated in shadow against
Calc for representative mutations and is ready to become the foundation for
later scheduler extraction.

## Current Implementation Status

The first substantial extraction slice is now in the tree.

Implemented:

- engine-owned dependency contracts in
  `detail/dependency/DependencyTypes.hxx`
- workbook-facade snapshot building in
  `detail/dependency/DependencySnapshot.hxx`
- reverse-dependency indexing for formula-bearing nodes inside the snapshot
- invalidation planning in
  `detail/dependency/InvalidationPlanner.hxx`
- opt-in Calc runtime auditing in
  `compat/libreoffice/DependencyShadow.hxx`
- standalone validation in
  `tests/unit/dependency_invalidation_tests.cxx`
- Calc shadow validation in
  `sc/qa/unit/ucalc_dependency_shadow.cxx`
- maintenance-profile integration via
  `integration/libreoffice/run_spreadsheet_unit_tests.sh`

Current promoted coverage:

- direct single-cell dependencies
- direct range dependencies
- named-range dependencies
- shared-formula group anchor normalization
- non-structural mutations:
  - `SetScalarValue`
  - `SetFormula`
  - `ClearCell`
  - `ClearRange`
- named-range mutations with rebuild signaling
- structural mutations with conservative workbook rebuild scopes
- Calc shadow comparison for representative structural edits
- workbook-scale multi-sheet shadow corpus
- first live runtime shadow consumer on:
  - `ScDocument::SetValue()`
  - `ScDocument::SetString()`
  - `ScDocument::SetEmptyCell()`

Current explicit limitations:

- unsupported/dynamic constructs such as `INDIRECT`, `OFFSET`, parse failures,
  and range constructors widen invalidation conservatively
- structural edits still rely on workbook rebuild scopes rather than precise
  address-stable parity after coordinate-shifting mutations
- the runtime audit is opt-in (`SPREADSHEET_ENGINE_DEPENDENCY_SHADOW`) and
  remains non-authoritative; Calc still owns actual dirty-bit side effects

## Milestone Closeout Status

The milestone is now closed out.

Stable promoted mutation families:

- `SetScalarValue`
- `SetFormula`
- named-range-dependent invalidation via value changes
- representative structural rebuild mutations:
  - insert rows
  - delete columns

Deferred unsupported or intentionally conservative areas:

- precise post-shift structural address parity for all row/column mutations
- runtime auditing for named-range mutations and structural edits
- dynamic reference constructs (`INDIRECT`, `OFFSET`) and parse failures,
  which still widen conservatively
- authoritative dirty-bit ownership and recalc scheduling, which remain the
  next extraction program

Validation closeout summary:

- standalone full suite: `26/26` passing
- Calc dedicated lanes:
  - `CppunitTest_sc_ucalc_dependency_shadow`
  - `CppunitTest_sc_ucalc_workbook_facade`
  - `CppunitTest_sc_ucalc_sharedformula`
  - `CppunitTest_sc_ucalc_compile_diff`
  - `CppunitTest_sc_ucalc`
- Calc smoke profile now includes `CppunitTest_sc_ucalc_dependency_shadow`
- the smoke profile still shares an unrelated pre-existing blocker:
  `CppunitTest_sc_spreadsheet_functions_test` aborts during `address.fods`
  import in `ScFormulaCell::CompileXML`

## Problem Statement

Today Calc still owns:

- dependency relationships embedded across `ScDocument`, `ScFormulaCell`,
  token trees, listeners, broadcasters, and various update paths
- dirty-state bookkeeping and mutation-triggered recalculation decisions
- the side effects that connect structural edits, reference updates, and
  formula invalidation

We now have two important prerequisites:

- an engine-owned token/compiler-host model
- an engine-owned Calc-backed workbook facade with mutation event modeling

That means the next clean extraction boundary is no longer "formula parsing" or
"basic workbook reads"; it is **dependency and invalidation planning**.

If we skip this and try to extract scheduling or more execution logic first, we
will keep core recalculation semantics tied to Calc's listener graph and dirty
flags, which makes later extraction significantly harder.

## Milestone Definition

### What this milestone must achieve

By the end of this milestone:

- `spreadsheet_engine` owns a dependency snapshot model over workbook facade
  state
- `spreadsheet_engine` owns an invalidation planner that consumes normalized
  mutation events and dependency snapshots
- Calc has adapter and shadow-validation paths that compare engine invalidation
  plans with Calc's current dirty/invalidation behavior
- the first mutation families are validated tightly enough to be trusted as the
  basis for later scheduler work
- the planner handles unsupported constructs conservatively and reports why it
  widened invalidation

### What this milestone does not require

This milestone does **not** require:

- moving listener/broadcaster side effects out of Calc
- making engine invalidation authoritative in production
- extracting recalc scheduling order
- extracting the CPU evaluator
- extracting OpenCL or threaded backends
- full external-reference graph ownership

## Scope Boundary

### In Scope

- engine-owned dependency node, edge, and invalidation-plan types
- dependency snapshot building over the workbook facade
- reverse-dependency indexing for formula-bearing nodes
- mutation classification and dirty-set planning
- conservative handling of unsupported dependency shapes
- Calc shadow validation and comparison tooling
- first low-risk shadow consumers built on top of the planner

### Explicitly Out Of Scope

- authoritative mutation execution
- authoritative dirty-bit setting in production
- scheduler extraction
- interpreter extraction
- listener/broadcaster replacement
- rendering/import/export/UI state

## Completion Criteria

The milestone is complete when all of the following are true.

### Architecture

- an engine-owned dependency snapshot model exists under `spreadsheet_engine`
- an engine-owned invalidation planner consumes workbook-facade state plus
  `MutationEvent`
- the model does not depend on `ScDocument` internals in its public contract

### Functional Coverage

- direct single-cell and range references participate in dependency snapshots
- named-range dependencies participate in dependency snapshots
- shared-formula groups are normalized enough for stable invalidation planning
- non-structural mutations produce stable dirty sets
- structural mutations produce stable dirty sets and/or explicit rebuild scopes

### Validation

- dedicated standalone dependency/invalidation tests are green
- dedicated Calc dependency/invalidation tests are green
- shadow comparison lanes against Calc behavior are green for the promoted
  mutation families
- existing compiler/token/FODS lanes stay green

### Adoption

- at least one real shadow invalidation consumer runs through the engine plan
- the facade + dependency planner combination is ready to host scheduler
  extraction next

## Design Principles

### 1. Planner first, side effects later

This milestone extracts **what should become dirty** and **why**.
Calc still performs the actual side effects.

### 2. Use facade identities, not raw Calc pointers

Dependency planning should use:

- `FormulaCellId`
- `NamedRangeId`
- `SheetId`
- `api::CellAddress`

not `ScFormulaCell*` or other Calc-owned pointer identity.

### 3. Normalize shared constructs early

Shared-formula groups and named ranges should be modeled explicitly so the
planner does not depend on ad hoc special cases later.

### 4. Conservative over precise when unsupported

If a construct cannot yet be modeled precisely, the planner should:

- over-invalidate safely
- report why it widened scope
- avoid silent under-invalidation

### 5. Separate snapshot rebuild from dirty planning

Structural edits and non-structural edits are different problems:

- some mutations invalidate existing edges
- some require dirtying dependents only
- some require both graph repair and dirtying

The architecture should model those separately.

### 6. Shadow before authority

The first planner runs in shadow mode with differential validation against
Calc's current behavior.

## Proposed Architecture

## 1. Core concepts

The extracted dependency/invalidation layer should be split into four
subsystems:

1. **Dependency snapshot model**
   - engine-owned nodes and edges
   - immutable or snapshot-style representation
2. **Snapshot builder**
   - builds dependency state from workbook facade + compiler pipeline
3. **Mutation classifier**
   - translates `MutationEvent` into dependency-relevant mutation semantics
4. **Invalidation planner**
   - computes dirty nodes, rebuild scopes, and conservative widening reasons

## 2. Suggested file layout

Recommended new cluster under `spreadsheet_engine`, for example:

- `inc/spreadsheetengine/detail/dependency/DependencyTypes.hxx`
- `inc/spreadsheetengine/detail/dependency/DependencySnapshot.hxx`
- `inc/spreadsheetengine/detail/dependency/DependencyBuilder.hxx`
- `inc/spreadsheetengine/detail/dependency/InvalidationTypes.hxx`
- `inc/spreadsheetengine/detail/dependency/InvalidationPlanner.hxx`
- `inc/spreadsheetengine/detail/dependency/MutationClassifier.hxx`
- `inc/spreadsheetengine/compat/libreoffice/DependencyShadow.hxx`

Exact names can shift, but the split should be:

- engine-owned contracts and logic in `detail/`
- Calc shadow/validation adapter helpers in `compat/libreoffice/`

## 3. Core engine-owned types

Recommended first-pass types:

| Type | Role |
| --- | --- |
| `DependencyNodeId` | stable node identity within a snapshot |
| `DependencyNodeKind` | formula cell, named range, shared group, opaque source |
| `DependencyNode` | normalized dependency-bearing entity |
| `DependencyEdge` | edge from source to dependent with dependency kind |
| `DependencySnapshot` | full forward + reverse dependency view |
| `DependencyBuildReport` | build stats, unsupported constructs, widening reasons |
| `DirtyReason` | scalar change, formula change, structural move, name change, volatile, etc. |
| `RebuildScope` | sheet-level, region-level, workbook-level snapshot repair request |
| `InvalidationPlan` | dirty nodes, rebuild scopes, widening diagnostics |
| `InvalidationStats` | summary for shadow validation and diagnostics |

Recommended identity backing:

- formula cells: `FormulaCellId`
- named ranges: `NamedRangeId`
- shared groups: anchor-based synthetic id over `FormulaGroupDescriptor`

## 4. Dependency node model

Recommended initial node kinds:

- `FormulaCell`
- `NamedRange`
- `SharedFormulaGroup`
- `OpaqueWorkbookSource`

`OpaqueWorkbookSource` is the safe escape hatch for constructs that participate
in invalidation but are not yet modeled precisely, for example:

- some table/db range dependencies
- some external references
- volatile constructs that force wider invalidation

## 5. Edge model

Recommended initial dependency-edge kinds:

- direct single-cell reference
- direct range reference
- named-range reference
- sheet-scope dependency
- structural dependency
- opaque/unsupported dependency

The first milestone does not need every edge kind to be fully precise, but it
must clearly separate:

- exact edges we trust
- conservative edges we widened

## 6. Snapshot builder inputs

The snapshot builder should consume:

- `WorkbookFacade`
- canonical compiled tokens from the engine compiler pipeline
- mutation-insensitive workbook metadata like grammar and sheet identity

It should **not** inspect raw Calc listeners or broadcasters directly.

## 7. Shared-formula normalization

Shared-formula groups should be modeled explicitly rather than as accidental
duplicates of N similar formula cells.

Recommended approach:

- keep formula-cell identity at cell granularity
- optionally build a group summary node for shadow planning and deduplication
- allow the invalidation plan to report both:
  - dirty formula cells
  - dirty shared groups

This gives later scheduler work a clean bridge without forcing this milestone
to become an execution rewrite.

## 8. Two-stage invalidation pipeline

The planner should be designed as two related steps:

### Step A: mutation classification and rebuild planning

Consumes `MutationEvent` and decides whether the existing dependency snapshot is
still structurally valid.

Outputs:

- no rebuild needed
- partial rebuild for specific sheets/regions/nodes
- full snapshot rebuild

### Step B: dirty-set planning

Consumes the existing or rebuilt snapshot and computes which nodes become dirty.

Outputs:

- dirty formula cells
- dirty named ranges
- dirty groups
- reasons and widening notes

This separation is critical because:

- value edits mostly dirty dependents
- formula edits can change both graph shape and dirty set
- structural edits can invalidate both graph shape and dirty set

## First Functional Slice

The smallest meaningful first slice should handle:

- formula-cell dependency snapshot building for direct references
- reverse-dependency indexing
- non-structural invalidation for:
  - `SetScalarValue`
  - `ClearCell`
  - `SetFormula`
- named-range dependency edges for simple references
- shadow comparison against Calc dirty behavior on synthetic workbooks

That slice is small enough to validate tightly and is sufficient to prove the
core architecture.

## Detailed Mutation Strategy

## 1. Non-structural mutations first

Promote first:

- `SetScalarValue`
- `ClearCell`
- `SetFormula`
- `ClearRange`

These are the best first mutations because they stress dependency tracking
without forcing row/column address-shift logic immediately.

## 2. Named-range mutations next

Promote next:

- `AddNamedRange`
- `RemoveNamedRange`
- `RenameNamedRange`

The workbook facade now carries full before/after descriptors for these
mutations, so this is a good second wave.

Expected first-pass behavior:

- mark dependent formulas dirty
- request snapshot rebuild where a name definition or scope changed

## 3. Sheet rename after names

Promote:

- `RenameSheet`

This is more than a cosmetic rename because some formula text and name scopes
may need rebuild or conservative widening. It should stay behind the simpler
name mutations.

## 4. Structural range mutations after that

Promote later:

- `InsertRows`
- `DeleteRows`
- `InsertColumns`
- `DeleteColumns`
- `MoveRange`
- `CopyRange`

These require explicit rebuild-scope planning because the old dependency edges
may no longer be structurally correct after the mutation.

## Unsupported And Conservative Cases

The planner should explicitly classify and report at least these cases:

- volatile functions
- `INDIRECT` / address-string-dependent references
- opaque external references
- unsupported table/db-range dependency shapes
- unsupported add-in/external-name dependency semantics
- unresolved or unsupported compile shapes

Recommended policy:

- keep a reason enum for every widening
- record counts in `DependencyBuildReport` and `InvalidationPlan`
- expose this to tests and diagnostics

## Validation Strategy

## 1. Standalone unit tests

Add a new standalone lane, for example:

- `spreadsheetengine_dependency_tests`

It should cover:

- dependency-node and edge equality
- snapshot building on `InMemoryWorkbookFacade`
- reverse dependency index behavior
- invalidation planning on synthetic mutations
- conservative widening cases

## 2. Calc unit tests

Add a Calc lane, for example:

- `CppunitTest_sc_ucalc_dependency_shadow`

It should cover:

- Calc-backed facade snapshot building
- direct-reference dependency extraction
- shared-formula group normalization
- name dependency extraction
- shadow dirty-set comparison after representative mutations

## 3. Differential shadow harness

For each promoted mutation family:

1. create workbook state in Calc
2. build engine dependency snapshot from `CalcWorkbookFacade`
3. record baseline dirty state
4. apply the mutation through existing Calc code
5. collect actual Calc dirty formula cells / relevant dirty metadata
6. compare against engine `InvalidationPlan`

This is the core proof for the milestone.

## 4. Validation progression

Recommended validation rollout:

1. synthetic single-sheet scalar dependencies
2. multi-sheet direct references
3. named-range dependencies
4. shared-formula groups
5. promoted structural mutations
6. representative FODS-derived shadow corpus

## 5. Acceptance policy

Until the planner is authoritative:

- exact matches are preferred
- conservative supersets are acceptable if explicitly annotated
- silent misses are not acceptable

That means the shadow harness should distinguish:

- exact parity
- conservative over-invalidation
- under-invalidation

Only the last category is a correctness failure.

## Recommended Phased Rollout

### Phase 0: Freeze scope and corpus

Define:

- first node kinds
- first mutation families
- first shadow corpus
- acceptance policy for conservative widening

Deliverables:

- plan checked in
- first validation targets named
- first mutation corpus frozen

### Phase 1: Core types and reports

Add:

- dependency node/edge types
- snapshot and invalidation-plan carriers
- report/diagnostic enums

Validation:

- standalone type tests

### Phase 2: Snapshot builder for direct refs

Add:

- snapshot builder over workbook facade
- direct cell/range dependency extraction from canonical tokens
- reverse dependency index

Validation:

- standalone synthetic workbooks
- Calc snapshot smoke

### Phase 3: Non-structural invalidation planner

Add:

- invalidation planning for `SetScalarValue`, `ClearCell`, `SetFormula`,
  `ClearRange`
- dirty-reason reporting

Validation:

- standalone mutation tests
- Calc shadow diff on synthetic mutations

### Phase 4: Names and groups

Add:

- named-range dependency edges
- shared-formula group normalization
- invalidation handling for named-range mutations

Validation:

- Calc shadow tests on local/global names
- row-0 shared group coverage

### Phase 5: Structural rebuild planning

Add:

- rebuild-scope modeling
- planner support for insert/delete rows/columns and move/copy range

Validation:

- Calc shadow tests on structural edits
- explicit parity vs conservative widening reporting

### Phase 6: Workbook-scale shadow harness

Add:

- multi-sheet representative corpus
- FODS-derived or TSV-derived shadow workbook cases
- summary diagnostics and failure triage output

Validation:

- dedicated Calc shadow lane
- maintenance script integration if stable

### Phase 7: First real shadow consumer

Add:

- a low-risk engine-side invalidation audit or diagnostic consumer in Calc
- optional debug/validation hook around promoted mutation families

This is the natural place to satisfy the remaining "first live facade
consumer" expectation from the workbook-facade program.

### Phase 8: Closeout and handoff to scheduler extraction

Closeout when:

- promoted mutation families are green in shadow
- unsupported areas are clearly documented
- the planner is good enough to become the scheduler extraction substrate

## Proposed Diagnostics

Every invalidation plan should be able to summarize:

- dirty formula-cell count
- dirty named-range count
- dirty group count
- rebuild scope count
- exact vs conservative classification
- unsupported construct counts by reason

This is important both for developer confidence and for future scheduler work.

## Risks And Mitigations

### Risk 1: hidden Calc-specific behavior around listeners

Mitigation:

- stay in shadow first
- compare dirty sets, not just graph counts
- preserve widening reasons explicitly

### Risk 2: dependency graph scope balloons into scheduler scope

Mitigation:

- keep this milestone planner-only
- do not move execution order or queue ownership yet

### Risk 3: structural edits require more rebuild than expected

Mitigation:

- model rebuild scopes explicitly
- allow conservative sheet/workbook rebuilds first
- tighten incrementally later

### Risk 4: unsupported reference constructs create false confidence

Mitigation:

- classify unsupported constructs explicitly
- fail on silent misses
- allow conservative supersets only when reported

## Deferred Areas

These should remain deferred until this milestone proves itself:

- listener/broadcaster ownership changes
- authoritative dirty-bit setting
- recalc scheduling order
- execution backends
- full external reference graph ownership
- conditional formatting / validation dependency integration
- pivot/runtime cache dependency ownership

## Tracking Checklist

### Phase 0

- [x] freeze first node kinds
- [x] freeze first mutation families
- [x] freeze acceptance policy for conservative widening
- [x] freeze first shadow corpus

### Phase 1

- [x] add engine-owned dependency and invalidation types
- [x] add standalone type/report tests

### Phase 2

- [x] add dependency snapshot builder for direct refs
- [x] add reverse-dependency indexing
- [x] add Calc snapshot smoke tests

### Phase 3

- [x] add non-structural invalidation planning
- [x] add dirty-reason reporting
- [x] add Calc shadow comparison for non-structural mutations

### Phase 4

- [x] add named-range dependency support
- [x] add shared-formula group normalization
- [x] add named-range mutation invalidation planning

### Phase 5

- [x] add rebuild-scope modeling
- [x] add structural mutation planning
- [x] add Calc shadow comparison for structural mutations

### Phase 6

- [x] add workbook-scale shadow corpus
- [x] add summary diagnostics and maintenance-lane integration

### Phase 7

- [x] add first real shadow invalidation consumer in Calc
- [x] keep fallback and safety lanes green

### Phase 8

- [x] document stable promoted mutation families
- [x] document deferred unsupported areas
- [x] close out the milestone

## Recommended First Implementation Slice

Start with:

1. `DependencyTypes.hxx`
2. `DependencySnapshot.hxx`
3. `DependencyBuilder.hxx`
4. standalone direct-reference snapshot tests on `InMemoryWorkbookFacade`
5. Calc smoke lane proving the same snapshot builder runs against
   `CalcWorkbookFacade`

That slice is small, testable, and establishes the exact substrate that the
non-structural invalidation planner will need next.

# Next Steps: Extracting Calc's Remaining Calculation Core

## Purpose

This report evaluates the feasibility of extracting the remaining major calculation subsystems from Calc into `spreadsheet_engine`:

1. the main spreadsheet formula compiler/parser
2. the document-backed dependency graph and invalidation model
3. recalculation orchestration and scheduling
4. the execution backend

The target end-state is:

- Calc consumes a workbook object from `spreadsheet_engine` for spreadsheet calculation behavior, while `ScDocument` retains styling, rendering, UI, import/export, and host-facing integration concerns.
- The same workbook object can also be used as a standalone spreadsheet engine outside Calc, with high parity to Calc's calculation behavior.

This report focuses on technical feasibility, recommended sequencing, and the validation strategy needed to surface regressions early.

## Executive Summary

The extraction is feasible, but it should be treated as a multi-stage migration rather than a single refactor.

The remaining work is not blocked by missing core engine abstractions. A large amount of pure calculation logic, planner logic, parity data, and a minimal standalone workbook runtime already exist in `spreadsheet_engine`. The hard part now is not extracting more pure functions. The hard part is separating Calc's current document runtime into:

- engine-owned calculation state and algorithms
- Calc-owned presentation, persistence, and host-side side effects

The four remaining areas have different difficulty levels:

| Area | Feasibility | Difficulty | Main reason |
| --- | --- | --- | --- |
| Main formula compiler/parser | High | Moderate to high | Generic parser core already exists, but spreadsheet-specific name/table/db/external-ref handling is still Calc-bound |
| Dependency graph and invalidation | High | High | The current graph is distributed across `ScDocument`, broadcasters/listeners, formula trees, and dirty-state orchestration |
| Recalculation orchestration | High | High | Planner seams already exist, but execution still flows through Calc-side document mutation and backend dispatch |
| Execution backend | Medium to high | Very high | The CPU interpreter is still tightly coupled to `ScDocument`, `ScFormulaCell`, `ScTokenArray`, and interpreter context; OpenCL/threading should remain adapters for a long time |

The recommended strategy is:

1. extract the compiler/token model first
2. introduce an engine-side workbook abstraction while Calc still provides storage
3. extract dependency and invalidation planning before trying to move recalc execution
4. extract the CPU execution path incrementally
5. keep OpenCL/threading as backend adapters, not core engine responsibilities, until very late
6. only move authority for workbook calculation state from Calc to the engine after the compiler, dependency, and scheduler layers are already stable

This sequencing allows incremental progress, keeps validation meaningful, and avoids a large destabilizing rewrite of `ScDocument`.

## Current State

### What `spreadsheet_engine` already owns

`spreadsheet_engine` already owns a large share of pure or low-coupling spreadsheet calculation behavior:

- extracted function logic across math, text, date/time, logical, lookup, array, reference, query, shared-formula, and formula-cell planner layers
- host abstractions for evaluation, reference resolution, text coercion, and formatting
- a standalone `InMemoryHost`
- parity TSV coverage shared with Calc
- a minimal FODS runtime:
  - sparse workbook model
  - FODS loader
  - small ODF formula parser
  - lazy evaluator with memoization and cycle detection
- raw FODS replay enabled for the `logical`, `mathematical`, `text`, `date_time`, `spreadsheet`, and `information` families

This means the engine already has enough shape to absorb more of Calc's calculation core.

### What Calc still owns

Calc still owns the full production runtime for spreadsheet calculation:

- `ScDocument` / `ScTable` / `ScColumn` storage
- the main spreadsheet compiler path via `ScCompiler`
- the current token-array representation and many reference-update hooks
- listener/broadcaster wiring
- formula trees and dirty tracking
- group calculation orchestration
- `ScFormulaCell` lifecycle and recalc execution
- the `ScInterpreter` CPU evaluator
- threaded and OpenCL backend orchestration

This is why the standalone module can replay workbook families today, but is not yet a full Calc-equivalent workbook engine.

## What Makes The Remaining Extraction Hard

### 1. Main formula compiler/parser

The generic parsing core is already relatively separable. `formula::FormulaCompiler` provides a reusable parser/tokenizer framework, and the spreadsheet-specific specialization lives in `ScCompiler`.

That is good news. It means the compiler does not need to be redesigned from scratch.

The hard part is that `ScCompiler` still reaches directly into Calc-owned services for:

- global and sheet-local named ranges
- database ranges
- table references
- external references
- add-ins
- column/row name lookup
- grammar-specific fallback parsing
- document-local configuration and localization

The compiler is therefore extractable, but only if it is turned into an engine compiler with a narrow host callback surface for those document-specific lookups.

### 2. Dependency graph and invalidation model

There is no single isolated "dependency graph" object in Calc today. The current dependency model is spread across:

- formula trees and tracked formula lists in `ScDocument`
- broadcaster/listener structures
- broadcast area slot machinery
- dirty flags on `ScFormulaCell`
- formula-group context and group execution state
- mutation-time reactions in tables and columns

Because of that, extracting the dependency graph is not a matter of moving one class. The extraction has to first identify and separate:

- pure dependency relationships
- dirty/invalidation planning
- listener registration and teardown
- document-side side effects and notifications

The dependency model is feasible to extract, but it is the highest structural risk area because it touches almost every formula-bearing mutation path.

### 3. Recalculation orchestration

This area is partly prepared already. Phase 10 extracted many planner seams from `ScFormulaCell`, especially around:

- dependency-check decisions
- group-evaluation fallback policy
- backend preflight decisions
- reference-update planning

What remains in Calc is the actual orchestration:

- formula-tree scheduling
- interpreter instantiation
- listener side effects
- document mutation
- dirty/reset transitions
- backend selection and dispatch

That makes recalculation orchestration feasible to extract, but only after the dependency/invalidation model is clearer and the evaluator has a more engine-owned execution contract.

### 4. Execution backend

The current CPU execution path still flows through `ScInterpreter`, which is deeply coupled to:

- `ScDocument`
- `ScFormulaCell`
- `ScTokenArray`
- `ScInterpreterContext`
- Calc's matrix, jump, query, search, lookup-cache, and document services

This is the most mechanically invasive extraction.

The right conclusion is not "do not extract it." The right conclusion is that execution backend extraction should be staged:

- first move token and execution-context abstractions
- then move CPU evaluator services and token walking incrementally
- keep thread-pool/OpenCL/group backends as adapters around the engine evaluator for as long as possible

Trying to move the whole execution backend at once would be high-risk and would make differential validation much harder.

## Feasibility Of The End-State

The requested end-state is feasible:

- Calc can be restructured so that `ScDocument` consumes an engine workbook object for calculation-related behavior.
- The same engine workbook can later become the standalone authoritative runtime.

However, the cleanest path is not to replace `ScDocument` storage first.

The optimal path is:

1. make the engine authoritative for more algorithms and more intermediate representations
2. introduce an engine workbook facade that can be backed by Calc storage
3. migrate compiler, dependency, and scheduling responsibilities onto that workbook facade
4. only later decide whether Calc should continue adapting its own storage or whether the engine workbook should become the authoritative calc-state container

In other words: use a sidecar and adapter model first, then converge on a true shared workbook runtime later.

## Recommended Target Architecture

The recommended architecture has five layers.

### Layer 1: Engine-owned calculation model

`spreadsheet_engine` should own the stable calculation-facing model:

- workbook
- sheet
- cell value
- formula record
- named range
- token/RPN representation
- dependency node and edge model
- dirty and invalidation state
- recalc queue and scheduling state

This does not require the engine to own every Calc storage concern.

### Layer 2: Engine service interfaces

`spreadsheet_engine` should define narrow host interfaces for the things that are still environment-specific:

- compile host
- name and table resolver
- external reference host
- backend dispatch host
- formatting and locale host
- notification and listener host
- import/export bridge points

These interfaces should be small, explicit, and versioned by behavior rather than by Calc implementation details.

### Layer 3: Engine execution services

The engine should own:

- formula compiler/parser
- token lowering
- dependency analysis
- invalidation planning
- recalc scheduling
- CPU evaluator core

These should operate over the engine calculation model and host interfaces.

### Layer 4: Calc adapter layer

Calc should provide adapters that connect the engine to the existing document world:

- workbook view over `ScDocument`
- name/table/db/external-ref adapters
- listener and broadcaster adapters
- formatting/localization adapters
- backend adapters for threading and OpenCL

This keeps Calc in control of UI and document lifecycle while moving calculation semantics into the engine.

### Layer 5: Standalone runtime

The standalone runtime should use the same engine workbook and execution services directly:

- in-memory workbook storage
- FODS loader/import path
- standalone compile/evaluate/recalc loop

That is the long-term path to high standalone parity with Calc.

## Recommended Sequencing

The sequencing below is designed to maximize incremental value and minimize destabilizing changes.

### Pass A: Extract the token and compiler host model

Goal:

- make compiler output and token storage engine-owned

Why first:

- the compiler is a natural seam
- it creates a stable contract for later evaluator and dependency work

Deliverables:

- engine-owned token/RPN representation
- engine-owned compile result object
- engine-owned compile host interfaces for:
  - names
  - table refs
  - db ranges
  - external refs
  - add-ins
  - column/row-name resolution
- Calc adapter from `ScCompiler` call sites into the engine compiler

Exit criteria:

- Calc can compile formulas through the engine compiler while preserving token parity
- standalone can compile a useful subset of workbook formulas through the same engine compiler

### Pass B: Migrate spreadsheet-specific compiler/parser logic

Goal:

- move the spreadsheet-specific compiler from `sc` into `spreadsheet_engine`

Deliverables:

- engine-owned spreadsheet compiler specialization
- Calc-side thin adapter only
- roundtrip stringification and grammar tests

Exit criteria:

- `ScCompiler` becomes a host shim or disappears entirely as an implementation owner
- compile output is differentially identical for the supported grammar modes

### Pass C: Introduce an engine workbook facade backed by Calc

Goal:

- create a workbook abstraction that the engine can reason about without taking over storage yet

Deliverables:

- engine workbook interface
- Calc-backed workbook adapter over `ScDocument`
- stable engine queries for cells, names, ranges, and sheet metadata

Exit criteria:

- compiler, evaluator, and dependency services can target the engine workbook interface rather than direct `ScDocument` access

### Pass D: Extract dependency analysis and invalidation planning

Goal:

- separate dependency logic from listener/document side effects

Deliverables:

- engine-owned dependency graph representation or dependency sidecar
- engine-owned dirty-set and invalidation planner
- engine-owned mutation planning for:
  - formula edits
  - row/column insert/delete/move
  - sheet insert/delete/move
  - name changes

Calc should still execute side effects at this stage:

- listener registration
- broadcaster updates
- formula tree mutation
- undo integration

Exit criteria:

- dependency edges and dirty sets can be computed and compared outside Calc's imperative mutation flow

### Pass E: Extract recalculation scheduler and orchestration policy

Goal:

- move recalc planning and queue policy into the engine

Deliverables:

- engine-owned dirty queue and recalc planner
- engine-owned group-evaluation preflight and fallback orchestration
- engine-owned scheduler outputs that tell the host what to execute

Calc should still own:

- the outer document mutation lifecycle
- host logging and notifications
- backend resource ownership

Exit criteria:

- Calc uses engine planner output to drive recalc rather than owning recalc policy itself

### Pass F: Incrementally extract the CPU execution backend

Goal:

- move the CPU evaluator core out of `ScInterpreter`

Deliverables:

- engine execution context
- engine token walker
- engine scalar/range evaluation core
- engine host callbacks for the remaining document-sensitive behaviors

This should be incremental. Start with the already-extracted function families and move surrounding interpreter mechanics around them.

Exit criteria:

- a meaningful subset of Calc formula execution runs through an engine CPU evaluator rather than `ScInterpreter`

### Pass G: Keep threading and OpenCL as adapters

Goal:

- avoid dragging backend-specific resource management into the engine core too early

Deliverables:

- engine backend API for batch/group execution
- Calc-side adapters for threading and OpenCL

Exit criteria:

- group execution routes through engine scheduling and engine evaluator contracts, even if Calc still owns the actual backend resources

### Pass H: Decide when to make the engine workbook authoritative

Goal:

- converge on the end-state only after the semantics are already stable

Two acceptable outcomes exist:

1. Calc continues to adapt `ScDocument` storage to an engine-owned calculation model.
2. The engine workbook becomes the primary calculation-state container and `ScDocument` becomes a richer host wrapper.

That decision should be postponed until the compiler, dependency, scheduler, and CPU evaluator have already been stabilized through shared validation.

## Detailed Implementation Checklist

### Compiler and token model

- define engine-owned token classes and compile result types
- add Calc-to-engine token conversion only as a temporary bridge
- define host interfaces for:
  - named range lookup
  - table ref lookup
  - db-range lookup
  - external refs
  - add-ins
  - column/row name resolution
  - grammar and locale settings
- implement engine spreadsheet compiler specialization
- add compile-diff tests comparing Calc and engine token streams
- switch Calc compiler call sites to the engine path behind a guard

### Workbook abstraction

- define engine workbook, sheet, and formula-record interfaces
- implement a Calc-backed workbook adapter over `ScDocument`
- wire engine compiler/evaluator inputs through that adapter
- define engine-side metadata for names, hidden rows, sheet identity, and sheet order
- add standalone workbook adapter parity tests alongside Calc-backed adapter tests

### Dependency and invalidation

- identify all dependency-bearing relationships currently updated during mutation
- define engine dependency node/edge storage
- define engine invalidation planner outputs
- shadow-build dependency edges from live Calc documents
- compare dirty-set results after representative mutations
- route selected Calc mutation paths through engine invalidation planning

### Recalc scheduler

- define engine recalc queue and scheduling state
- extract formula-tree ordering policy
- extract group preflight/fallback decisions onto engine scheduler outputs
- add shadow scheduler comparison for:
  - document recalc after edit
  - row/column mutation
  - name changes
  - shared-formula regrouping

### CPU evaluator

- define engine execution context and stack model
- move token walking and operand coercion into the engine
- move a first narrow scalar execution subset off `ScInterpreter`
- grow family coverage while comparing results against Calc
- leave remaining host/document-sensitive operations behind explicit interfaces

### Backends

- define engine batch-execution contract
- adapt Calc threading backend to that contract
- adapt Calc OpenCL backend to that contract if still needed
- keep backend resources and host logging on the Calc side

### Authority shift

- measure how much calculation state still lives only in `ScDocument`
- decide whether to:
  - keep Calc storage with engine calc services
  - or move to an authoritative engine workbook
- make this decision only after earlier stages are stable

## Validation Strategy

This extraction should be driven by differential validation at every stage.

### 1. Compiler differential validation

Add a compiler shadow mode that runs both:

- Calc compiler
- engine compiler

Compare:

- token stream shape
- token payloads
- reference lowering
- external/name/table/db resolution results
- re-serialized formula text where applicable

This should run first on:

- unit fixtures
- parity TSV cases where formulas are available
- selected raw FODS workbooks

### 2. Dependency and dirty-set differential validation

For selected document mutations, build both:

- Calc's existing dependency/dirty result
- engine planner result

Compare:

- affected cells
- dirty cells
- listener refresh candidates
- formula-group regrouping effects

Use representative mutation scenarios:

- edit formula
- edit value
- insert/delete row or column
- move/copy ranges
- insert/delete/move sheet
- rename names or sheets

### 3. Recalc scheduler differential validation

Add a recalc planner comparison that checks:

- next cells/groups scheduled
- group fallback decisions
- dependency-check failures
- recalc queue exhaustion

This should be used before the engine becomes the execution driver.

### 4. Execution differential validation

For migrated CPU evaluator slices:

- evaluate with Calc
- evaluate with engine
- compare scalar result, type, error, and text formatting-sensitive cases where relevant

Use:

- shared TSV parity
- selected `ucalc` scenarios
- raw FODS replay

### 5. Existing validation gates to preserve

The existing Calc and standalone gates should remain the foundation:

- standalone maintenance validation
- standalone parity TSV/shared-case suites
- standalone raw FODS replay suites
- Calc:
  - `CppunitTest_sc_ucalc`
  - `CppunitTest_sc_ucalc_formula2`
  - `CppunitTest_sc_ucalc_sharedformula`
  - `CppunitTest_sc_ucalc_shared_cases`
  - `CppunitTest_sc_cache_test`
  - `CppunitTest_sc_ucalc_sort`
  - spreadsheet/text/datetime/logical function suites

### 6. New validation to add specifically for this work

- compile-diff harness
- dependency-diff harness
- recalc-scheduler-diff harness
- execution-diff harness for migrated evaluator slices
- standalone workbook mutation/recalc tests once the workbook runtime grows beyond FODS replay

### 7. Performance and scale checks

Even if functional parity is correct, this extraction can regress Calc badly if scheduling or dependency updates become too expensive.

Add a small performance lane for:

- large sheet compile
- bulk formula edits
- row/column mutation with broad dependencies
- document recalc with shared formulas

This does not need to be a strict benchmark gate initially, but it should be recorded from the beginning.

## Risks

### Main risks

- extracting storage too early and destabilizing `ScDocument`
- trying to move `ScInterpreter` wholesale instead of incrementally
- conflating dependency planning with listener side effects
- prematurely baking Calc-specific concepts into the engine API
- forcing OpenCL/threading concerns into the engine core too early

### Non-goals for the next stage

The next extraction stage should not try to solve:

- styling or rendering
- UNO object model concerns
- chart or draw integration
- UI state
- macro or VBA integration
- import/export fidelity beyond what is needed for calculation behavior

Those belong to Calc as host concerns.

## Recommended First Slice

The best next concrete move is:

1. extract an engine-owned token and compile result model
2. define engine compile host interfaces
3. build a compiler differential harness
4. route a narrow but real Calc compile path through the engine compiler

This is the best first slice because it:

- creates a stable contract for the evaluator and dependency graph
- gives immediate differential validation
- avoids touching `ScDocument` storage first
- makes both Calc and standalone depend on the same compilation semantics earlier

In practice, the next milestone should be:

- engine spreadsheet compiler hosted by Calc adapters
- token parity proven on selected `ucalc` and raw FODS corpora
- no behavior change visible to users

Once that is stable, the dependency/invalidation sidecar becomes much easier to introduce.

## Recommendation

Proceed with the extraction.

The work is large, but it is technically feasible, already partially prepared by the earlier phases, and aligned with the current architecture of `spreadsheet_engine`.

The optimal route is not a bold rewrite of Calc's document core. The optimal route is a staged transfer of calculation authority:

- compiler first
- dependency and invalidation next
- scheduler after that
- CPU evaluator incrementally
- backend adapters last
- authority shift only when the shared semantics are already stable

That path gives the best chance of reaching the desired end-state while keeping regressions visible and reversible throughout the migration.

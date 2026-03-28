# Calc-Backed Workbook Facade Plan

## Purpose

This document turns the next sequencing step from
[NEXT_STEPS.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/NEXT_STEPS.md)
into an implementation-ready plan:

- introduce an engine-side workbook facade
- keep Calc as the storage authority for now
- make that facade concrete enough to host the next dependency and invalidation
  extraction work

This is intentionally a separate plan because "add a workbook abstraction"
could easily sprawl into a premature storage rewrite. That is not the goal of
this milestone.

## Executive Summary

The right next move is a **Calc-backed workbook facade**, not a new
authoritative workbook runtime.

The facade should:

1. live in `spreadsheet_engine`
2. present a stable calculation-facing workbook/sheet/cell/formula model
3. be backed by `ScDocument` and related Calc state through adapters
4. start read-only
5. become the surface that compiler, dependency, invalidation, and later
   scheduling logic target

The facade should **not**:

- replace `ScDocument` storage
- become the new persistence model
- own rendering, styling, UNO, or UI state
- rewrite `ScInterpreter`
- duplicate large workbook state into a second authoritative graph

The milestone is complete when the engine owns a stable Calc-backed workbook
facade that:

- can answer the calculation-facing queries needed by the next extraction steps
- has strong shadow validation in Calc
- is used by at least one real low-risk consumer path
- is ready to host dependency/invalidation planning work

## Problem Statement

We now have:

- an engine-owned canonical token/compiler-host model
- a compiler switchover path that lets standalone replay use the shared
  compiler by default
- a growing standalone workbook runtime for FODS replay

But the production Calc runtime still has no stable engine-owned workbook
boundary.

That means any dependency/invalidation extraction done directly today would be
tightly coupled to:

- `ScDocument`
- `ScTable`
- `ScColumn`
- `ScFormulaCell`
- listener/broadcaster machinery
- dirty-state bookkeeping that is scattered across Calc internals

If we skip the facade step, we will likely extract dependency logic once
against raw Calc storage and then extract it a second time later when we try to
move more authority into `spreadsheet_engine`.

The workbook facade prevents that double work by giving the engine a stable
runtime target while Calc still provides the actual storage.

## Milestone Definition

### What this milestone must achieve

By the end of this milestone:

- `spreadsheet_engine` owns an engine-facing workbook facade contract
- Calc provides an adapter implementation of that contract over existing
  storage
- the facade exposes enough calculation-facing data to support:
  - sheet identity and lookup
  - cell value and formula access
  - named-range access
  - formula-cell enumeration
  - shared-formula/group metadata needed by low-risk consumers
  - mutation-event input modeling for the next invalidation milestone
- at least one low-risk Calc consumer uses the facade
- the facade is validated in shadow against existing Calc behavior

### What this milestone does not require

This milestone does **not** require:

- moving storage ownership out of `ScDocument`
- replacing the standalone `WorkbookModel`
- extracting the dependency graph itself yet
- migrating all compile host lookups to the facade in one pass
- changing formula behavior
- changing file format behavior
- changing UI/rendering code paths

## Scope Boundary

### In Scope

- engine-owned workbook facade types and interfaces
- Calc-backed adapter implementation
- read/query access for calculation-facing state
- lightweight identity and view objects
- formula-cell iteration and metadata access
- mutation event descriptions for shadow invalidation work
- shadow validation and low-risk consumer adoption

### Explicitly Out Of Scope

- a new authoritative workbook storage model for Calc
- document rendering/styling/state unrelated to calculation
- filter/import/export ownership changes
- interpreter extraction
- full dependency graph ownership changes
- full recalculation scheduling changes

## Completion Criteria

The milestone is complete when all of the following are true.

### Architecture

- an engine-owned workbook facade exists under `spreadsheet_engine`
- Calc has a compat adapter that implements the facade over current storage
- facade concepts are stable enough to be used by later dependency work

### Functional Coverage

- the facade can answer the calculation-facing queries needed by the first
  dependency/invalidation shadow consumers
- formula and named-range identity are stable enough for differential tests
- shared-formula/group metadata needed by low-risk consumers is exposed

### Adoption

- at least one low-risk Calc path uses the facade directly
- at least one shadow consumer computes engine-side results from the facade and
  compares them against Calc behavior

### Validation

- dedicated Calc workbook-facade tests are green
- existing compiler/token tests stay green
- existing FODS replay lanes stay green
- adopted consumer gates stay green

## Design Principles

### 1. Calc-backed first, engine-owned contract first

The facade contract belongs to `spreadsheet_engine`.
The first implementation is backed by Calc storage.

### 2. Read-only first

The first milestone should provide read/query and identity access before it
tries to own mutation side effects.

### 3. Separate calculation state from presentation state

Only calculation-facing state should cross the facade boundary.
Formatting, rendering, UI, and persistence concerns stay in Calc.

### 4. Prefer explicit context over implicit globals

Formula grammar, base address, lookup scope, mutation cause, and scheduling
reason should be passed explicitly rather than read indirectly from scattered
Calc state.

### 5. Treat identity as a first-class problem

Dependency/invalidation work will be fragile if formula-bearing cells and named
ranges do not have stable identities.

### 6. Shadow before authority

The facade should first power shadow consumers and diff checks before it
becomes authoritative for anything stateful.

## Proposed Facade Architecture

## 1. Engine-owned facade contract

Add the contract under `spreadsheet_engine`, likely in a new cluster such as:

- `inc/spreadsheetengine/detail/workbook/WorkbookFacade.hxx`
- `inc/spreadsheetengine/detail/workbook/WorkbookFacadeTypes.hxx`
- `inc/spreadsheetengine/compat/libreoffice/WorkbookFacade.hxx`

The exact filenames can shift, but the split should be:

- engine-owned interface and types in `detail/`
- Calc adapter in `compat/libreoffice/`

## 2. Core facade types

Recommended engine-owned types:

| Type | Role |
| --- | --- |
| `WorkbookFacade` | top-level calculation-facing workbook contract |
| `SheetDescriptor` | stable sheet identity and naming data |
| `CellDescriptor` | cell address plus type/value/formula presence |
| `FormulaCellDescriptor` | formula-bearing cell view with metadata |
| `NamedRangeDescriptor` | name, scope, base, and target range metadata |
| `FormulaGroupDescriptor` | shared-formula/group metadata for low-risk consumers |
| `MutationEvent` | normalized input for later invalidation shadow work |
| `WorkbookSnapshotInfo` | version/counter metadata for differential validation |

Recommended identity carriers:

- `api::SheetId`
- `api::CellAddress`
- `FormulaCellId`
- `NamedRangeId`

`FormulaCellId` and `NamedRangeId` should be engine-owned stable identifiers for
the life of a snapshot or adapter session. They do not need to be globally
persistent.

## 3. Read/query surface for version 1

The first facade should answer these questions:

### Workbook-level

- how many sheets exist
- map sheet name <-> sheet id
- current grammar/options relevant to calculation
- workbook generation/snapshot token for debug and differential validation

### Sheet-level

- sheet name
- hidden/deleted state if relevant to calculation semantics
- sheet-local named ranges
- iteration over formula-bearing cells

### Cell-level

- does a cell exist
- scalar cell value view
- formula source view
- formula/cached-value presence
- formula cell kind:
  - ordinary
  - shared/grouped
  - matrix/array if relevant
- current dirty/recalc flags as read-only metadata when available

### Named-range level

- global vs sheet-local scope
- name text
- base address
- target address/range text or normalized reference data

### Shared-formula/group level

- group anchor
- group span or member enumeration
- shareability flags already used by low-risk consumers

## 4. Explicit non-goals for version 1 surface

Do not put these into the first facade version unless a real consumer proves
they are needed:

- style/format objects
- notes/comments
- drawing objects
- conditional formatting state
- filter/import/export metadata
- full listener graph structures
- interpreter execution context

## 5. Calc adapter shape

The Calc adapter should be sidecar-style and non-owning:

```cpp
class CalcWorkbookFacade final : public WorkbookFacade
{
public:
    explicit CalcWorkbookFacade(ScDocument& rDoc);
};
```

Recommended internal backing:

- `ScDocument&`
- lightweight projections over `ScTable`, `ScColumn`, and `ScFormulaCell`
- explicit snapshot helpers for iteration-heavy operations

The adapter should not:

- duplicate the full workbook into a second mutable model
- cache long-lived raw pointers without explicit lifetime rules

## 6. Snapshot and lifetime model

The first facade should support a lightweight snapshot model:

- construct facade over `ScDocument`
- expose a snapshot token or generation counter
- views are valid only for the snapshot lifetime or until explicit refresh

This avoids silent use-after-mutation assumptions.

## 7. Mutation event model

The facade milestone should include normalized mutation events even before the
engine owns invalidation.

Recommended event families:

- set scalar cell value
- set formula
- clear cell/range
- insert/delete rows
- insert/delete columns
- move/copy range
- rename sheet
- add/remove/rename named range

These events should describe:

- operation kind
- affected addresses/ranges
- before/after sheet identity when relevant
- whether operation is copy vs move

The event model is the handoff point into dependency/invalidation extraction.

## Consumer Map

The best first consumers are the low-risk ones that prove the facade shape
without moving authority too early.

| Consumer | Use facade in this milestone? | Why |
| --- | --- | --- |
| compile-diff representative formula corpus | yes | already stable diff harness exists |
| formula-cell enumeration for shadow validation | yes | needed for later dependency shadow work |
| shared-formula/group comparison support | yes | low-risk metadata consumer |
| compile host itself | maybe partially | only after facade shape settles |
| dirty/invalidation planning | shadow next milestone | main reason this facade exists |
| scheduler/execution backend | no | too early |

## Proposed Phased Plan

### Phase 0: Contract And Corpus Freeze

Goal:

- define the facade contract boundary before code spreads

Tasks:

- inventory the first intended consumers
- define the minimum v1 query surface
- define identity and snapshot rules
- define the mutation event vocabulary
- freeze the first validation corpus

Exit criteria:

- facade contract and milestone scope are documented
- first validation lanes are named

### Phase 1: Engine-Owned Types And Empty Contract

Goal:

- land the facade types and interfaces with no Calc behavior yet

Tasks:

- add engine-owned facade headers
- add core descriptor/identity types
- add mutation event types
- add standalone unit tests for type semantics and invariants

Exit criteria:

- the contract exists in `spreadsheet_engine`
- type-level tests are green

### Phase 2: Calc-Backed Workbook/Sheet/Cell Read Surface

Goal:

- make the facade useful for read-only calculation queries

Tasks:

- add `CalcWorkbookFacade`
- implement:
  - sheet enumeration
  - sheet lookup
  - cell existence/value/formula queries
  - formula-cell iteration
- add Calc tests for:
  - sheet naming/lookup
  - scalar/formula cell reads
  - formula-cell enumeration stability

Exit criteria:

- facade can enumerate formula-bearing cells in real Calc documents
- no behavior regression in existing compiler/token lanes

### Phase 3: Named Ranges And Formula Metadata

Goal:

- expose the minimum metadata needed by compiler-adjacent and dependency
  shadow consumers

Tasks:

- add named-range enumeration and lookup
- expose formula-cell metadata:
  - formula source
  - shared/group flags
  - matrix flags if needed
  - dirty/recalc flags as read-only metadata where available
- add shared-formula/group descriptor support
- add Calc tests for scope, shadowing, and group metadata

Exit criteria:

- facade can represent the key formula-bearing metadata needed by low-risk
  consumers
- shared-formula/group metadata is stable enough for diff checks

### Phase 4: First Real Consumers

Goal:

- prove the facade is useful on real code paths without moving authority

Tasks:

- add a Calc shadow consumer that enumerates formula cells through the facade
- add a low-risk direct consumer:
  - shared-formula comparison
  - formula-cell enumeration helper
  - representative compile-diff corpus feeder
- keep legacy direct `ScDocument` access as fallback where appropriate

Exit criteria:

- at least one real path uses the facade
- shadow validation is green

### Phase 5: Mutation Event Modeling And Invalidation Inputs

Goal:

- make the facade ready for dependency/invalidation extraction

Tasks:

- add normalized mutation event types
- add Calc-side translation from representative document operations into those
  events
- add tests for:
  - row/column insert/delete
  - set formula/value
  - sheet rename
  - named-range edits

Exit criteria:

- mutation events exist and are tested
- dependency/invalidation work can target the facade instead of raw Calc
  storage

### Phase 6: Dependency/Invalidation Handoff

Goal:

- close this milestone by handing off to the next one cleanly

Tasks:

- document the stable facade subset
- record deferred surface areas
- wire the first dependency shadow plan to consume the facade
- update `NEXT_STEPS.md` / status docs if needed

Exit criteria:

- the facade milestone is complete
- the next dependency/invalidation milestone can begin against the facade

## Validation Strategy

Validation should happen at four levels.

### 1. Type-Level Validation

- standalone unit tests for:
  - ids
  - descriptors
  - mutation events
  - snapshot invariants

### 2. Calc Adapter Validation

- new Calc tests for:
  - sheet lookup
  - formula-cell enumeration
  - named-range scope/shadowing
  - shared-formula metadata

### 3. Shadow Consumer Validation

- representative corpus fed through facade-driven shadow helpers
- results compared against existing Calc behavior

### 4. Safety Validation

Existing lanes that should remain green:

- `CppunitTest_sc_ucalc_token_bridge`
- `CppunitTest_sc_ucalc_compile_host`
- `CppunitTest_sc_ucalc_shadow_compiler`
- `CppunitTest_sc_ucalc_compile_diff`
- `CppunitTest_sc_ucalc_sharedformula`
- `CppunitTest_sc_ucalc`
- standalone `ctest`

## Recommended First Implementation Slice

The best first slice is:

1. add engine-owned facade types and interfaces
2. add a Calc-backed read-only adapter for:
   - workbook
   - sheet lookup
   - formula-cell enumeration
3. add a dedicated Calc test lane, likely something like
   `ucalc_workbook_facade`
4. feed one existing low-risk helper from facade-driven formula-cell
   enumeration in shadow mode

That slice is small enough to validate tightly and puts a real engine-owned
runtime boundary in place before dependency extraction starts.

## Deferred Areas

These should stay deferred until the facade proves itself:

- conditional formatting state
- validation rules
- pivot/runtime caches
- listener graph ownership
- interpreter-specific runtime context
- rendering/import/export metadata

## Tracking Checklist

### Phase 0

- [ ] freeze the v1 facade contract
- [ ] freeze the first consumer set
- [ ] freeze the first validation lanes

### Phase 1

- [ ] add engine-owned facade headers and types
- [ ] add type-level standalone tests

### Phase 2

- [ ] add Calc-backed workbook facade
- [ ] add sheet lookup support
- [ ] add cell and formula-cell read support
- [ ] add Calc tests for the read surface

### Phase 3

- [ ] add named-range enumeration and lookup
- [ ] add formula metadata descriptors
- [ ] add shared-formula/group descriptors
- [ ] add Calc tests for range-name and group metadata

### Phase 4

- [ ] add first facade-driven shadow consumer
- [ ] adopt first low-risk direct consumer
- [ ] keep fallback path green

### Phase 5

- [ ] add mutation event types
- [ ] add Calc translation for representative mutation kinds
- [ ] add mutation-event validation tests

### Phase 6

- [ ] document the stable facade subset
- [ ] record deferred surfaces
- [ ] wire the first dependency/invalidation plan to consume the facade
- [ ] close out the milestone

## Current Status

Not started.


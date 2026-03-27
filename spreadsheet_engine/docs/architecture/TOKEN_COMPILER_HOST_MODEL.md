# Token And Compiler Host Model Plan

## Purpose

This document turns the first major item from [NEXT_STEPS.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/NEXT_STEPS.md) into an implementation-ready plan:

- extract an engine-owned token model
- extract the compiler host model
- make that combination robust enough to guide real implementation work

This is intentionally a separate plan because `ScTokenArray` is not a narrow compiler detail. It is one of Calc's most pervasive types and currently sits at the intersection of:

- compiler output
- interpreter input
- reference update
- shared-formula comparison
- group evaluation
- persistence/import/export
- name storage
- validation and filter paths

If this work is treated as a thin compiler refactor, the scope will be underestimated.

## Executive Summary

The right first milestone is **not** "remove `ScTokenArray` from Calc."

The right first milestone is:

1. define an engine-owned canonical token model
2. define engine-owned compiler host interfaces
3. build a lossless bridge between the canonical token model and `ScTokenArray`
4. run the engine compiler in shadow against Calc and prove token parity
5. migrate a small set of low-risk consumers first

This gives `spreadsheet_engine` an authoritative token and compiler contract without forcing an immediate rewrite of every current `ScTokenArray` consumer.

That distinction matters. A full consumer migration is too large for the first milestone. A canonical token model plus bridge is achievable and creates the platform needed for later extraction of dependency graph, recalc orchestration, and execution backend work.

## Problem Statement

`ScTokenArray` currently bundles several different responsibilities into one pervasive type:

- token storage
- token construction
- equality and hashing
- position-dependent reference interpretation
- reference update and mutation
- vectorization/backend flags
- stringification helpers
- document-specific behaviors
- interoperability with many Calc-specific consumers

That makes direct extraction dangerous for two reasons:

1. there is no single "replacement" that should inherit all of those responsibilities
2. many consumers do not need the same abstraction level

The milestone therefore has to separate the concerns that `ScTokenArray` currently conflates.

## Milestone Definition

### What this milestone must achieve

By the end of this milestone:

- `spreadsheet_engine` owns the canonical spreadsheet token model
- `spreadsheet_engine` owns the compiler host interfaces required to compile spreadsheet formulas
- Calc can compile formulas through the engine compiler into the canonical token model
- a lossless bridge exists between the canonical token model and `ScTokenArray`
- token parity is proven through differential validation on meaningful corpora
- a first set of low-risk consumers uses the canonical token model directly

### What this milestone does **not** require

This milestone does **not** require:

- removing `ScTokenArray` from every Calc consumer
- replacing `ScInterpreter`
- migrating all persistence/filter code to the new token model
- eliminating Calc-side reference-update code immediately
- changing the on-disk file format
- changing formula behavior

Those are later milestones.

## Completion Criteria

The milestone is complete when all of the following are true:

### Core architecture

- an engine-owned `TokenArray` equivalent exists in `spreadsheet_engine`
- an engine-owned compile result object exists
- engine-owned compiler host interfaces exist for all spreadsheet-specific lookups needed by `ScCompiler`

### Functional behavior

- formulas compiled by Calc through the engine compiler produce token output equivalent to the current Calc compiler
- token arrays can be losslessly converted to and from `ScTokenArray`
- the canonical token model supports the token classes actually emitted by Calc's spreadsheet compiler for the chosen validation corpus

### Adoption

- Calc compiler call sites can execute through the engine compiler path behind a guard or shadow mode
- at least the following behaviors are native on the canonical model:
  - hashing
  - equality
  - basic stringification support
  - low-coupling shared-formula comparison or grouping support

### Validation

- compile-diff validation is green on:
  - selected `ucalc` corpora
  - parity/shared-case formulas
  - the current raw FODS replay families used by standalone
- bridge roundtrip tests are green

## Recommended Scope Boundary

The biggest scope-control decision is this:

**Treat the canonical token model as the new source of truth, but treat `ScTokenArray` as a compatibility adapter for a while.**

That lets us complete the milestone without blocking on:

- every persistence consumer
- every filter path
- every UNO token API path
- every execution backend detail

The plan below is built around that boundary.

## Design Principles

### 1. Do not recreate `ScTokenArray` as a monolith

The engine-owned replacement should not be a line-by-line clone of `ScTokenArray`.

Instead, split responsibilities into separate components:

- token storage
- token metadata
- reference-update services
- stringification services
- bridge/adaptation services

### 2. Prefer composition over inheritance

Even if some implementation details continue to rely on `formula::FormulaToken` or `formula::FormulaTokenArray` concepts, the engine surface should be defined by engine-owned types and services.

### 3. Keep Calc-specific side effects out of the token model

The canonical token model should not directly depend on:

- `ScDocument`
- listener structures
- formula-tree mutation
- backend execution state

Position-sensitive operations should be handled by services that accept context explicitly.

### 4. Make lossless bridging a first-class requirement

Because Calc has many pervasive consumers, the bridge is not an afterthought. It is part of the milestone.

### 5. Validate by differential comparison from day one

Compiler migration without a differential harness will be too risky.

## Consumer Inventory By Risk And Target State

This table defines how each consumer class should be handled in this milestone.

| Consumer area | Current state | Milestone target |
| --- | --- | --- |
| Compiler output | `ScCompiler -> ScTokenArray` | Native engine compile result |
| Hash/equality | `ScTokenArray` methods | Native engine token services |
| Shared-formula comparison | `ScTokenArray`-driven | Native or bridged early |
| Basic stringification | `ScTokenArray` / compiler helpers | Native or bridged early |
| Reference update | `ScTokenArray` methods | Bridge first, then selectively migrate |
| Interpreter input | `ScTokenArray` | Bridge only in this milestone |
| Group evaluation/vector state | `ScTokenArray` flags | Preserve via sidecar/bridge |
| Persistence/import/export | `ScTokenArray` pervasive | Bridge only |
| UNO token APIs | `ScTokenArray` | Bridge only |
| Named range storage | `ScTokenArray` | Bridge initially; can adopt native later |

This is the key scope-control mechanism.

## Proposed Architecture

## 1. Canonical engine token model

Add an engine-owned token model in `spreadsheet_engine`, likely under `inc/spreadsheetengine/detail/` first.

Recommended pieces:

- `TokenKind`
- `TokenPayload`
- `Token`
- `TokenBuffer`
- `CompiledFormula`

Recommended characteristics:

- stable iteration order
- immutable or mostly-immutable after construction
- explicit token categories for:
  - opcodes
  - scalars
  - strings
  - errors
  - single refs
  - double refs
  - names
  - db/table refs
  - external refs
  - matrices/arrays
  - jump tokens
  - whitespace/XML placeholders if needed for lossless bridging

The canonical model should preserve the information required for:

- compilation
- string roundtrip
- reference update
- shared-formula equivalence
- persistence bridging

### 2. Token services split out from the model

Do not store algorithmic behavior inside the token container where it can be avoided.

Recommended services:

- `TokenHash`
- `TokenEquality`
- `TokenStringifier`
- `TokenReferenceUpdate`
- `TokenBridgeToCalc`
- `TokenBridgeFromCalc`

### 3. Compiler host model

Add engine-owned host interfaces for spreadsheet-specific compile-time lookups.

Recommended split:

- `CompileHost`
- `NameResolver`
- `TableRefResolver`
- `DbRangeResolver`
- `ExternalRefResolver`
- `AddInResolver`
- `ColRowNameResolver`
- `GrammarSettings`
- `LocaleAndCharClassHost`

The exact type shape can vary, but the host model must be designed around behavioral contracts, not Calc classes.

### 4. Calc adapter layer

Create a Calc adapter package that implements the compile host model over:

- `ScDocument`
- `ScRangeName`
- `ScDBCollection`
- `ScExternalRefManager`
- `ScFormulaParserPool`
- Calc grammar/localization helpers

This is where Calc-specific lookup logic should live once the compiler is moved.

### 5. Compatibility bridge

Create an explicit bridge between:

- engine canonical token model
- `ScTokenArray`

This bridge must be lossless for the milestone corpus.

It should support:

- native -> `ScTokenArray`
- `ScTokenArray` -> native
- roundtrip tests

## Phased Plan

### Phase 0: Inventory And Guardrails

Goal:

- establish the real scope before new types are introduced

Tasks:

- inventory the token kinds emitted by Calc on a representative corpus
- inventory the `ScTokenArray` methods actually used by:
  - compiler
  - interpreter
  - shared formula
  - persistence/import/export
  - filters
  - named ranges
- classify consumers into:
  - must-support now
  - bridge-only this milestone
  - defer
- define the milestone corpus used for parity

Artifacts:

- token kind coverage matrix
- consumer matrix with owner and migration target
- validation corpus list

Exit criteria:

- no ambiguity about what the first milestone does and does not need to support

### Phase 1: Canonical Token Schema

Goal:

- define the engine token types without switching any live compiler path yet

Tasks:

- define `TokenKind`, `Token`, `TokenBuffer`, and `CompiledFormula`
- decide what metadata lives:
  - inline in tokens
  - in sidecars
  - in compile result wrapper
- include fields needed for lossless bridging, even if not all are used natively yet
- define immutable iteration and ownership rules

Design rule:

- no direct `ScDocument` dependency
- no persistence or interpreter behavior embedded in the storage type

Exit criteria:

- canonical token schema is stable enough for bridging and diffing

### Phase 2: Bridge Scaffold

Goal:

- make the new model interoperable before any big consumer switch

Tasks:

- implement `ScTokenArray -> EngineTokenBuffer`
- implement `EngineTokenBuffer -> ScTokenArray`
- add bridge roundtrip tests
- add token-level diff tooling

Important requirement:

- bridge fidelity must include the odd cases, not just common formula tokens:
  - external refs
  - names
  - table refs
  - jumps
  - matrices
  - whitespace/XML placeholders if needed

Exit criteria:

- roundtripping through the bridge does not change token semantics on the milestone corpus

### Phase 3: Compiler Host Interfaces

Goal:

- define the host lookup model before moving spreadsheet-specific compiler logic

Tasks:

- design the compile host interfaces
- separate mandatory compiler dependencies from optional fallback hooks
- specify how failures are reported
- specify how grammar and locale are supplied
- implement Calc adapters over current services

Important design question:

- host interfaces should return engine-oriented lookup results, not raw Calc objects

Exit criteria:

- spreadsheet-specific lookups can be expressed without direct compiler dependence on `ScDocument`

### Phase 4: Engine Compiler Skeleton

Goal:

- make the engine capable of compiling formulas into the canonical token model in shadow mode

Tasks:

- move or adapt spreadsheet-specific compiler logic into `spreadsheet_engine`
- preserve the current generic parsing behavior from `formula::FormulaCompiler`
- emit canonical tokens instead of `ScTokenArray`
- keep Calc compiler path intact for comparison

Recommended approach:

- start with a shadow compile mode that runs engine compilation alongside the current Calc path
- avoid replacing live production output immediately

Exit criteria:

- engine compiler can compile representative formulas in Calc and standalone contexts

### Phase 5: Differential Validation Harness

Goal:

- prove correctness before any consumer migration

Tasks:

- add compile-diff tests:
  - opcode sequence
  - reference payloads
  - name/table/db/external-ref lowering
  - error/token placement
- add string roundtrip checks where meaningful
- add bridge parity checks

Corpora to use:

- shared parity formulas
- selected `ucalc` scenarios
- named range and shared-formula cases
- raw FODS families already enabled in standalone

Exit criteria:

- the engine compiler is green in shadow mode on the agreed corpus

### Phase 6: First Native Consumers

Goal:

- adopt the canonical model in low-risk places first

Recommended first consumers:

- token hash
- token equality
- shared-formula equivalence/grouping helpers
- basic stringification support

Why these first:

- they are lower-risk than interpreter execution
- they provide immediate proof the canonical model is useful beyond compilation

Exit criteria:

- these consumer paths can operate directly on canonical tokens in Calc or standalone

### Phase 7: Bridge-Based Calc Adoption

Goal:

- make the engine compiler the effective owner of compile output while leaving broad consumers bridged

Tasks:

- route selected Calc compile call sites through the engine compiler
- convert compiled output back to `ScTokenArray` for consumers that are not migrated yet
- keep differential mode available for safety

This is the moment when the canonical token model becomes operationally meaningful without requiring full consumer migration.

Exit criteria:

- selected real Calc compile flows use the engine compiler path successfully

### Phase 8: Milestone Closeout

Goal:

- finish the milestone without letting it sprawl into interpreter or dependency-graph work

Tasks:

- document remaining `ScTokenArray` bridge-only consumers
- freeze the canonical token and compile-host contracts
- identify the next consumers for later milestones:
  - reference update
  - named-range storage
  - execution backend

Exit criteria:

- engine token model is canonical
- compile host model is stable
- Calc can use the engine compiler with a compatibility bridge
- parity validation is green

## Recommended First Consumer Slice

The best first consumer slice after the compiler itself is:

1. hashing
2. equality
3. shared-formula comparison

These consumers are attractive because they:

- do not require full interpreter migration
- are heavily token-centric
- surface canonical-model correctness quickly
- reduce dependence on `ScTokenArray` utility methods

## Detailed Validation Plan

### 1. Token corpus discovery

Before switching anything live, record which token kinds appear in:

- `ucalc`
- named range definitions
- shared formulas
- the currently enabled standalone FODS families

This prevents "unknown unknowns" in bridge fidelity.

### 2. Bridge roundtrip tests

Required checks:

- `ScTokenArray -> Engine -> ScTokenArray`
- `Engine -> ScTokenArray -> Engine`

Compare:

- token kinds
- payload values
- reference payloads
- flags and metadata needed for selected consumers

### 3. Compile differential tests

Run current Calc compiler and engine compiler side by side and compare:

- token stream
- compile errors
- stringification where supported
- canonical hash/equality behavior

### 4. Consumer shadow tests

For early migrated consumers, run:

- old Calc path
- canonical token path

Compare results before changing ownership.

### 5. Integration gates

Keep existing broad gates in place:

- standalone maintenance validation
- standalone parity/shared-case suites
- standalone raw FODS replay
- Calc:
  - `CppunitTest_sc_ucalc`
  - `CppunitTest_sc_ucalc_formula2`
  - `CppunitTest_sc_ucalc_sharedformula`
  - `CppunitTest_sc_ucalc_shared_cases`
  - `CppunitTest_sc_cache_test`
  - `CppunitTest_sc_ucalc_sort`

Add targeted compiler/token gates on top.

## Risks And Mitigations

### Risk: trying to migrate every consumer in the first milestone

Mitigation:

- make the bridge a first-class deliverable
- define explicit bridge-only consumers

### Risk: designing a token model that is too minimal

Mitigation:

- require lossless bridge fidelity on the milestone corpus
- include rarely used token forms early

### Risk: designing a token model that simply clones Calc's monolith

Mitigation:

- split storage from services
- keep document-dependent behavior out of the storage type

### Risk: hidden dependency on `ScDocument`

Mitigation:

- express all spreadsheet-specific compiler lookups through host interfaces
- ban direct `ScDocument` access in engine compiler code

### Risk: not knowing when the milestone is done

Mitigation:

- use the completion criteria in this document
- do not redefine the milestone as "remove `ScTokenArray` everywhere"

## Non-Goals

This milestone should not absorb:

- dependency graph extraction
- recalc scheduler extraction
- full reference-update migration
- `ScInterpreter` replacement
- persistence-format redesign
- OpenCL or threading backend redesign

Those are later programs that will build on this one.

## Recommended Work Breakdown

If implementation begins immediately, the recommended order is:

1. Phase 0: inventory and guardrails
2. Phase 1: canonical token schema
3. Phase 2: bridge scaffold
4. Phase 3: compiler host interfaces
5. Phase 4: engine compiler skeleton in shadow mode
6. Phase 5: differential validation harness
7. Phase 6: first native consumers
8. Phase 7: bridge-based Calc adoption
9. Phase 8: milestone closeout

## Execution Tracker

This section is the working task list for the implementation effort.

- [x] Phase 0 kickoff: confirm milestone boundary and treat `ScTokenArray` replacement as a staged migration program rather than a full first-pass consumer rewrite
- [x] Phase 0 initial inventory snapshot: identify compiler-emitted token repertoire and main consumer categories
- [ ] Phase 0 closeout: produce the fuller token-kind coverage matrix from the milestone validation corpus
- [x] Phase 1 start: add initial canonical token schema scaffolding in `spreadsheet_engine`
- [x] Phase 1 start: add initial compiler host interface scaffolding in `spreadsheet_engine`
- [x] Phase 1 start: add first standalone schema/interface validation tests
- [x] Phase 1 expansion: add compile-result metadata for XML formula source and split the host surface into narrower resolver interfaces
- [x] Phase 1 substantial-complete checkpoint: canonical compiler-emitted token schema, compile-result container, and compile-host interface bundle exist with focused standalone validation
- [x] Phase 2 start: extend the canonical model with bridge-relevant metadata (`code error`, recalc bits, hyperlink/range-name/shareable flags, jump param-class payload)
- [x] Phase 2 start: add a Calc-side `ScTokenArray <-> CompiledFormula` bridge scaffold for the core compiler-emitted token repertoire
- [x] Phase 2 start: add focused Calc roundtrip validation in `CppunitTest_sc_ucalc_token_bridge`
- [x] Phase 2 substantial-complete checkpoint: bridge scaffolding exists for the main compiler-emitted token categories with focused standalone and Calc validation
- [x] Phase 3: implement Calc compile-host adapters
- [x] Phase 4: add engine compiler skeleton in shadow mode
- [x] Phase 5: add compile-diff validation harness
- [ ] Phase 6: migrate first low-risk native consumers
- [x] Phase 6 first slice: route shared-formula token comparison through canonical engine token services with Calc fallback for unsupported bridge cases
- [x] Phase 6 second slice: route `ScTokenArray::EqualTokens()` through canonical lexical token equality with Calc fallback for unsupported bridge cases
- [x] Phase 6 third slice: route `ScTokenArray::GenHash()` through canonical lexical hashing with Calc fallback for unsupported bridge cases
- [x] Phase 6 fourth slice: add engine-owned diagnostic token stringification and consume it from Calc compile-diff diagnostics
- [x] Phase 6 substantial-complete checkpoint: hashing, equality, shared-formula comparison, and basic diagnostic stringification now operate on canonical tokens in live Calc or standalone paths
- [ ] Phase 7: bridge-based Calc compiler adoption
- [ ] Phase 8: milestone closeout

## Phase 0 Snapshot

The first inventory pass confirms that the milestone should target **compiler-emitted tokens first**, not all interpreter-only token carriers.

### Compiler-emitted token repertoire

The current compiler-facing repertoire is effectively the `ScRawToken` repertoire in `sc/inc/compiler.hxx`, which is much smaller and more tractable than the full universe of `FormulaToken` subclasses. The key compiler-emitted categories are:

- plain opcode / separator / missing tokens
- byte-param tokens
- doubles
- strings and string names
- single and double references
- names and db ranges
- external single refs, double refs, and names
- table refs and column/row-name refs
- matrix literals
- jump payloads
- error constants
- whitespace / XML placeholder preservation

### Main `ScTokenArray` consumer categories

The current first-pass consumer categories visible in `sc/` fall into these groups:

- compiler output and stringification
- formula cell storage and document mutation (`ScFormulaCell`, `ScDocument`, named ranges)
- interpreter and group execution input
- reference update and copy/move/tab adjustment
- shared-formula comparison and grouping
- persistence / import / export / filter pipelines
- UNO token exposure and validation formulas

### Milestone corpus for differential validation

The planned validation corpus for this milestone remains:

- `CppunitTest_sc_ucalc`
- `CppunitTest_sc_ucalc_formula2`
- `CppunitTest_sc_ucalc_sharedformula`
- `CppunitTest_sc_ucalc_shared_cases`
- `CppunitTest_sc_cache_test`
- `CppunitTest_sc_ucalc_sort`
- standalone parity/shared-case formulas
- standalone raw FODS replay families already enabled today

That corpus is broad enough to support the bridge-and-shadow-compiler milestone without requiring immediate consumer migration.

## Current Phase 5 Status

Phase 5 is now substantially complete.

The current checkpoint now includes:

- a canonical compiler-emitted token schema
- a compile-result container with vector/backend metadata and XML formula source preservation
- native token hashing and equality support
- a split compiler-host interface bundle built around narrower resolver roles
- a LibreOffice bridge scaffold in `spreadsheetengine/compat/libreoffice/TokenBridge.hxx`
- Calc-backed compile-host adapters in `spreadsheetengine/compat/libreoffice/CompileHost.hxx`
- an engine-owned compile request / status layer in `spreadsheetengine/detail/CompilerPipeline.hxx`
- a Calc-backed shadow compiler wrapper in `spreadsheetengine/compat/libreoffice/ShadowCompiler.hxx`
- a Calc-side compile-diff harness in `spreadsheetengine/compat/libreoffice/CompilerDiff.hxx`
- bridge roundtrip coverage in `CppunitTest_sc_ucalc_token_bridge`
- focused Calc compile-host validation in `CppunitTest_sc_ucalc_compile_host`
- focused Calc shadow-compiler validation in `CppunitTest_sc_ucalc_shadow_compiler`
- focused Calc compile-diff validation in `CppunitTest_sc_ucalc_compile_diff`
- focused validation covering:
  - standalone schema/hash/host tests
  - standalone compile-pipeline shape tests
  - Calc import/export roundtrips for mixed core token arrays
  - Calc document-backed name / db-range / table-ref / col-row-name / external-name lookup checks
  - Calc shadow compilation into the canonical token model for:
    - range names
    - database ranges
    - table refs
    - col/row names
    - external names
  - Calc synthetic compile-diff checks comparing:
    - canonical token streams
    - compile-status metadata
    - bridge roundtrips back to `ScTokenArray`
    - whitespace-sensitive and array-literal formulas
    - db-range and table-reference lowering
  - XML placeholder bridge handling via `AssignXMLString()`
  - structured-reference bridge fidelity for `ScTableRefToken::Item` values
  - bridge-semantic matrix equality for roundtrip comparisons, instead of pointer-identity `ScMatrixToken` equality

The current bridge scaffold covers the main compiler-emitted token categories that matter for the first milestone checkpoint:

- plain opcode / separator tokens
- missing tokens
- byte-param tokens, including in-force-array param class
- doubles, strings, and string-name tokens
- single refs, double refs, range names, db ranges, and col/row-name refs
- external single refs, double refs, and names
- matrix literals containing numeric, string, and error elements
- table refs
- jump tokens
- error constants
- whitespace tokens
- pure XML placeholder arrays via `AssignXMLString()`

Phase 5 still compiles through the legacy Calc compiler backend in shadow mode and then lowers the resulting `ScTokenArray` through the engine bridge. That remains intentional at this stage: Calc can now execute a compiler-shaped request against an engine-owned compile request / status contract, lower the result into canonical tokens, and compare those artifacts in a stable diff harness before any live production ownership changes.

The current compile-diff harness is deliberately **synthetic-first**. The green validated corpus today is:

- standalone compiler / token-model tests
- Calc token-bridge roundtrip cases
- Calc compile-host lookup cases
- Calc shadow-compiler cases
- Calc compile-diff cases covering:
  - plain arithmetic
  - whitespace-preserving function syntax
  - matrix / array literals
  - named database ranges
  - Excel-style structured table references

That is enough to make Phase 5 substantially complete because the project now has:

- an engine-owned canonical token model
- a lossless bridge for the primary compiler-emitted repertoire
- Calc-backed host adapters
- shadow compilation into canonical artifacts
- a stable differential harness that can compare legacy-vs-shadow compile output end to end

What Phase 5 still does **not** include is also intentional:

- `svExternal` / plain external-string tokens
- interpreter-only carriers such as jump-matrix, ref-list, matrix-cell, hybrid-cell, and vector-ref tokens
- broader special-opcode reference variants beyond the initial bridge path
- explicit export-side preservation of vector/OpenCL/threading flags beyond whatever Calc recomputes while rebuilding the token array
- corpus-scale compile-diff execution across the full milestone validation set
- raw workbook / FODS compile-diff replay
- stable compile-diff coverage yet for the more fragile corners of:
  - col/row-name lookup formulas
  - external-name formulas
  - XML namespaced formula placeholders
- native compiler lowering that no longer depends on `ScCompiler`

Those are the closeout items for later Phase 5 expansion or the first work inside Phase 6.

## Current Phase 6 Status

Phase 6 is now substantially complete.

The milestone now has the recommended first native consumers operating on the canonical token model:

- shared-formula equivalence in `ScFormulaCell::CompareByTokenArray()`
- lexical equality in `ScTokenArray::EqualTokens()`
- lexical hashing in `ScTokenArray::GenHash()`
- basic diagnostic stringification in Calc compile-diff reporting

The adoption pattern is intentionally conservative:

- Calc imports legacy lexical and RPN streams through the engine bridge
- canonical token services perform hashing / equality / relative-reference classification
- Calc keeps the old handwritten implementations as fallbacks when import hits an unsupported bridge corner
- surrounding Calc consumers still receive the same compare states, hashes, and diagnostics they expect

That means the milestone has now crossed from pure infrastructure into live low-risk consumer ownership without touching interpreter execution, persistence, or reference-update ownership.

The current Phase 6 checkpoint specifically covers:

- engine-owned shared-formula token hashing for lexical token streams
- engine-owned shared-formula token comparison for lexical and RPN token streams
- engine-owned canonical lexical-token equality reused by `ScTokenArray::EqualTokens()`
- engine-owned canonical lexical hashing reused by `ScTokenArray::GenHash()`
- engine-owned diagnostic token / compiled-formula stringification reused by Calc compile-diff diagnostics
- Calc adapter plumbing in:
  - `spreadsheetengine/compat/libreoffice/SharedFormula.hxx`
  - `spreadsheetengine/compat/libreoffice/TokenBridge.hxx`
  - `spreadsheetengine/compat/libreoffice/CompilerDiff.hxx`
- live adoption in:
  - `ScFormulaCell::CompareByTokenArray()`
  - `ScTokenArray::EqualTokens()`
  - `ScTokenArray::GenHash()`
- focused validation for:
  - standalone compiler / token-host / shared-formula tests
  - full standalone suite including raw FODS replay
  - Calc shared-formula grouping cases
  - Calc token-bridge equality / hashing regressions
  - Calc compile-diff diagnostics staying buildable and green

What Phase 6 still does **not** include yet:

- eliminating fallback-to-legacy behavior for every unsupported bridge corner
- broader production consumers beyond hashing / equality / shared-formula helpers
- user-facing or persistence-grade formula pretty-printing beyond basic diagnostics
- wider token-service adoption in reference-update, named-range storage, or interpreter-facing paths

That is enough to make Phase 6 substantially complete. The remaining work is closeout-grade expansion or later-phase adoption, not missing milestone scaffolding.

## Recommendation

Proceed with this milestone, but frame it correctly:

- it is a token-model and compiler-host extraction program
- it is not yet a full consumer migration program

If the milestone is kept to "canonical model + host model + bridge + validated compiler adoption," it is large but tractable.

If it is allowed to expand into "remove `ScTokenArray` from Calc," it will become too large, too risky, and too hard to validate incrementally.

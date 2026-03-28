# Compiler Switchover Plan

## Purpose

This document turns the next compiler milestone into an execution-ready plan:

- switch standalone FODS replay from the bespoke parser path to the shared
  production compiler path
- do that in a way that also advances Calc toward the same engine-owned
  compiler
- keep validation strong enough that parity regressions are surfaced early

This plan builds directly on:

- [TOKEN_COMPILER_HOST_MODEL.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/TOKEN_COMPILER_HOST_MODEL.md)
- [BASIC_FODS_SUPPORT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/BASIC_FODS_SUPPORT.md)
- [NEXT_STEPS.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/NEXT_STEPS.md)

## Executive Summary

The switchover should not start by deleting the standalone `OdfFormulaParser`
or by forcing Calc to use a half-native compiler.

The right sequence is:

1. add a standalone workbook-backed compile host
2. make the engine compiler native for a narrow FODS-safe subset
3. keep standalone replay on dual paths for a while:
   - current FODS parser/evaluator
   - shared compiler plus token execution adapter
4. switch families one by one behind strong diff validation
5. only then retire the bespoke standalone parser from the hot path

That gives the project a real shared production compiler without creating one
large un-debuggable jump.

## Problem Statement

Today the two worlds are still split:

- Calc production formulas are compiled by Calc
- standalone FODS replay parses formulas through:
  - `spreadsheetengine/detail/OdfFormulaParser.hxx`
  - `spreadsheetengine/detail/FodsEvaluator.hxx`

The token/compiler-host milestone created the platform needed for convergence:

- canonical token model
- compile-host interfaces
- Calc bridge
- shadow compiler and compile-diff harness
- first real Calc compile adopters

But that milestone did **not** yet make the engine compiler authoritative, and
the standalone FODS runtime still does not consume compiled tokens from the
shared path.

## Target End-State

The target end-state for this program is:

- the authoritative spreadsheet formula compiler lives in
  `spreadsheet_engine`
- Calc compiles formulas through that engine compiler using Calc-backed host
  adapters
- standalone workbook/FODS replay compiles formulas through the same engine
  compiler using workbook-backed host adapters
- the standalone-specific parser is removed from the execution hot path
  or retained only as a narrow import/debug utility

This plan does **not** require solving the dependency graph, recalc scheduler,
or full execution backend extraction first. It only requires enough execution
support to run the compiled formulas already covered by standalone FODS replay.

## Scope Boundary

### In Scope

- standalone compile-host adapters over the internal workbook model
- native engine compilation for the FODS-safe grammar subset
- compiled-token execution support for the currently enabled standalone replay
  families
- dual-path validation between:
  - Calc legacy compiler
  - engine compiler
  - current standalone parser/evaluator
- family-by-family switchover of standalone replay

### Explicitly Out Of Scope

- full `ScInterpreter` extraction
- dependency graph extraction
- recalc queue/scheduler extraction
- full workbook mutation/rewrite behavior
- filter/import/export unification
- OpenCL/threaded backend migration
- removing all Calc compiler fallbacks immediately

## Current State

### What already exists

- canonical token / compile-host contract:
  - [TokenModel.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/TokenModel.hxx)
  - [CompileHost.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/CompileHost.hxx)
  - [CompilerPipeline.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/CompilerPipeline.hxx)
- Calc adapters and diff harness:
  - [TokenBridge.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/TokenBridge.hxx)
  - [CompileHost.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/CompileHost.hxx)
  - [ShadowCompiler.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ShadowCompiler.hxx)
  - [CompilerDiff.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/CompilerDiff.hxx)
- standalone FODS runtime:
  - [WorkbookModel.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/WorkbookModel.hxx)
  - [FodsLoader.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/FodsLoader.hxx)
  - [OdfFormulaParser.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/OdfFormulaParser.hxx)
  - [FodsEvaluator.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/FodsEvaluator.hxx)
- enabled raw FODS replay families:
  - logical
  - mathematical
  - text
  - date_time
  - spreadsheet
  - information

### What is still missing

- a workbook-backed compile host for standalone
- native engine compile logic that no longer relies on Calc compiler output
- an execution path that consumes canonical compiled tokens in standalone
- corpus-scale differential validation on raw FODS formulas through the shared
  compiler

## Design Rules

1. Keep the current standalone parser/evaluator alive until the shared compiler
   path proves itself on real workbook families.
2. Do not couple engine compiler code directly to `ScDocument`.
3. Make every switchover step corpus-driven.
4. Prefer family-by-family enablement over broad partial switchover.
5. Treat fallback removal as the last step, not the first.

## Recommended Phased Plan

### Phase 0: Freeze Corpus And Success Metrics

Goal:

- define exactly what “switchover complete” means

Tasks:

- freeze the initial standalone switchover corpus:
  - logical
  - mathematical
  - text
  - date_time
  - spreadsheet
  - information
- record the current replay counts and baseline runtime
- record the formulas currently executed via the standalone parser/evaluator
  rather than cached fallback
- define family-level exit gates:
  - compile parity
  - replay parity
  - no regression in current standalone maintenance lane

Exit criteria:

- the initial corpus and pass/fail thresholds are fixed for the first
  switchover milestone

Current frozen baseline for the initial milestone:

- enabled raw replay corpus:
  - logical: 9 workbooks
  - mathematical: 79 workbooks
  - text: 44 workbooks
  - date_time: 32 workbooks
  - spreadsheet: 44 workbooks
  - information: 20 workbooks
- total raw replay corpus: 228 workbooks
- current default replay runtime:
  - `spreadsheetengine_fods_replay_tests`: about `234s`
- current formula inventory on the enabled corpus:
  - formula cells: `19930`
  - parsed formulas: `18991`
  - cached-fallback cells: `6694`
  - cached-fallback rate: about `33.6%`
  - cell-reference nodes: `22903`
  - range-reference nodes: `3056`
  - named-reference nodes: `224`
  - array-constant nodes: `208`
  - function-call nodes: `17929`
- most frequent function calls in the baseline corpus:
  - `FORMULA`: `5941`
  - `ISERROR`: `1559`
  - `ROUND`: `1235`
  - `IF`: `867`
  - `LOOKUP`: `811`
  - `FALSE`: `615`
  - `AND`: `471`
  - `TRUE`: `461`
  - `VLOOKUP`: `316`
  - `MMULT`: `301`

Current milestone pass/fail thresholds:

- standalone `ctest` remains green
- the default raw replay lane remains green for all `228` workbooks
- replay runtime stays in the current rough envelope unless a deliberate
  switchover-mode expansion is being measured
- cached-fallback count does not increase above the frozen `6694` baseline
  while the legacy parser/evaluator remains the default execution path

Current manual compiler-preflight baseline on the same corpus:

- preflight-ready formula cells: `19838 / 19930` (about `99.54%`)
- preflight-expected-error formula cells: `41`
  - these are formulas that the current preflight cannot compile, but whose
    workbook cells already cache an expected error result
- preflight-not-ready formula cells: `92`
- preflight-hard-blocker formula cells: `51`
- current blocked categories:
  - parse failure: `14`
  - missing named reference: `37`
- current expected-error categories:
  - expected-error parse failure: `40`
  - expected-error missing named reference: `1`
- first representative examples:
  - parse failure: `columns.fods Sheet2.A19 of:=COLUMNS({1;2;3;4|5;6;7;8})`
  - missing named reference: `error.type.fods Sheet2.A15 of:=ERROR.TYPE(ahoj)`
  - expected-error parse failure:
    `not.fods Sheet2.A11 of:=NOT(0)NOT(0) => #VALUE!`
  - expected-error missing named reference:
    `substitute.fods Sheet2.K8 of:=SUBSTITUTE([.K1:.K4];a;f) => #VALUE!`

Current manual native-lowering smoke baseline on the same corpus:

- formula cells scanned: `19930`
- preflight-ready formula cells: `19838`
- successfully lowered formula cells: `19838`
- lowering failures on preflight-ready formulas: `0`
- ready-to-lowered rate: `100%`
- overall lowered rate: about `99.54%`
- implication:
  - the remaining `51` hard blockers no longer stop the first native-lowering
    smoke path
  - they are now strictly a preflight/parser/host tail, not a live lowering
    failure bucket

### Phase 1: Standalone Workbook Compile Host

Goal:

- let the engine compiler compile formulas against the standalone workbook
  model without any Calc dependency

Tasks:

- define a workbook-backed compile host adapter over:
  - sheets
  - named ranges
  - local/global name scope
  - copied-result imported sheets
  - grammar/base-address context
- decide and document what compile lookups are unsupported in standalone for
  the first milestone:
  - db ranges
  - table refs
  - col/row names
  - external refs
- implement clear status codes for unsupported host lookups so diff failures
  are diagnosable
- add focused standalone compile-host tests parallel to the Calc compile-host
  tests

Exit criteria:

- the engine compile request pipeline can be executed from the standalone
  workbook runtime with a complete host bundle for the initial corpus

### Phase 2: Native Engine Compiler For FODS-Safe Subset

Goal:

- make the engine compiler authoritative for the subset actually needed by the
  initial standalone replay families

Tasks:

- identify the formula forms that dominate the current enabled FODS families:
  - scalar literals
  - arithmetic operators
  - comparison operators
  - string concatenation
  - unary operators
  - scalar/range refs
  - named ranges
  - function calls with `;`
  - array literals used by the enabled families
  - whitespace / XML placeholder preservation where needed
- implement native engine lowering for those forms directly into canonical
  tokens
- keep Calc shadow/diff comparison active for the same formulas
- add standalone compiler tests that compile raw FODS formulas without using
  the old AST parser

Exit criteria:

- the initial formula subset compiles natively in `spreadsheet_engine`
- those formulas no longer require Calc compiler lowering to produce canonical
  token output

### Phase 3: Token Execution Adapter In Standalone

Goal:

- let standalone workbook replay execute compiled tokens instead of the bespoke
  AST

Current checkpoint:

- the standalone evaluator now has an initial compiled-token execution lane
  over canonical lowered formulas
- the first implementation inflates canonical tokens back into executable
  formula nodes and then reuses the existing evaluator semantics
- execution modes now keep separate AST vs compiled-token caches, preserve
  recursive evaluation mode, and keep cycle detection intact
- focused standalone side-by-side tests now cover:
  - recursive compiled cell evaluation
  - cached fallback in compiled mode
  - cycle detection in compiled mode
  - direct compiled-token formula execution
  - named-range execution in compiled mode

Current limitation:

- preflight-blocked formulas still fall back to the legacy AST path
- compiled-token execution currently reuses the AST evaluator semantics by
  inflating canonical lowered tokens back into executable nodes rather than
  running a direct token interpreter

Tasks:

- choose the first execution strategy:
  - direct token interpreter over canonical tokens
  - or a temporary lowering from canonical tokens into a smaller internal
    executable form
- implement only the execution behaviors needed for the initial corpus
- preserve:
  - lazy memoized evaluation
  - cycle detection
  - named-range evaluation
  - raw formula preservation for `FORMULA()`
  - cached-value fallback for still-unsupported formulas
- add side-by-side standalone tests that compare:
  - AST-evaluator result
  - token-execution result

Exit criteria:

- standalone can execute the initial compiled-token subset with parity to the
  existing evaluator

### Phase 4: Dual-Path FODS Replay Harness

Goal:

- compare old and new standalone execution paths before switching families

Current checkpoint:

- the raw FODS replay binary now has a manual `--compiled-diff` mode
- that mode compares AST vs compiled-token execution only on preflight-ready
  formulas, so the remaining parser/host tail does not drown the signal
- the diff output now records per-workbook outcomes instead of only a single
  corpus-wide count:
  - `matched`
  - `cached_fallback_only`
  - `execution_mismatch`
- the combined maintenance runner now has `--compiler-diff`, which exercises a
  fast logical-family diff smoke by default
- current real-corpus baselines are:
  - logical family:
    - formula cells: `517`
    - eligible: `513`
    - skipped: `4`
    - matched: `513`
    - cached-fallback-only mismatches: `0`
    - workbook outcomes: `matched:9`
  - full default corpus:
    - formula cells: `19930`
    - eligible: `19838`
    - skipped: `92`
    - matched: `19838`
    - cached-fallback-only mismatches: `1`
    - first cached-fallback-only example:
      `let.fods Sheet2.I49 of:=([.A49]=[.E49])`
      `AST='cached:1'` vs `compiled='live:1'`

Interpretation:

- the first dual-path replay slice is now proving value parity on the entire
  currently eligible corpus
- the remaining signal has narrowed to fallback-behavior differences rather
  than value mismatches

Tasks:

- extend the FODS replay harness so each workbook can run in:
  - legacy parser/evaluator mode
  - shared compiler + token execution mode
  - optional diff mode
- record per-workbook failure cause:
  - compile failure
  - token bridge mismatch
  - execution mismatch
  - cached fallback only
- add a switch to run only one family in diff mode for fast iteration

Exit criteria:

- the standalone harness can compare old-vs-new execution paths on real raw
  FODS workbooks

### Phase 5: Family-By-Family Switchover

Goal:

- move enabled families onto the shared production compiler one controlled
  family at a time

Recommended order:

1. logical
2. mathematical
3. text
4. date_time
5. information
6. spreadsheet

Tasks per family:

- enable shared compiler mode for that family in diff-only mode first
- fix compile parity gaps
- fix token execution gaps
- rerun full family replay until green
- promote the family so shared compiler mode becomes the default path
- keep the old parser path as a fallback until multiple families are stable

Exit criteria:

- the entire initial corpus runs through the shared production compiler path in
  standalone replay

Current checkpoint:

- all six enabled replay families are now promoted into the default standalone
  replay path:
  - logical
  - mathematical
  - text
  - date_time
  - information
  - spreadsheet
- the default replay lane now prefers shared compiler plus compiled-token
  execution for every preflight-ready formula in those families
- the remaining `51` hard preflight blockers are no longer on the hot path for
  family promotion; they remain a fallback-only tail
- the default standalone replay lane remains green for the full `228` workbook
  corpus with that promoted execution path

### Phase 6: Calc/Standalone Compiler Convergence Hardening

Goal:

- prove that the shared compiler is not only “standalone-good” but really the
  same production path Calc can rely on

Tasks:

- expand the Calc compile-diff corpus using formulas sourced from the enabled
  raw FODS families
- add standalone-vs-Calc compile artifact comparison for representative
  formulas from each family
- explicitly compare:
  - token streams
  - code error metadata
  - XML placeholder preservation
  - whitespace-sensitive cases
  - array literals
  - name resolution behavior
- keep legacy Calc fallback in adopted compile call sites until this phase is
  green

Exit criteria:

- the shared compiler has proven parity across both Calc and standalone
  contexts for the initial FODS corpus

Current checkpoint:

- Calc compile-diff smoke now includes formulas sourced from all six enabled
  FODS families, including:
  - logical
  - mathematical
  - text
  - date_time
  - information
  - spreadsheet
- a first standalone-vs-Calc artifact smoke is now in place for the currently
  lowerer-safe subset:
  - arithmetic / operator formulas
  - concat-operator formulas
  - reference formulas
  - array literals
  - error literals
  - range-name formulas
- current limitation:
  - exact canonical token-stream parity is not yet asserted here because the
    standalone lowerer still uses synthetic operator/function encodings that
    intentionally differ from Calc's imported canonical stream
  - XML formula-source preservation also still diverges between the two paths
    on representative formulas

### Phase 7: Switchover Completion And Retirement

Goal:

- make the shared compiler the default standalone production compiler

Tasks:

- flip the standalone FODS replay path to default to shared compiler mode
- keep a debug-only legacy parser mode for one stabilization period
- remove the legacy parser from the normal maintenance lane
- decide whether `OdfFormulaParser` remains:
  - as a debug tool
  - as an import-only helper
  - or is retired completely
- update docs and maintenance scripts

Exit criteria:

- standalone FODS replay uses the shared production compiler by default
- the legacy standalone parser is no longer part of the standard execution path

Current checkpoint:

- default standalone replay now uses the shared compiler path for all promoted
  families
- `spreadsheetengine_fods_replay_tests --legacy-only` provides the explicit
  legacy parser/evaluator escape hatch for stabilization and debugging
- the standard maintenance path now keeps the compiled-default replay lane in
  the hot path, with optional `--compiler-diff` coverage for dual-path smoke

## Detailed Execution Checklist

### Phase 0

- [x] Freeze the initial FODS switchover corpus.
- [x] Record current replay counts and runtime for the enabled families.
- [x] Record current cached-fallback rate on the enabled families.
- [x] Define milestone pass/fail thresholds.

### Phase 1

- [x] Add a workbook-backed compile host adapter under
      `spreadsheetengine/detail/` or a standalone-specific compat layer.
- [x] Support standalone name lookup for sheet-local and global named ranges.
- [x] Support compile context generation from workbook/sheet/cell position.
- [x] Add focused standalone compile-host tests.
- [x] Document unsupported standalone host lookups for the first milestone.

### Phase 2

- [x] Inventory the exact formula constructs used by the enabled FODS corpus.
- [x] Implement native engine lowering for the FODS-safe subset.
- [x] Add standalone compiler tests for raw FODS formulas.
- [x] Keep Calc shadow/diff comparisons green for the same subset.

### Phase 3

- [x] Add a compiled-token execution path in standalone.
- [x] Preserve lazy evaluation, memoization, and cycle detection.
- [x] Add standalone parity tests between AST execution and token execution.
- [x] Ensure cached fallback still works for unsupported formulas.

### Phase 4

- [x] Extend the raw FODS replay binary with dual-path execution modes.
- [x] Add per-workbook mismatch diagnostics.
- [x] Add family-scoped diff execution options.
- [x] Add maintenance-lane support for shared-compiler diff mode.

### Phase 5

- [x] Switch `logical` in diff mode, then promote it.
- [x] Switch `mathematical` in diff mode, then promote it.
- [x] Switch `text` in diff mode, then promote it.
- [x] Switch `date_time` in diff mode, then promote it.
- [x] Switch `information` in diff mode, then promote it.
- [x] Switch `spreadsheet` in diff mode, then promote it.

### Phase 6

- [x] Expand Calc compile-diff coverage with formulas sourced from enabled FODS
      families.
- [x] Add standalone-vs-Calc compile artifact comparisons for representative
      formulas.
- [ ] Keep adopted Calc compile call sites green with legacy fallback retained.

### Phase 7

- [x] Flip standalone replay to shared compiler by default.
- [x] Retain legacy parser mode only as a debug/escape hatch during
      stabilization.
- [x] Remove legacy parser from the standard maintenance path.
- [x] Update architecture docs, status docs, and maintenance scripts.

## Validation Strategy

Validation should happen at four levels.

### 1. Unit-Level Compiler Validation

- standalone compiler/token tests
- Calc token bridge tests
- Calc compile-host tests
- Calc shadow compiler tests
- Calc compile-diff tests

### 2. Standalone Dual-Path Validation

For each enabled FODS family:

- current parser/evaluator result
- shared compiler + token execution result
- workbook expected result

No family should be promoted until all three line up.

### 3. Calc/Standalone Cross-Context Validation

For representative formulas drawn from the FODS corpus, compare:

- Calc legacy compiler output
- engine compiler output in Calc context
- engine compiler output in standalone workbook context

### 4. Maintenance Validation

Keep the normal lanes green throughout:

- standalone `ctest`
- standalone FODS replay
- Calc compiler/token gates
- adopted Calc formula gates

## Recommended Immediate Next Slice

The best first implementation slice is:

1. add the standalone workbook-backed compile host
2. add focused standalone compiler tests for named ranges, refs, and scalar
   formulas
3. run the shared compiler in standalone shadow mode alongside the current
   `OdfFormulaParser`

That slice is small enough to validate tightly and starts the switchover
without forcing execution-path decisions too early.

### Current Progress

The first implementation slice is now in place:

- `spreadsheetengine/detail/WorkbookCompileHost.hxx` provides a workbook-backed
  compile host over `WorkbookModel`
- standalone compile context generation now exists for workbook/sheet/cell
  positions
- named-range lookup is supported for:
  - sheet-local names
  - global names
- the first-milestone unsupported lookups are explicitly fixed as:
  - database ranges
  - table refs
  - col/row names
  - external names
- focused standalone coverage lives in
  `tests/unit/token_compiler_host_tests.cxx`

The second implementation slice is now in place too:

- `tests/unit/fods_replay_tests.cxx` now has a `--summary` mode that scans the
  enabled raw replay corpus and records:
  - workbook counts by family
  - formula-cell / parsed-formula counts
  - cached-fallback usage
  - formula-construct counts
  - top function-call counts
- the Phase 0 baseline is now frozen in this document
- the initial Phase 2 formula inventory is now recorded from the real replay
  corpus rather than estimated manually

The third implementation slice is now in place:

- `spreadsheetengine/detail/FodsCompilerPreflight.hxx` provides a standalone
  compiler-readiness classifier over raw workbook formulas
- `tests/unit/fods_replay_tests.cxx` now has a manual `--preflight` mode that
  reports:
  - preflight-ready vs blocked formula counts
  - expected-error vs hard-blocker split
  - grouped reason counts
  - first representative blocking examples
- parser/preflight burn-down work now also covers:
  - namespace-prefixed error literals like `of:#ERR502!`
  - signed array-constant elements like `-0.4`
  - bare `A1` / `A1:B2` references such as `I13:K13`
  - reference-list / union syntax like `([.A1:.B3]~[.F2]~[.G1])`
  - range-constructor formulas like:
    - `[.$O6]:CHOOSE(...)`
    - `INDEX(...):INDEX(...)`
    - `XLOOKUP(...):XLOOKUP(...)`
  - adjacent logical-function chains like:
    - `([.A3]=[.D3])AND([.B3]=[.E3])AND([.C3]=[.F3])`
- the current full-corpus readiness baseline is now:
  - ready: `19838`
  - expected-error: `41`
  - hard blockers: `51`
  - the remaining gap is now a small tail rather than a dominant parser wall

The next implementation slice should move from measurement into execution:

1. close the highest-volume preflight blockers first:
   - remaining parser tail items like ragged-array constructs
   - unresolved workbook names that should remain explicit host/compiler gaps
   - cleanup of false-positive range-constructor cases like `err:7`
2. promote the manual preflight from classifier-only to first native-lowering
   attempts on the FODS-safe subset
3. keep that preflight manual and corpus-scoped before promoting it into the
   default maintenance lane

## Success Criteria

This program is complete when all of the following are true:

- standalone raw FODS replay compiles formulas through the shared production
  compiler by default
- the enabled FODS families remain green
- Calc and standalone share the same authoritative compiler implementation for
  the promoted corpus
- the old standalone parser is no longer required for the normal replay path

At that point, the project will have a real shared production compiler rather
than:

- Calc using one compiler
- standalone replay using another parser/evaluator stack

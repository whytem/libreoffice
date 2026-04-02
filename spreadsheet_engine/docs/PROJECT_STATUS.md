# Spreadsheet Engine: Project Status

This document is the current-state reference for the `spreadsheet_engine/`
project. It is intentionally written around the engine's present capabilities,
remaining boundary, and forward direction rather than around the phase
structure used to get here.

Historical implementation plans and closeout records still matter, but they
live in the archive and milestone docs. This file is for answering a simpler
question: what does the engine own today, what still lives in Calc, and what
should happen next?

## Project Objective

The long-term objective is to make `spreadsheet_engine/` the home for Calc's
spreadsheet-specific computation engine while keeping Calc as the document and
application host.

In practical terms, that means the engine should increasingly own:

- formula compilation
- token modeling and compiler-host interfaces
- formula evaluation semantics
- lookup, reference, matrix, and scalar execution helpers
- dependency analysis and invalidation planning
- recalculation planning and queue construction
- standalone workbook loading and evaluation

Calc should continue to own:

- document storage and mutation
- formula-cell lifecycle and host-side side effects
- UI, import/export, UNO, rendering, and shell integration
- host-only services that cannot sensibly move into a standalone engine

This remains an incremental extraction, not a rewrite.

## Current Summary

The project is past the "can this work?" stage. The engine now builds and runs
both standalone and inside LibreOffice, and the strategic center of gravity has
already moved into `spreadsheet_engine/`.

Today:

- the engine builds standalone with CMake and also builds inside LibreOffice
- the engine owns the shared token model and compiler-host surface
- the engine owns the standalone workbook model, FODS loader, parser, and
  evaluator
- the engine owns dependency snapshots, invalidation planning, and recalc
  planning/queue construction
- Calc already consumes a growing set of engine-owned runtime and compat
  helpers for execution semantics
- the promoted Calc FODS replay corpus is fully green with zero cached fallback

The active work is no longer replay promotion, compiler switchover, or
execution-shell extraction. Those programs are complete. The main frontier is
now widening engine-first adoption inside Calc while keeping the host boundary
clean and explicit. The active go-forward plan for that work lives in
[ENGINE_FIRST_CALC_ADOPTION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/ENGINE_FIRST_CALC_ADOPTION.md).

## Verified Baseline

The current promoted replay baseline is:

- `500` workbooks
- `50,661` formula cells
- `50,652` parsed formulas
- `0` cached-fallback cells
- `0` cached-fallback rate

Enabled FODS function-workbook families in the promoted corpus:

- `addin`
- `array`
- `database`
- `date_time`
- `financial`
- `information`
- `logical`
- `mathematical`
- `spreadsheet`
- `statistical`
- `text`

This baseline is maintained by a combined validation lane that includes:

- standalone evaluator and runtime unit tests
- Calc Cppunit coverage for extracted bridges and shared behavior
- one-shot `spreadsheetengine_fods_replay_tests --summary`
- diff hygiene checks

## What The Engine Owns Today

### Compiler And Token Infrastructure

The engine now owns the shared compilation substrate used to reason about
formulas independently of Calc internals:

- canonical token schema and token utilities
- compile-host interfaces and Calc-backed host adapters
- compile request/status pipeline
- token bridge and shadow-compiler infrastructure
- differential compiler comparison and diagnostics

This is no longer experimental infrastructure. It is part of the current
authoritative computation stack.

### Standalone Workbook And Evaluation Runtime

The engine can load and evaluate real spreadsheet workbooks without LibreOffice
runtime services:

- sparse workbook model with sheets, cells, named ranges, and imported-sheet
  metadata
- FODS loader and ODF formula parsing
- lazy evaluator with memoization, cycle handling, compiled-token execution,
  and replay support
- engine-owned runtime modules for lookup, query, text, date/time, financial,
  conversion, aggregate, statistical, reference, and scalar helper logic

The standalone evaluator is not a toy harness anymore. It is the main
regression oracle for shared spreadsheet semantics.

### Calculation-Facing Workbook And Recalc Planning

The engine now owns the calculation-facing planning layers that sit above raw
document storage:

- workbook facade contract with in-memory and Calc-backed implementations
- formula-cell and named-range discovery surfaces
- dependency snapshot construction
- reverse-dependency indexing and invalidation planning
- recalc planning and queue construction
- Calc queue-consumption bridge for applying engine-owned recalc output

At this point, the boundary between "planning" and "host execution" is explicit
and stable.

### Shared Runtime And Execution Adoption Inside Calc

Calc is already using engine-owned code for a substantial part of spreadsheet
execution semantics, including:

- broad pure-computation runtime families
- lookup and reference planning helpers
- bounded execution-shell helpers for lookup, jump, reference-shape, and
  inspection behavior
- shared coercion and normalization helpers
- bounded special-form and modern function shell helpers

The execution-shell extraction program is now complete. Calc no longer carries
an open in-scope spreadsheet-semantic interpreter tail that is waiting to be
moved into `spreadsheet_engine/`.

This means the project is no longer just extracting code for standalone use.
It is actively changing what Calc runs in production.

## What Still Lives In Calc

Calc remains the application and storage host, and some responsibilities are
intentionally still there:

- `ScDocument`, `ScTable`, `ScColumn`, and formula-cell storage
- listener/broadcaster wiring and host-side dependency side effects
- document mutation and formula-tree ownership
- UI, import/export, rendering, UNO, shell, and persistence
- threaded and OpenCL backend execution

There is also a remaining technical boundary that is intentionally still
Calc-owned:

- stack container mutation and formula-token cursor ownership
- `ScTokenArray` construction, range/union token-container operations, and
  other Calc-local token plumbing
- external-reference cache integration and document/session lookup services
- host-heavy inspection and environment services such as `INFO(...)`
- printer, path, number-format, and other document-service integrations that
  are not useful standalone engine semantics

This is now a defined host boundary rather than an open extraction gap.

## Current Assessment

The project is in a strong position.

The big strategic questions that existed earlier in the project have largely
been answered:

- standalone packaging works
- shared compiler infrastructure works
- replay parity works
- dependency and recalc planning extraction works
- Calc can consume extracted execution helpers safely through compat bridges

Because of that, the next work does not need to prove the architecture again.
It needs to broaden engine-first adoption inside Calc and keep the host
boundary stable.

In other words: the problem is now mostly one of disciplined boundary
tightening, not of feasibility.

One important cleanup milestone is also now true: within the execution-shell
surface extracted into `spreadsheet_engine`, there are no known remaining
standalone-vs-Calc duplicate helper implementations. The remaining Calc-owned
surface is explicitly host-shaped.

## Go-Forward Plan

The next plan should be organized around the end-state boundary rather than
around historical milestone names.

### 1. Deepen Engine-First Execution Inside Calc

This is the highest-priority stream.

The goal is to make the extracted engine-owned execution helpers the default
path in more production Calc call sites, while leaving storage and host
services in Calc.

The immediate target areas are:

- widening use of engine-owned runtime and compat helpers in Calc
- removing legacy Calc-local call paths where the shared path is already
  proven
- keeping Calc adapters thin and explicit

Success looks like a simpler Calc host shell whose remaining logic is clearly
host-only and whose production execution paths rely on engine-owned semantics
by default.

### 2. Consolidate The Host Boundary

The project still has legacy coupling points that should continue to shrink,
but they should now be treated as host-boundary cleanup rather than as
unfinished shell extraction:

- `ScTokenArray` as the pervasive Calc runtime token container
- direct use of Calc or LibreOffice vocabulary in code that should instead use
  engine-owned types
- compatibility logic spread across too many call sites instead of being
  concentrated in narrow adapters

This work is not primarily about adding new features. It is about making the
boundary cleaner and easier to maintain.

### 3. Keep The Zero-Fallback Baseline Stable

The zero-fallback promoted replay baseline is now an asset that needs to be
protected continuously.

That means:

- keeping one-shot promoted replay green
- adding focused regression coverage for every extracted execution slice
- treating replay regressions as boundary regressions, not just test failures
- keeping standalone and Calc validation lanes aligned

This is now part of normal project maintenance, not a side effort.

### 4. Reassess Deeper Authority Shifts Only After Adoption Widens

There are bigger long-term questions that may eventually matter, but they
should not drive near-term work:

- how much production Calc evaluation should directly route through engine
  entry points
- whether more of the remaining interpreter shell can become engine-owned
- whether the production compiler path should tighten further around the shared
  compiler model

Those are valid future questions, but the right way to reach them is first to
keep widening proven engine-first adoption inside Calc.

## Working Rules For The Next Stage

The project should continue under these rules:

- do not reopen the recalc authority boundary unless a concrete gap requires it
- do not attempt to move document storage into the engine
- do not rewrite `ScInterpreter` wholesale
- prefer narrow compat bridges over broad cross-layer entanglement
- keep one core implementation where semantics are spreadsheet-specific
- validate every extraction slice in both standalone and Calc

## Definition Of Progress

From this point forward, meaningful progress should be measured less by
"completed phases" and more by these questions:

- Is more spreadsheet-specific execution logic engine-owned?
- Is Calc thinner and more clearly host-only?
- Is the zero-fallback replay baseline still intact?
- Are duplicate implementations continuing to disappear?
- Is the public/compat boundary getting simpler rather than more tangled?

If the answer to those questions keeps moving in the right direction, the
project is on track.

## Active Reference Docs

The most relevant active docs are:

- [EXECUTION_BACKEND_EXTRACTION.md](architecture/EXECUTION_BACKEND_EXTRACTION.md)
- [EXECUTION_SHELL_CLOSEOUT_PLAN.md](architecture/EXECUTION_SHELL_CLOSEOUT_PLAN.md)
- [README.md](architecture/README.md)

Historical extraction records and completed workplans live under:

- [docs/archive/](archive/)
- [docs/extraction-history/](extraction-history/)

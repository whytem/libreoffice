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
execution-shell extraction. Those programs are complete. The engine-first Calc
adoption program is also complete as a bounded workstream, and its standing
guardrails now live in
[ENGINE_FIRST_CALC_ADOPTION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/ENGINE_FIRST_CALC_ADOPTION.md).
The first bounded direct-entry adoption stream is also complete and now lives
as a closeout record in
[ENGINE_ENTRYPOINT_ADOPTION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/ENGINE_ENTRYPOINT_ADOPTION_PLAN.md).
The follow-on production-boundary tightening stream is now complete as well,
and the remaining retained production boundary is explicitly documented as an
intentional host-owned surface rather than an open cleanup tail. The
second-wave engine-entry widening stream is complete too, and widened direct
production entry further for the selected add-in financial and bounded local
`CELL(...)` paths.

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
- one-shot `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
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
- document null-date services and holiday-list expansion used by host-owned
  add-ins
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
It needs to keep tightening the production compiler/evaluation boundary while
keeping the host boundary stable.

In other words: the problem is now mostly one of disciplined boundary
tightening, not of feasibility.

The last fresh retained-host-boundary analysis selected the
external-reference production seam as the best next bounded candidate, and
that stream is now complete. It narrowed the remaining external `CELL(...)`
projection tail, moved the selected single-ref and double-ref fetch packaging
behind a named compat seam, and removed the superseded wrapper layer in that
scope.

The next fresh reassessment result is narrower still: the retained host
boundary itself now looks largely stable, and the clearest remaining
non-host-shaped residue is the final legacy pocket in the Analysis add-in
financial surface around `ODDFPRICE` and `ODDFYIELD`.

One important cleanup milestone is also now true: within the execution-shell
surface extracted into `spreadsheet_engine`, there are no known remaining
standalone-vs-Calc duplicate helper implementations. The remaining Calc-owned
surface is explicitly host-shaped.

## Go-Forward Plan

The next plan should be organized around the end-state boundary rather than
around historical milestone names.

### 1. Widen Direct Engine Entry Inside Calc

The first bounded direct-entry adoption stream is complete, the
production-boundary tightening stream is complete, and the second-wave
engine-entry widening stream is complete.

Together, those streams moved Calc from a small first-wave of direct engine
entry to a broader production set that now includes:

- direct text parsing entry adoption
- direct formula inspection entry adoption
- second-wave add-in financial direct entry for the selected pure-computation,
  date-mode, and null-date callers
- direct bounded local `CELL(...)` inspection entry for the local-workbook
  subset

The bounded external-reference `CELL(...)` inspection candidate was explicitly
reassessed and deferred rather than left as an ambiguous open tail, because
external cache ownership and host-shaped address/file projection still dominate
that surface.

Future direct-entry widening should now start from a fresh inventory rather
than by treating the second-wave stream as still active.

The completed direct-entry closeout records are:

- [ENGINE_ENTRYPOINT_ADOPTION_PLAN.md](architecture/ENGINE_ENTRYPOINT_ADOPTION_PLAN.md)
- [ENGINE_ENTRY_WIDENING_PLAN.md](architecture/ENGINE_ENTRY_WIDENING_PLAN.md)

### 1A. Narrow The Remaining Host-Service Facades

The host-service facade narrowing stream is complete.

That stream narrowed the bounded external `CELL(...)` projection subset,
converged add-in null-date and holiday assembly on one named context seam,
moved the retained local `CELL(...)` host-property tail and bounded
`INFO(...)` projection behind explicit compat adapters, and removed the
superseded wrapper layer that used to sit alongside those facades.

What remains in this area is now an explicit host-owned tail rather than an
open packaging cleanup stream:

- external-reference cache ownership and the remaining external `CELL(...)`
  format-style projection
- live null-date and holiday-input ownership in the add-in layer
- inherently host-bound environment services behind `INFO(...)`

The completed closeout record for that stream is:

- [HOST_SERVICE_FACADE_NARROWING_PLAN.md](architecture/HOST_SERVICE_FACADE_NARROWING_PLAN.md)

### 2. Keep The Production Boundary Tight And Explicit

The first bounded direct-entry adoption stream is complete, and the follow-on
production-boundary tightening stream is complete.

Together, that work switched Calc's text-parsing and formula-inspection
production paths onto explicit engine entry adapters, converged host-service
context packaging, tightened retained `CELL(...)` and `INFO(...)` result
projection, isolated external-reference fetch/projection seams, and collapsed
the repeated direct-entry setup that still remained in Calc callers.

This means the remaining retained production boundary now reads as deliberate
host ownership rather than unfinished compiler/evaluation packaging.

The completed closeout records for those two streams are:

- [ENGINE_ENTRYPOINT_ADOPTION_PLAN.md](architecture/ENGINE_ENTRYPOINT_ADOPTION_PLAN.md)
- [PRODUCTION_BOUNDARY_TIGHTENING_PLAN.md](architecture/PRODUCTION_BOUNDARY_TIGHTENING_PLAN.md)

### 3. Keep The Remaining Token Boundary Explicit

The bounded token-boundary reduction stream is complete.

That work froze the remaining token/container inventory, extracted the in-scope
non-owning reference-token traversal and adaptation seams, normalized the
touched `AREAS` caller vocabulary, and marked the retained token-array and
external-reference paths explicitly as Calc-owned host seams.

The remaining token/container surface is now treated as intentional host
ownership or explicit defer scope, not as an open cleanup tail.

The completed closeout record for that stream is:

- [TOKEN_BOUNDARY_REDUCTION_PLAN.md](architecture/TOKEN_BOUNDARY_REDUCTION_PLAN.md)

### 4. Deepen Engine-First Execution Inside Calc

The current deepening stream is complete for the identified safe adoption
surface.

That work widened default Calc use of engine-owned calendar, financial, and
execution helpers, removed the superseded local semantic tail in the touched
add-in surface, and left the remaining local code explicitly host-only.

Future widening should continue, but now as new bounded slices selected from a
fresh inventory rather than as unfinished work from the current plan.

The detailed implementation and closeout record for that stream is:

- [DEEPEN_ENGINE_FIRST_EXECUTION_PLAN.md](architecture/DEEPEN_ENGINE_FIRST_EXECUTION_PLAN.md)

### 5. Keep The Host Boundary Explicit And Stable

The bounded host-boundary consolidation stream is complete.

That work froze the remaining execution-adjacent Calc surface into an explicit
inventory, narrowed the touched compat vocabulary, collapsed the first
translation seam, isolated the in-scope host-backed `CELL(...)` services, and
removed the known duplicate helper in the touched boundary.

The remaining Calc-owned surfaces are now treated as intentional host concerns
or explicit defer items, not as an ambiguous backlog of unfinished shell
extraction.

The completed closeout record for that stream is:

- [HOST_BOUNDARY_CONSOLIDATION_PLAN.md](architecture/HOST_BOUNDARY_CONSOLIDATION_PLAN.md)

### 6. Keep The Zero-Fallback Baseline Stable

The zero-fallback promoted replay baseline is now an asset that needs to be
protected continuously.

That means:

- keeping one-shot promoted replay green
- adding focused regression coverage for every extracted execution slice
- treating replay regressions as boundary regressions, not just test failures
- keeping standalone and Calc validation lanes aligned

This is now part of normal project maintenance, not a side effort.

### 7. Reassess New Bounded Frontiers Only After A Fresh Inventory

There are bigger long-term questions that may eventually matter, but they
should not drive near-term work:

- how much production Calc evaluation should directly route through engine
  entry points
- whether more of the remaining interpreter shell can become engine-owned
- whether the production compiler path should tighten further around the shared
  compiler model

Those are valid future questions, but the right way to reach them is to start
from the now-cleaner host boundary and choose the next bounded stream from a
fresh inventory instead of carrying forward historical cleanup debt.

That reassessment is now complete and recorded in:

- [COMPILER_EVALUATION_BOUNDARY_REASSESSMENT_PLAN.md](architecture/COMPILER_EVALUATION_BOUNDARY_REASSESSMENT_PLAN.md)

The completed reassessment established three stable outcomes:

- the remaining local compiler packaging around `CELL(...)` address projection
  and `INDIRECT` reference compilation was tightened behind named compile
  helpers instead of staying open-coded in production callers
- the remaining date-mode financial add-in family now uses the direct engine
  adapter path by default
- the still-local `CELL(...)` host-property tail, bounded `INFO(...)`
  projection, and external-reference cache/session packaging remain explicit
  Calc-owned host services rather than ambiguous extraction residue

There is no new pre-committed follow-on stream from that reassessment. Future
work should again begin from a fresh inventory of the intentionally retained
host boundary instead of treating the completed reassessment as open-ended
cleanup.

That fresh inventory has now been done for the retained host boundary, and the
selected external-reference stream is now complete:

- [EXTERNAL_REFERENCE_FACADE_TIGHTENING_PLAN.md](architecture/EXTERNAL_REFERENCE_FACADE_TIGHTENING_PLAN.md)

That completed stream established three stable outcomes:

- the remaining external `CELL(...)` property/result projection tail now sits
  behind the compat boundary instead of living inline in `ScCellExternal()`
- external single-ref and double-ref fetch packaging now uses one named
  result-shape seam in the compat layer
- the remaining external-reference boundary is explicitly host-owned:
  external cache/session access, document linkage, token-container lifetime,
  and stack mutation stay in Calc

There is no new pre-committed follow-on stream from that closeout. Future work
should again begin from a fresh inventory of the intentionally retained host
boundary instead of treating the completed external-reference stream as an
open cleanup tail.

That latest reassessment identifies one new bounded follow-on stream:

- [ADDIN_FINANCIAL_TAIL_CONVERGENCE_PLAN.md](architecture/ADDIN_FINANCIAL_TAIL_CONVERGENCE_PLAN.md)

The reason for choosing it is straightforward:

- `INFO(...)` and the local/external `CELL(...)` host tails are already thin
  and explicitly host-owned
- add-in null-date and holiday ownership is already explicit and stable
- the remaining `ODDFPRICE` / `ODDFYIELD` path still stands out as legacy
  semantic residue rather than a deliberate host seam
- the neighboring Analysis financial surface already uses direct shared-runtime
  adapters, so the payoff/risk ratio is favorable

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

- [ADDIN_FINANCIAL_TAIL_CONVERGENCE_PLAN.md](architecture/ADDIN_FINANCIAL_TAIL_CONVERGENCE_PLAN.md)
- [EXTERNAL_REFERENCE_FACADE_TIGHTENING_PLAN.md](architecture/EXTERNAL_REFERENCE_FACADE_TIGHTENING_PLAN.md)
- [COMPILER_EVALUATION_BOUNDARY_REASSESSMENT_PLAN.md](architecture/COMPILER_EVALUATION_BOUNDARY_REASSESSMENT_PLAN.md)
- [HOST_SERVICE_FACADE_NARROWING_PLAN.md](architecture/HOST_SERVICE_FACADE_NARROWING_PLAN.md)
- [ENGINE_ENTRY_WIDENING_PLAN.md](architecture/ENGINE_ENTRY_WIDENING_PLAN.md)
- [ENGINE_ENTRYPOINT_ADOPTION_PLAN.md](architecture/ENGINE_ENTRYPOINT_ADOPTION_PLAN.md)
- [PRODUCTION_BOUNDARY_TIGHTENING_PLAN.md](architecture/PRODUCTION_BOUNDARY_TIGHTENING_PLAN.md)
- [TOKEN_BOUNDARY_REDUCTION_PLAN.md](architecture/TOKEN_BOUNDARY_REDUCTION_PLAN.md)
- [HOST_BOUNDARY_CONSOLIDATION_PLAN.md](architecture/HOST_BOUNDARY_CONSOLIDATION_PLAN.md)
- [DEEPEN_ENGINE_FIRST_EXECUTION_PLAN.md](architecture/DEEPEN_ENGINE_FIRST_EXECUTION_PLAN.md)
- [ENGINE_FIRST_CALC_ADOPTION.md](architecture/ENGINE_FIRST_CALC_ADOPTION.md)
- [EXECUTION_BACKEND_EXTRACTION.md](architecture/EXECUTION_BACKEND_EXTRACTION.md)
- [EXECUTION_SHELL_CLOSEOUT_PLAN.md](architecture/EXECUTION_SHELL_CLOSEOUT_PLAN.md)
- [README.md](architecture/README.md)

Historical extraction records and completed workplans live under:

- [docs/archive/](archive/)
- [docs/extraction-history/](extraction-history/)

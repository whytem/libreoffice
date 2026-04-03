# Spreadsheet Engine: Project Status

This document is the current-state reference for the
`spreadsheet_engine/` project.

It is written around three questions:

- what the engine owns today
- what still intentionally lives in Calc
- what the project is actively doing next

Completed extraction streams and closeout records still matter, but they live
in the architecture, archive, and extraction-history docs rather than
structuring this status file.

## Project Objective

The long-term objective is to make `spreadsheet_engine/` the home for Calc's
spreadsheet-specific computation engine while keeping Calc as the document and
application host.

In practical terms, the engine should own:

- formula compilation and compiler-host interfaces
- token modeling and execution-facing compiler output
- spreadsheet evaluation semantics
- lookup, reference, matrix, scalar, and coercion helpers
- dependency analysis, invalidation planning, and recalc planning
- standalone workbook loading and evaluation

Calc should continue to own:

- UI, import/export, rendering, UNO, and shell integration
- persistence and document-service integrations
- host-only services that are not useful as standalone engine semantics

The active program described below goes further than the original boundary and
intentionally reopens some of the computational storage and lifecycle split,
but it still does not change the goal of keeping Calc as the application host.

## Current State

The original extraction objective is close to being met.

Today:

- the engine builds standalone with CMake and also builds inside LibreOffice
- the engine owns the shared token model and compiler-host surface
- the engine owns the standalone workbook model, FODS loader, parser, and
  evaluator
- the engine owns dependency snapshots, invalidation planning, and recalc
  planning/queue construction
- Calc already consumes a substantial body of engine-owned runtime and compat
  code in production execution paths
- the promoted Calc FODS replay corpus is fully green with zero cached
  fallback

`spreadsheet_engine/` is therefore already a real shared computation layer,
not just a standalone harness.

## Verified Baseline

The standing promoted replay baseline is:

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

This baseline is maintained by a standing validation lane that includes:

- standalone evaluator and runtime unit tests
- Calc Cppunit coverage for extracted bridges and shared behavior
- one-shot `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- diff hygiene checks

## Capabilities Now Owned By `spreadsheet_engine`

### Compiler And Token Infrastructure

The engine owns the shared compilation substrate used to reason about formulas
independently of Calc internals:

- canonical token schema and token utilities
- compile-host interfaces and Calc-backed host adapters
- compile request and status plumbing
- token bridge and shadow-compiler infrastructure
- differential compiler comparison and diagnostics

### Standalone Workbook Loading And Evaluation

The engine can load and evaluate real spreadsheet workbooks without depending
on LibreOffice runtime services:

- sparse workbook model with sheets, cells, named ranges, and imported-sheet
  metadata
- FODS loader and ODF formula parsing
- lazy evaluator with memoization, cycle handling, compiled-token execution,
  and replay support
- shared runtime modules for lookup, query, text, date/time, financial,
  conversion, aggregate, statistical, reference, and scalar helper logic

The standalone evaluator now acts as a primary regression oracle for shared
spreadsheet semantics.

### Dependency And Recalc Planning

The engine owns the calculation-facing planning layers above raw document
storage:

- workbook facade contracts with in-memory and Calc-backed implementations
- formula-cell and named-range discovery surfaces
- dependency snapshot construction
- reverse-dependency indexing and invalidation planning
- recalc planning and queue construction
- Calc queue-consumption bridges for engine-owned recalc output

This means the engine already owns the declarative dependency and recalc side
of the computation stack.

### Shared Execution Logic Used By Calc

Calc already runs a substantial body of engine-owned execution behavior in
production:

- broad pure-computation runtime families
- lookup and reference helpers
- coercion and normalization helpers
- bounded jump, special-form, inspection, and reference-shape helpers
- direct entry adapters for selected parsing, inspection, calendar, and
  financial paths
- narrowed compat facades for selected host-bound inspection and
  external-reference projection paths

Within the extracted execution surface, there are no known remaining
standalone-versus-Calc duplicate helper implementations for the in-scope
spreadsheet semantics that have already been ported.

### Add-In And Host Integration Surfaces

The engine now owns a meaningful part of the spreadsheet semantics used by the
Analysis add-in and related host adapters:

- shared financial runtime for the adopted financial surface
- shared calendar/workday/month-shift semantics where the host provides null
  date or holiday inputs
- direct add-in adapters that package host services around engine-owned
  semantics instead of duplicating spreadsheet logic inside the add-in layer

The remaining add-in-specific semantic gap is explicit rather than hidden:
`ODDFPRICE` and `ODDFYIELD` are still deferred because there is no shared
odd-first-period runtime implementation in the current engine or Calc tree.

## Current Boundary With Calc

Under the presently shipped architecture, Calc still owns:

- `ScDocument`, `ScTable`, `ScColumn`, and formula-cell storage
- document mutation and formula-tree ownership
- listener and broadcaster wiring plus host-side dependency side effects
- stack container mutation and formula-token cursor ownership
- `ScTokenArray` construction, range/union token-container operations, and
  other Calc-local token plumbing
- external-reference cache integration plus document and session lookup
  services
- null-date acquisition and holiday-input expansion for host-owned add-ins
- host-heavy inspection and environment services such as `INFO(...)`
- printer, path, number-format, and other document-service integrations
- UI, import/export, rendering, UNO, shell, persistence, and threaded/OpenCL
  backends

That boundary is clear and stable enough that the first-stage extraction can
be considered successful on its own terms.

## Active Execution Path

The project has now made an explicit decision to proceed beyond that stable
first-stage boundary.

The active path is:

- [COMPUTATIONAL_SUBSTRATE_EXTRACTION_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_EXTRACTION_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_PHASE0_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_PHASE0_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_PHASE1_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_PHASE1_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_PHASE1_DECISION_RECORD.md](architecture/COMPUTATIONAL_SUBSTRATE_PHASE1_DECISION_RECORD.md)
- [COMPUTATIONAL_SUBSTRATE_PHASE2_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_PHASE2_PLAN.md)

This is a major second-stage architecture program. Its target end state is not
just "more helpers extracted" or "more direct engine entry adoption." The goal
is to make `spreadsheet_engine/` the intended owner of the live computational
substrate that currently still sits in Calc, including:

- formula tree
- broadcast track
- broadcast-area machine
- listener and broadcaster graph semantics
- computation-facing table and column storage, if needed
- an engine-owned execution-facing IR instead of long-term dependence on
  `ScTokenArray` as the authoritative representation

Calc would remain the application and document host, but the center of gravity
for live computational state would move further into the engine.

The program has now completed:

- Phase 0 observability and scope-freeze on a narrowed subset
- Phase 1 engine-owned computational storage shadowing for that subset,
  including representative row-insert and column-delete rebuild coverage

The next frontier is Phase 2 live dependency-graph shadowing on the admitted
Phase 1 subset rather than a broad authority jump.

## Why This Path Was Chosen

The first-stage extraction work answered the original feasibility questions:

- shared compiler infrastructure works
- standalone packaging works
- replay parity works
- dependency and recalc planning extraction works
- Calc can safely consume extracted execution logic through compat seams and
  direct entry adapters

Because those questions are now settled, the next meaningful expansion is no
longer another narrow helper-cleanup stream. The largest remaining
computation-owned residue still sits where live dependency services, listener
and broadcaster state, formula-tree ownership, and execution-facing token
containers remain embedded in Calc storage.

That makes computational substrate extraction the clearest path if the project
intends to keep moving the authoritative computation boundary outward.

## Phase 0 Outcome

The active program did not begin by shifting authority immediately. It began by
proving that the live computational substrate could be observed precisely
enough to justify later shadow work.

That Phase 0 gate is now complete.

The checked-in closeout material lives in:

- [COMPUTATIONAL_SUBSTRATE_PHASE0_INVENTORY.md](architecture/COMPUTATIONAL_SUBSTRATE_PHASE0_INVENTORY.md)
- [COMPUTATIONAL_SUBSTRATE_PHASE0_OBSERVABLE_STATE_MODEL.md](architecture/COMPUTATIONAL_SUBSTRATE_PHASE0_OBSERVABLE_STATE_MODEL.md)
- [COMPUTATIONAL_SUBSTRATE_PHASE0_SCENARIO_MATRIX.md](architecture/COMPUTATIONAL_SUBSTRATE_PHASE0_SCENARIO_MATRIX.md)
- [COMPUTATIONAL_SUBSTRATE_PHASE0_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_PHASE0_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_PHASE0_DECISION_RECORD.md](architecture/COMPUTATIONAL_SUBSTRATE_PHASE0_DECISION_RECORD.md)

The outcome is:

- proceed to Phase 1 on a narrower subset

What Phase 0 established:

- an exact ownership map for formula tree, broadcast track, BASM, listener
  contexts, broadcaster storage, and `ScTokenArray` mutation/update sites
- a stable observable-state model for formula-tree, formula-track,
  broadcaster, listener, dependency, and recalc comparisons
- checked-in capture helpers for live formula-tree, formula-track, and
  normalized broadcaster state
- a representative mutation scenario matrix
- an automated differential lane covering scalar edits, formula edits,
  delayed listener startup, and delayed broadcaster deletion

What Phase 0 did not claim:

- broad authority readiness for every structural edit, load-time setup, or
  Calc container layout detail

That is why the proceed decision is intentionally narrowed rather than broad.

## Current Assessment

The project is in a strong position, but the active program is intentionally a
higher-risk undertaking than the bounded extraction streams that came before
it.

That should shape expectations:

- the current architecture is already successful without this second-stage
  program
- the computational substrate path is a deliberate expansion, not unfinished
  first-stage work
- each later phase must earn the next one through exact shadowing and
  validation
- stopping after Phase 0 or a later shadow phase remains a valid outcome if
  the new boundary proves too costly or too tangled

## Working Rules For The Active Program

The project should continue under these rules:

- do not sacrifice the zero-fallback replay baseline
- do not move UI, UNO, rendering, persistence, or environment services into
  the engine
- do not accept a dual-authority model where Calc remains the hidden real
  authority
- prefer exact shadowing and explicit rollback over optimistic authority
  shifts
- treat memory and performance regressions as architecture issues, not
  follow-up polish
- stop when a phase proves the new boundary is not worth the complexity

## Definition Of Progress

From this point forward, progress should be measured by questions like:

- Is more of the live computational state engine-owned?
- Is the dependency graph becoming engine-authoritative?
- Is Calc becoming thinner as a computation host while still remaining the
  application host?
- Is the replay and differential validation baseline still intact?
- Is the boundary getting cleaner rather than creating a tangled dual system?

If those answers stay positive, the second-stage program is moving in the
right direction.

## Reference Material

For the active architecture path, see:

- [COMPUTATIONAL_SUBSTRATE_EXTRACTION_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_EXTRACTION_PLAN.md)
- [README.md](architecture/README.md)

For completed plans, closeout records, and historical extraction context, see:

- [docs/archive/](archive/)
- [docs/extraction-history/](extraction-history/)

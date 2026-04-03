# Spreadsheet Engine: Project Status

This document is the current-state reference for the
`spreadsheet_engine/` project.

It is intentionally written around the engine's present capabilities, the
current boundary with Calc, and the remaining strategic choices. Completed
implementation streams still matter, but they now live primarily as reference
material in the architecture, archive, and extraction-history docs rather than
as the structure for this status file.

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

- document storage and mutation
- formula-cell lifecycle and host-side side effects
- UI, import/export, UNO, rendering, and shell integration
- host-only services that are not useful as standalone engine semantics

This remains an incremental extraction, not a rewrite.

## Current State

The original extraction objective is now close to being met.

`spreadsheet_engine/` is no longer just a standalone experiment or a replay
harness. It is a production-shared computation layer that builds both
standalone and inside LibreOffice, and it already owns most of the
spreadsheet-specific semantics that motivated the extraction in the first
place.

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

There is no longer a broad open extraction tail inside the in-scope execution
surface. The remaining Calc-owned boundary is now mostly intentional and
host-shaped.

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

This is part of the authoritative computation stack, not scaffolding.

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

The boundary between planning and host execution is now explicit.

### Shared Execution Logic Used By Calc

Calc already runs a meaningful amount of engine-owned execution behavior in
production:

- broad pure-computation runtime families
- lookup and reference helpers
- coercion and normalization helpers
- bounded jump, special-form, inspection, and reference-shape helpers
- direct entry adapters for selected parsing, inspection, calendar, and
  financial paths
- narrowed compat facades for selected host-bound inspection and
  external-reference projection paths

One important milestone is now true: within the extracted execution surface,
there are no known remaining standalone-versus-Calc duplicate helper
implementations for the in-scope spreadsheet semantics that have already been
ported.

### Add-In And Host Integration Surfaces

The engine also now owns a meaningful part of the spreadsheet semantics used by
the Analysis add-in and related host adapters:

- shared financial runtime for the adopted financial surface
- shared calendar/workday/month-shift semantics where the host provides
  document services such as null date or holiday inputs
- direct add-in adapters that package host services around engine-owned
  semantics instead of duplicating spreadsheet logic inside the add-in layer

The remaining add-in-specific semantic gap is explicit rather than hidden:
`ODDFPRICE` and `ODDFYIELD` are still deferred because there is no shared
odd-first-period runtime implementation in the current engine or Calc tree.

## Current Boundary With Calc

The current boundary is defined less by "what has not been extracted yet" and
more by "what is intentionally host-owned."

### Calc-Owned By Design

Calc remains the document and application host. The following responsibilities
intentionally still live there:

- `ScDocument`, `ScTable`, `ScColumn`, and formula-cell storage
- document mutation and formula-tree ownership
- listener and broadcaster wiring plus host-side dependency side effects
- UI, import/export, rendering, UNO, shell, and persistence
- threaded and OpenCL backend execution

There is also a remaining technical boundary that stays Calc-owned by design:

- stack container mutation and formula-token cursor ownership
- `ScTokenArray` construction, range/union token-container operations, and
  other Calc-local token plumbing
- external-reference cache integration plus document and session lookup
  services
- null-date acquisition and holiday-input expansion for host-owned add-ins
- host-heavy inspection and environment services such as `INFO(...)`
- printer, path, number-format, and other document-service integrations that
  do not make useful standalone engine semantics

### Explicitly Retained Or Deferred

The remaining non-adopted surfaces are now supposed to be explicit:

- `ODDFPRICE` and `ODDFYIELD` are a deliberate defer item, not a hidden helper
  path
- the remaining host-heavy `CELL(...)`, `INFO(...)`, and external-reference
  tails are retained because the dominant concerns there are host service,
  document integration, or cache ownership
- token ownership and interpreter stack mutation remain intentionally on the
  Calc side unless a very narrow seam clearly justifies moving again

This means the project is no longer carrying an ambiguous "cleanup tail" in
the main extracted execution surface.

## Current Assessment

The project is in a strong position.

The major architectural questions that existed earlier in the extraction have
already been answered positively:

- shared compiler infrastructure works
- standalone packaging works
- replay parity works
- dependency and recalc planning extraction works
- Calc can safely consume extracted execution logic through compat seams and
  direct entry adapters

The remaining decisions are mostly about optional scope expansion, not about
whether the extracted architecture is viable.

If work stopped here, the project would already represent a successful
extraction of Calc's spreadsheet-specific computation core into a standalone
and shared engine with a well-defined host boundary.

## Options For Further Scope Expansion

If the goal is to move additional functionality out of Calc and into
`spreadsheet_engine/`, the most plausible options are now narrower and more
selective than the original extraction program.

### Ranked Options

| Rank | Option | Value | Risk | Assessment |
| --- | --- | --- | --- | --- |
| 1 | Wider direct engine-entry adoption for safe production Calc paths | High | Medium | Best next expansion if the goal is to make Calc run more engine-owned code without reopening the host boundary. The spreadsheet semantics already exist in the engine; the work is mainly careful production adoption. |
| 2 | Add new shared runtime implementations for explicitly deferred spreadsheet semantics | Medium to High | Medium | Best next expansion if the goal is semantic completeness rather than boundary cleanup. The odd-first-period financial pair is the clearest current example, but this would be real algorithm/runtime work, not simple rewiring. |
| 3 | Deeper external-reference semantic extraction above the current facade | Medium | Medium to High | Still potentially useful, but the remaining complexity is dominated by host-owned cache, session, and document integration. Worth pursuing only if a fresh inventory reveals another narrow seam. |
| 4 | Further compiler and evaluation entry tightening around retained production callers | Medium | Medium | There may still be some benefit in making production Calc callers thinner, but the payoff is now incremental rather than transformational. This should be inventory-driven rather than assumed. |
| 5 | Additional reduction of Calc-local token and interpreter shell ownership | Low to Medium | High | Possible in theory, but the remaining token and stack surfaces are tightly coupled to Calc host ownership. The risk-to-payoff ratio is much worse now than it was earlier in the extraction. |
| 6 | Migration of more host-heavy inspection or document-service behavior | Low | High | Full `INFO(...)`, host-heavy `CELL(...)`, printer/path, and number-format semantics are dominated by document and environment services. These are poor standalone-engine targets unless product requirements change. |
| 7 | Storage, mutation, lifecycle, or recalc-execution migration | Very Low | Very High | Not recommended under the current architecture. This would move the project away from its successful "engine for spreadsheet semantics, Calc for document hosting" boundary. |

There is now a dedicated architecture plan for the conditional boundary-shift
version of option 7:

- [COMPUTATIONAL_SUBSTRATE_EXTRACTION_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_EXTRACTION_PLAN.md)

That document should be read as a major second-stage architecture program, not
as the default next step for the current project boundary.

### Recommended Direction If Expansion Continues

If additional scope is desired, the most sensible default is:

1. look first for more safe direct engine-entry adoption opportunities in
   production Calc callers
2. only after that, consider new shared semantic implementations where there is
   a clear product or interoperability reason to fill an explicitly deferred
   gap

Everything below those two options now has materially worse payoff relative to
its architectural risk.

## Working Rules For Any Future Expansion

Future work should continue under these rules:

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
- Is the public and compat boundary getting simpler rather than more tangled?

If the answer to those questions keeps moving in the right direction, the
project is still advancing.

## Reference Material

For current architecture references, see:

- [docs/architecture/README.md](architecture/README.md)

For completed plans, closeout records, and historical extraction context, see:

- [docs/archive/](archive/)
- [docs/extraction-history/](extraction-history/)

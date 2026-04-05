# Spreadsheet Engine: Project Status

This document is the current-state reference for the
`spreadsheet_engine/` project.

It is written as a settled-boundary snapshot rather than a milestone ledger.
Completed plans, phase records, and historical closeout notes still matter,
but they live under [architecture/](architecture/), [archive/](archive/),
and [extraction-history/](extraction-history/) instead of structuring this
status file.

## Project Objective

The long-term objective is to make `spreadsheet_engine/` the home for Calc's
spreadsheet-specific computation engine while keeping Calc as the document and
application host.

In practical terms, the engine is intended to own:

- formula compilation and compiler-host interfaces
- token modeling and execution-facing compiler output
- spreadsheet evaluation semantics
- lookup, reference, matrix, scalar, and coercion helpers
- dependency analysis, invalidation planning, and recalc planning
- standalone workbook loading and evaluation

Calc is intended to remain responsible for:

- UI, import/export, rendering, UNO, and shell integration
- persistence and document-service integrations
- host-only services that do not make sense as standalone engine semantics

## Current Status At A Glance

The original extraction objective is effectively achieved.

Today:

- the engine builds standalone with CMake and also builds inside LibreOffice
- the engine owns the shared compiler and token-model surface
- the engine owns the standalone workbook model, FODS loader, parser, and
  evaluator
- the engine owns dependency snapshots, invalidation planning, and recalc
  planning/queue construction
- Calc already consumes a substantial body of engine-owned runtime and compat
  logic in production execution paths
- the promoted Calc FODS replay corpus is fully green with zero cached
  fallback

`spreadsheet_engine/` is therefore already a real shared computation layer,
not just a standalone harness or experimental replay tool.

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

The engine owns the declarative planning layers above raw document storage:

- workbook facade contracts with in-memory and Calc-backed implementations
- formula-cell and named-range discovery surfaces
- dependency snapshot construction
- reverse-dependency indexing and invalidation planning
- recalc planning and queue construction
- Calc queue-consumption bridges for engine-owned recalc output

This means the engine already owns the dependency and recalc-planning side of
the computation stack.

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

One explicit add-in defer remains:

- `ODDFPRICE`
- `ODDFYIELD`

Those are still deferred because there is no shared odd-first-period runtime
implementation in the current engine or Calc tree.

## Settled Boundary With Calc

Calc still intentionally owns:

- broad `ScDocument`, `ScTable`, and `ScColumn` storage outside the admitted
  slice
- document mutation APIs and broad formula-cell object lifetime outside the
  admitted slice
- live host realization and rollback of dependency side effects
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

That first-stage extraction boundary is clear and can be treated as
successful on its own terms.

## Settled Conclusions From The Computational Substrate Program

The computational-substrate architecture program is complete enough to support
clear conclusions.

What it did justify:

- engine-authored shadow models for computational state, dependency graph,
  and execution-facing IR on a narrowed subset
- engine-authored authority for a bounded live compat slice
- a real opt-in rollout path for that bounded authority slice
- one bounded widening step beyond the original admitted structural surface

What it did not justify:

- broad storage migration
- broad listener/broadcaster ownership transfer
- broad token-container ownership transfer
- shared-group-sensitive structural rollout
- named-range-sensitive structural rollout
- sheet-wide structural or document-wide authority transfer

The result is intentionally narrow:

- not stop
- not broad rollout
- a bounded opt-in authority slice with exact verification and explicit
  rollback

## Current Opt-In Narrow Rollout Surface

The current admitted narrow rollout surface is:

- admitted scalar lifecycle authority
- admitted scalar mutation entry
- single-sheet `InsertRows`
- single-sheet `DeleteRows`
- single-sheet `InsertColumns`
- single-sheet `DeleteColumns`
- ordinary scalar formulas only
- engine-owned admitted-slice formula-cell lifetime decisions
- clean baseline only
- no shared groups
- no named-range-sensitive structural behavior
- exact queue verification
- exact computational verification
- exact graph verification
- repair-detected rollback on structural divergence

This rollout remains opt-in and bounded. It is not evidence for broad
computational storage migration by itself.

Calc still owns the live host side of this slice:

- raw document mutation APIs
- final application of engine-authored realization records
- final live verification and rollback mechanics

That is acceptable because the current rollout is a compat-driven authority
slice, not a blanket handoff of the computational document core.

## Explicitly Deferred Surfaces

The following remain outside the admitted rollout and outside the settled
engine-owned boundary:

- shared-group-sensitive structural behavior
- named-range-sensitive structural behavior
- sheet insert, delete, rename, or move
- copy, move, clipboard, load-time, or undo-like structural flows
- broader storage migration
- broad formula-cell object lifetime migration outside the admitted slice
- token-container ownership transfer
- broad listener/broadcaster ownership transfer

The completed named-range widening proof cycle clarified that boundary rather
than moving it:

- the live rollout still excludes named-range-sensitive structural behavior
- a bounded global single-area named-range class now exists only as a
  validation-only pilot surface
- sheet-local, multi-area, and scope-ambiguous named-range classes remain
  explicitly deferred

The completed global-admission proof cycle narrowed that further:

- the bounded global single-area class now has exact standalone prediction and
  a separate gated live-candidate path
- that same bounded class still does not meet live rollout standards
- off-sheet global-name consumers do not stay inside the current promotion
  surface

Any expansion into named-range-sensitive structural behavior should therefore
begin with another bounded reassessment rather than being inferred from the
current rollout.

## Storage And Wiring Outcome

The first storage-and-wiring proof cycle is now complete.

It justified one additional bounded boundary shift:

- the engine now owns the mutable sidecar state on the admitted slice
- the engine now owns the listener/broadcaster, formula-tree, and
  formula-track target sets on that same slice
- Calc can clear and rebuild the admitted live listener/tree/track surface
  from those engine-owned targets with exact graph verification

It did not justify physical container residency migration. Calc still owns:

- `ScDocument` storage and mutation entry
- formula-cell object lifetime
- live broadcaster/listener container residency
- final rollback

That means the current boundary is stronger than a read-only shadow model but
still narrower than a true storage transplant.

The key storage-and-wiring closeout references are:

- [COMPUTATIONAL_SUBSTRATE_STORAGE_AND_WIRING_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_STORAGE_AND_WIRING_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_STORAGE_AND_WIRING_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_STORAGE_AND_WIRING_CONTRACT.md)
- [COMPUTATIONAL_SUBSTRATE_MUTABLE_SUBSTRATE_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_MUTABLE_SUBSTRATE_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_GRAPH_DELTA_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_GRAPH_DELTA_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_WIRING_APPLY_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_WIRING_APPLY_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_STORAGE_AND_WIRING_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_STORAGE_AND_WIRING_EVIDENCE.md)
- [COMPUTATIONAL_SUBSTRATE_STORAGE_AND_WIRING_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_STORAGE_AND_WIRING_DECISION_RECORD.md)
- [COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_RESIDENCY_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_RESIDENCY_PLAN.md)

The named-range closeout references remain relevant because they still define
what stays outside this admitted storage-and-wiring slice:

- [COMPUTATIONAL_SUBSTRATE_NAMED_RANGE_STRUCTURAL_WIDENING_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_NAMED_RANGE_STRUCTURAL_WIDENING_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_NAMED_RANGE_STRUCTURAL_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_NAMED_RANGE_STRUCTURAL_DECISION_RECORD.md)
- [COMPUTATIONAL_SUBSTRATE_GLOBAL_NAMED_RANGE_ADMISSION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_GLOBAL_NAMED_RANGE_ADMISSION_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_GLOBAL_NAMED_RANGE_ADMISSION_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_GLOBAL_NAMED_RANGE_ADMISSION_DECISION_RECORD.md)

## Cell Storage Residency Outcome

The first cell-storage residency proof cycle is now complete.

It justified one additional bounded boundary shift:

- the engine now owns resident admitted-slice cell storage
- the engine still owns mutable computational state on that slice
- the engine still owns graph and wiring target sets on that slice
- Calc can mirror the admitted live cell surface from engine-owned resident
  state and still pass exact computational and graph verification after
  wiring replay

It did not justify broad `ScDocument` storage migration. Calc still owns:

- mutation entry and document mutation APIs
- formula-cell object lifetime
- live broadcaster/listener container residency
- final rollback

That means the current boundary is now stronger than the earlier
mutable-sidecar-plus-host-storage split, but it is still narrower than a
broad computational document transplant.

The key cell-storage closeout references are:

- [COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_RESIDENCY_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_RESIDENCY_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_RESIDENCY_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_RESIDENCY_CONTRACT.md)
- [COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_SCHEMA.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_SCHEMA.md)
- [COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_MIRRORING.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_MIRRORING.md)
- [COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_EVIDENCE.md)
- [COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_DECISION_RECORD.md)
- [COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_RESIDENCY_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_RESIDENCY_PLAN.md)

## Wiring Container Residency Outcome

The first wiring-container residency proof cycle is now complete.

It justified one additional bounded boundary shift:

- the engine now owns resident admitted-slice wiring containers
- the engine still owns resident admitted-slice cell storage
- the engine still owns mutable computational state plus graph, wiring, and
  queue decisions on that slice
- Calc can realize the admitted live wiring surface from engine-owned
  resident wiring state and still pass exact computational and graph
  verification

It did not justify formula-cell object lifetime migration or broad
dependency-container migration. Calc still owns:

- mutation entry and document mutation APIs
- formula-cell object lifetime
- final rollback

That means the current boundary is now stronger than the earlier
resident-cell-plus-host-wiring split, but it is still narrower than a broad
computational document transplant.

The key wiring-container closeout references are:

- [COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_RESIDENCY_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_RESIDENCY_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_RESIDENCY_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_RESIDENCY_CONTRACT.md)
- [COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_SCHEMA.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_SCHEMA.md)
- [COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_REALIZATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_REALIZATION.md)
- [COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_EVIDENCE.md)
- [COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_DECISION_RECORD.md)

## Formula-Cell Lifetime Outcome

The first formula-cell lifetime proof cycle is now complete.

It justified one additional bounded boundary shift:

- the engine now owns admitted-slice formula-cell lifetime decisions
- the engine still owns resident admitted-slice cell storage
- the engine still owns resident admitted-slice wiring containers
- the engine still owns mutable computational state plus graph, wiring, and
  queue decisions on that slice
- Calc can realize admitted live `ScFormulaCell` objects from engine-owned
  lifetime state and still pass exact computational, graph, and queue
  verification

It did not justify direct mutation-entry migration or broad formula-cell
object migration outside the admitted slice. On the current settled
boundary, Calc still owns:

- mutation entry and document mutation APIs
- final rollback

The admitted live object-realization surface named there has since been
pulled further into the engine by the completed object-realization
reassessment.

That means the current boundary is now stronger than the earlier
resident-cell-plus-host-lifetime split, but it is still narrower than a
broad computational document transplant.

The key formula-cell-lifetime closeout references are:

- [COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_CONTRACT.md)
- [COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_SCHEMA.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_SCHEMA.md)
- [COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_REALIZATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_REALIZATION.md)
- [COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_EVIDENCE.md)
- [COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_DECISION_RECORD.md)

## Mutation Entry Outcome

The first mutation-entry proof cycle and the completed scalar convergence
reassessment are now both complete.

Together they justify one additional bounded boundary shift:

- the engine now owns admitted scalar mutation-entry request shape and
  routing on the bounded slice
- the engine now owns admitted scalar mutation-entry after-state decisions on
  that slice
- direct scalar mutation entry now closes with exact queue, computational,
  graph, and broadcaster verification after Calc realization
- the already-proven direct formula-entry and admitted structural-entry lanes
  remain green
- explicit dirty-baseline rejection and rollback remain in place

They still do not justify broad host-independent mutation application. Calc
still owns:

- raw document mutation APIs
- final rollback

The key mutation-entry closeout references are:

- [COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_CONTRACT.md)
- [COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_SCHEMA.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_SCHEMA.md)
- [COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_REALIZATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_REALIZATION.md)
- [COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_EVIDENCE.md)
- [COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_DECISION_RECORD.md)
- [COMPUTATIONAL_SUBSTRATE_SCALAR_MUTATION_ENTRY_CONVERGENCE_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SCALAR_MUTATION_ENTRY_CONVERGENCE_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_SCALAR_MUTATION_ENTRY_CONVERGENCE_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SCALAR_MUTATION_ENTRY_CONVERGENCE_CONTRACT.md)
- [COMPUTATIONAL_SUBSTRATE_SCALAR_MUTATION_ENTRY_CANONICALIZATION_MATRIX.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SCALAR_MUTATION_ENTRY_CANONICALIZATION_MATRIX.md)
- [COMPUTATIONAL_SUBSTRATE_SCALAR_MUTATION_ENTRY_OBSERVATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SCALAR_MUTATION_ENTRY_OBSERVATION.md)
- [COMPUTATIONAL_SUBSTRATE_SCALAR_MUTATION_ENTRY_CONVERGENCE_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SCALAR_MUTATION_ENTRY_CONVERGENCE_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_SCALAR_MUTATION_ENTRY_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SCALAR_MUTATION_ENTRY_EVIDENCE.md)
- [COMPUTATIONAL_SUBSTRATE_SCALAR_MUTATION_ENTRY_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SCALAR_MUTATION_ENTRY_DECISION_RECORD.md)

## Object Realization Outcome

The object-realization reassessment is now complete.

It justifies one additional bounded boundary shift:

- the engine now owns the admitted live object-realization record on the
  bounded slice
- Calc realizes admitted formula-cell, wiring, formula-tree, and
  formula-track live objects from that engine-authored record
- applied admitted mutation-entry results now carry explicit
  object-realization observations
- admitted exact cases close with exact object realization on the bounded
  slice

It still does not justify broad host independence. Calc still owns:

- raw document mutation APIs
- final rollback

That means the current boundary is stronger than the earlier resident-state-
plus-host-realization split, but it is still narrower than a broad
computational document transplant.

The key object-realization closeout references are:

- [COMPUTATIONAL_SUBSTRATE_OBJECT_REALIZATION_REASSESSMENT_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_OBJECT_REALIZATION_REASSESSMENT_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_OBJECT_REALIZATION_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_OBJECT_REALIZATION_CONTRACT.md)
- [COMPUTATIONAL_SUBSTRATE_OBJECT_REALIZATION_MATRIX.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_OBJECT_REALIZATION_MATRIX.md)
- [COMPUTATIONAL_SUBSTRATE_OBJECT_REALIZATION_OBSERVATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_OBJECT_REALIZATION_OBSERVATION.md)
- [COMPUTATIONAL_SUBSTRATE_OBJECT_REALIZATION_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_OBJECT_REALIZATION_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_OBJECT_REALIZATION_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_OBJECT_REALIZATION_EVIDENCE.md)
- [COMPUTATIONAL_SUBSTRATE_OBJECT_REALIZATION_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_OBJECT_REALIZATION_DECISION_RECORD.md)

## Next Explicit Reassessment Target

The next explicit bounded concern is:

- final rollback reassessment

The object-realization reassessment is now closed with a bounded proceed
result. The most actionable remaining host-owned surface on the admitted
slice is no longer live realization. It is the retained rollback boundary
around the already-engine-authored resident, mutation-entry, and realization
surfaces.

## Current Assessment

The project is in a strong position:

- the first-stage extraction is successful and operationally valuable on its
  own
- the zero-fallback replay baseline is stable
- Calc and standalone share a large body of real spreadsheet semantics
- the computational-substrate program produced a meaningful bounded authority
  result without forcing a risky broad boundary flip

The current state should be read as:

- a successful shared-engine project
- plus a successful narrow second-stage authority experiment
- plus a successful bounded storage-and-wiring authority proof
- plus a successful bounded admitted-slice cell-residency proof
- plus a successful bounded admitted-slice wiring-container residency proof
- plus a successful bounded admitted-slice formula-cell lifetime proof
- plus a successful bounded admitted-slice object-realization proof
- but still not as proof that a full computational storage migration is
  already justified

## Working Rules Going Forward

Any further expansion should continue under these rules:

- do not sacrifice the zero-fallback replay baseline
- do not move UI, UNO, rendering, persistence, or environment services into
  the engine
- do not accept a dual-authority model where Calc remains the hidden real
  authority
- prefer exact verification and explicit rollback over optimistic authority
  shifts
- treat memory and performance regressions as architecture issues, not
  follow-up polish
- require a new explicit plan before widening beyond the current admitted
  rollout, storage-and-wiring slice, admitted cell-residency slice, and
  admitted wiring-container residency slice, admitted formula-cell-lifetime
  slice, and admitted object-realization slice

## Reference Material

For the current second-stage boundary and rollout closeout, see:

- [COMPUTATIONAL_SUBSTRATE_EXTRACTION_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_EXTRACTION_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_NARROW_ROLLOUT_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_NARROW_ROLLOUT_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_NARROW_ROLLOUT_DECISION_RECORD.md](architecture/COMPUTATIONAL_SUBSTRATE_NARROW_ROLLOUT_DECISION_RECORD.md)
- [COMPUTATIONAL_SUBSTRATE_NAMED_RANGE_STRUCTURAL_WIDENING_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_NAMED_RANGE_STRUCTURAL_WIDENING_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_NAMED_RANGE_STRUCTURAL_DECISION_RECORD.md](architecture/COMPUTATIONAL_SUBSTRATE_NAMED_RANGE_STRUCTURAL_DECISION_RECORD.md)
- [COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_RESIDENCY_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_RESIDENCY_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_DECISION_RECORD.md](architecture/COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_DECISION_RECORD.md)
- [COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_RESIDENCY_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_RESIDENCY_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_DECISION_RECORD.md](architecture/COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_DECISION_RECORD.md)
- [COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_DECISION_RECORD.md](architecture/COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_DECISION_RECORD.md)
- [COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_DECISION_RECORD.md](architecture/COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_DECISION_RECORD.md)
- [COMPUTATIONAL_SUBSTRATE_SCALAR_MUTATION_ENTRY_CONVERGENCE_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_SCALAR_MUTATION_ENTRY_CONVERGENCE_PLAN.md)
- [README.md](architecture/README.md)

For completed plans, closeout records, and historical extraction context, see:

- [archive/](archive/)
- [extraction-history/](extraction-history/)

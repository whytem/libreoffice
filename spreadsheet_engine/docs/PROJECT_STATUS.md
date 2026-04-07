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
- one bounded shared-group structural authority expansion beyond the
  ownership-complete slice
- one bounded shared-group non-structural member-exit expansion on top of
  that structural slice
- one additional bounded shared-group non-structural same-text preserve
  expansion on top of the member-exit slice

What it did not justify:

- broad storage migration
- broad listener/broadcaster ownership transfer
- broad token-container ownership transfer
- broad shared-group-sensitive rollout
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
- ordinary scalar formulas plus exact same-sheet shareable shared-group
  structural `Preserve`, `Split`, and `Rebuild` cases behind the dedicated
  shared-group gate
- exact same-sheet shareable shared-group non-structural member-exit
  `SetScalarValue`, `SetFormula`, and `ClearCell` cases behind the dedicated
  non-structural shared-group gate
- exact same-sheet shareable shared-group same-text preserve `SetFormula`
  behind that same dedicated non-structural shared-group gate
- engine-owned admitted-slice formula-cell lifetime decisions
- clean baseline only
- no named-range-sensitive structural behavior
- exact queue verification
- exact computational verification
- exact graph verification
- repair-detected rollback on structural divergence

This rollout remains opt-in and bounded. It is not evidence for broad
computational storage migration by itself.

Calc still owns the live host side of this slice:

- raw document mutation APIs
- final application of engine-authored realization and rollback records
- final live verification shell around those records

That is acceptable because the current rollout is a compat-driven authority
slice, not a blanket handoff of the computational document core.

## Explicitly Deferred Surfaces

The following remain outside the admitted rollout and outside the settled
engine-owned boundary:

- shared-group-sensitive structural behavior outside the bounded same-sheet
  shareable exact structural plus non-structural member-exit slice
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

The completed shared-group widening workstream now promotes additional
bounded shared-group families while keeping the rest of the boundary
explicit:

- same-sheet shareable shared-group preserve, split, and rebuild outcomes now
  have explicit facade-side classification
- same-sheet shareable structural preserve, split, and rebuild cases now
  reach the admitted live structural lane when both structural gates are
  enabled
- same-sheet shareable non-structural member-exit scalar, formula, and clear
  cases now reach the admitted authority/lifecycle lane when the dedicated
  non-structural shared-group gate is enabled
- same-sheet shareable non-structural same-text preserve `SetFormula` now
  reaches the admitted lifecycle and mutation-entry lanes behind that same
  dedicated non-structural shared-group gate
- same-sheet shareable non-structural edge-regroup `SetFormula` now reaches
  the admitted lifecycle and mutation-entry lanes behind that same
  dedicated non-structural shared-group gate
- same-sheet shareable non-structural gap-closing merge `SetFormula` now
  reaches the admitted lifecycle and mutation-entry lanes behind that same
  dedicated non-structural shared-group gate
- same-sheet shareable non-structural edge replacement-merge `SetFormula`
  now reaches the admitted lifecycle and mutation-entry lanes behind that
  same dedicated non-structural shared-group gate
- same-sheet shareable non-structural one-sided adjacent insertion
  `SetFormula` now reaches the admitted lifecycle and mutation-entry lanes
  behind that same dedicated non-structural shared-group gate
- non-exact or broader shared-group classes still fall back to the
  validation-only pilot lane or stay deferred
- gate-off and named-range-combined shared-group classes reject
  deterministically
- repair-sensitive shared-group divergence remains repair-detected and
  rollback-capable
- broader shared-group live rollout still stays deferred because
  multi-group collapse, named-range-combined, repair-sensitive, off-sheet,
  and broader non-edge regroup or merge classes are not yet engine-authored

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
- the final live apply shell around engine-authored rollback records

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
- the final live apply shell around engine-authored rollback records

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
- the final live apply shell around engine-authored rollback records

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
- the final live apply shell around engine-authored rollback records

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
- the final live apply shell around engine-authored rollback records

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
- the final live apply shell around engine-authored rollback records

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

## Final Rollback Outcome

The final-rollback reassessment is now complete.

It justifies one additional bounded boundary shift:

- the engine now owns the admitted rollback record on the bounded slice
- mutation entry now reuses that explicit rollback record for
  dirty-baseline rejection, verification failure, and repair-detected
  rollback
- admitted rollback now reports explicit rollback observation instead of
  silently restoring through an ad hoc host path
- helper-level and admitted runtime rollback proof lanes close exactly on the
  bounded slice

It still does not justify broad host independence. Calc still owns:

- raw document mutation APIs
- the final live apply shell that executes engine-authored realization and
  rollback records

That means the current boundary is now stronger than the earlier
resident-state-plus-host-rollback split, but it is still narrower than a
broad computational document transplant.

The key final-rollback closeout references are:

- [COMPUTATIONAL_SUBSTRATE_FINAL_ROLLBACK_REASSESSMENT_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FINAL_ROLLBACK_REASSESSMENT_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_FINAL_ROLLBACK_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FINAL_ROLLBACK_CONTRACT.md)
- [COMPUTATIONAL_SUBSTRATE_FINAL_ROLLBACK_MATRIX.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FINAL_ROLLBACK_MATRIX.md)
- [COMPUTATIONAL_SUBSTRATE_FINAL_ROLLBACK_OBSERVATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FINAL_ROLLBACK_OBSERVATION.md)
- [COMPUTATIONAL_SUBSTRATE_FINAL_ROLLBACK_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FINAL_ROLLBACK_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_FINAL_ROLLBACK_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FINAL_ROLLBACK_EVIDENCE.md)
- [COMPUTATIONAL_SUBSTRATE_FINAL_ROLLBACK_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FINAL_ROLLBACK_DECISION_RECORD.md)

## Raw Mutation API Outcome

The raw-mutation reassessment is now complete.

It justifies one additional bounded boundary shift:

- the engine now owns the admitted raw mutation record on the bounded slice
- admitted scalar, formula, clear, and narrow structural entry now pass
  through that explicit engine-authored record before live apply
- admitted exact apply lanes now carry explicit raw-mutation observation
  instead of relying on hidden host-originated mutation identity
- dirty-baseline rejection and rollback stay exact through the raw-mutation
  path

It still does not justify broad host independence. Calc still owns:

- the underlying raw document mutation APIs used to execute the admitted
  record
- the final live apply shell that executes engine-authored raw mutation,
  realization, and rollback records

That means the current boundary is now stronger than the earlier
resident-state-plus-host-raw-mutation split, but it is still narrower than a
broad computational document transplant.

The key raw-mutation closeout references are:

- [COMPUTATIONAL_SUBSTRATE_RAW_MUTATION_API_MIGRATION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_RAW_MUTATION_API_MIGRATION_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_RAW_MUTATION_API_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_RAW_MUTATION_API_CONTRACT.md)
- [COMPUTATIONAL_SUBSTRATE_RAW_MUTATION_API_SCHEMA.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_RAW_MUTATION_API_SCHEMA.md)
- [COMPUTATIONAL_SUBSTRATE_RAW_MUTATION_API_OBSERVATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_RAW_MUTATION_API_OBSERVATION.md)
- [COMPUTATIONAL_SUBSTRATE_RAW_MUTATION_API_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_RAW_MUTATION_API_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_RAW_MUTATION_API_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_RAW_MUTATION_API_EVIDENCE.md)
- [COMPUTATIONAL_SUBSTRATE_RAW_MUTATION_API_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_RAW_MUTATION_API_DECISION_RECORD.md)

## Live Apply-Shell Outcome

The live apply-shell reassessment is now complete.

It justifies one additional bounded boundary shift:

- the engine now owns the admitted live apply plan on the bounded slice
- admitted scalar, formula, clear, and narrow structural entry now carry
  that explicit engine-authored apply plan before exact verification
  completes
- applied admitted mutation-entry results now carry explicit live-apply
  observation instead of relying on hidden host sequencing
- dirty-baseline rejection and rollback stay exact through the live
  apply-plan path

It still does not justify broad host independence. Calc still owns:

- the underlying raw document mutation APIs used to execute the admitted
  plan stages
- the primitive realization and rollback host operations
- the final verification host shell around those primitive operations

That means the current boundary is now stronger than the earlier
resident-state-plus-host-apply-shell split, but it is still narrower than a
broad computational document transplant.

The key live apply-shell closeout references are:

- [COMPUTATIONAL_SUBSTRATE_LIVE_APPLY_SHELL_REASSESSMENT_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_LIVE_APPLY_SHELL_REASSESSMENT_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_LIVE_APPLY_SHELL_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_LIVE_APPLY_SHELL_CONTRACT.md)
- [COMPUTATIONAL_SUBSTRATE_LIVE_APPLY_SHELL_SCHEMA.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_LIVE_APPLY_SHELL_SCHEMA.md)
- [COMPUTATIONAL_SUBSTRATE_LIVE_APPLY_SHELL_OBSERVATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_LIVE_APPLY_SHELL_OBSERVATION.md)
- [COMPUTATIONAL_SUBSTRATE_LIVE_APPLY_SHELL_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_LIVE_APPLY_SHELL_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_LIVE_APPLY_SHELL_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_LIVE_APPLY_SHELL_EVIDENCE.md)
- [COMPUTATIONAL_SUBSTRATE_LIVE_APPLY_SHELL_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_LIVE_APPLY_SHELL_DECISION_RECORD.md)

## Raw Document Mutation Outcome

The raw document mutation reassessment is now complete.

It justifies one additional bounded boundary shift:

- the engine now owns the explicit admitted raw document mutation record on
  the bounded slice
- admitted scalar, formula, clear, and narrow structural entry now carries
  that explicit engine-authored primitive mutation record before the
  retained primitive host shell completes
- applied admitted mutation-entry results now carry explicit raw document
  mutation observation instead of relying only on the broader raw-mutation
  observation
- dirty-baseline rejection and rollback keep the primitive mutation layer
  explicit and exact on the bounded slice

It still does not justify broad host independence. Calc still owns:

- the primitive realization host operations around admitted primitive
  execution
- the primitive rollback host operations around admitted primitive
  execution
- the final verification host shell around those primitive operations

That means the current boundary is now stronger than the earlier
resident-state-plus-host-primitive-mutation split, but it is still narrower
than a broad computational document transplant.

The key raw document mutation closeout references are:

- [COMPUTATIONAL_SUBSTRATE_RAW_DOCUMENT_MUTATION_API_MIGRATION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_RAW_DOCUMENT_MUTATION_API_MIGRATION_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_RAW_DOCUMENT_MUTATION_API_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_RAW_DOCUMENT_MUTATION_API_CONTRACT.md)
- [COMPUTATIONAL_SUBSTRATE_RAW_DOCUMENT_MUTATION_API_SCHEMA.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_RAW_DOCUMENT_MUTATION_API_SCHEMA.md)
- [COMPUTATIONAL_SUBSTRATE_RAW_DOCUMENT_MUTATION_API_OBSERVATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_RAW_DOCUMENT_MUTATION_API_OBSERVATION.md)
- [COMPUTATIONAL_SUBSTRATE_RAW_DOCUMENT_MUTATION_API_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_RAW_DOCUMENT_MUTATION_API_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_RAW_DOCUMENT_MUTATION_API_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_RAW_DOCUMENT_MUTATION_API_EVIDENCE.md)
- [COMPUTATIONAL_SUBSTRATE_RAW_DOCUMENT_MUTATION_API_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_RAW_DOCUMENT_MUTATION_API_DECISION_RECORD.md)

## Primitive Realization And Rollback Outcome

The primitive realization and rollback reassessment is now complete.

It justifies one additional bounded boundary shift:

- the engine now owns the explicit admitted primitive realization record on
  the bounded slice
- the engine now owns the explicit admitted primitive rollback record on
  the bounded slice
- applied admitted mutation-entry results now carry explicit primitive
  realization observation instead of relying only on the broader
  object-realization observation
- rollback admitted mutation-entry results now carry explicit primitive
  rollback observation instead of relying only on the broader rollback
  observation

It still does not justify broad host independence. Calc still owns:

- the final verification host shell around admitted primitive realization
  and rollback execution

That means the current boundary is now stronger than the earlier
resident-state-plus-host-realization-rollback split, but it is still
narrower than a broad computational document transplant.

The key primitive realization and rollback closeout references are:

- [COMPUTATIONAL_SUBSTRATE_PRIMITIVE_REALIZATION_ROLLBACK_REASSESSMENT_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PRIMITIVE_REALIZATION_ROLLBACK_REASSESSMENT_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_PRIMITIVE_REALIZATION_ROLLBACK_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PRIMITIVE_REALIZATION_ROLLBACK_CONTRACT.md)
- [COMPUTATIONAL_SUBSTRATE_PRIMITIVE_REALIZATION_ROLLBACK_SCHEMA.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PRIMITIVE_REALIZATION_ROLLBACK_SCHEMA.md)
- [COMPUTATIONAL_SUBSTRATE_PRIMITIVE_REALIZATION_ROLLBACK_OBSERVATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PRIMITIVE_REALIZATION_ROLLBACK_OBSERVATION.md)
- [COMPUTATIONAL_SUBSTRATE_PRIMITIVE_REALIZATION_ROLLBACK_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PRIMITIVE_REALIZATION_ROLLBACK_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_PRIMITIVE_REALIZATION_ROLLBACK_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PRIMITIVE_REALIZATION_ROLLBACK_EVIDENCE.md)
- [COMPUTATIONAL_SUBSTRATE_PRIMITIVE_REALIZATION_ROLLBACK_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PRIMITIVE_REALIZATION_ROLLBACK_DECISION_RECORD.md)

## Final Verification Host-Shell Outcome

The final verification host-shell reassessment is now complete.

It justifies one additional bounded boundary shift:

- the engine now owns the explicit admitted final verification record on the
  bounded slice
- applied admitted mutation-entry results now carry explicit final
  verification observation instead of relying only on the implicit
  path-specific verification outcome
- dirty-baseline rollback admitted mutation-entry results now also carry
  explicit final verification observation on the bounded slice

It still does not justify broad host independence. Calc still owns:

- the primitive execution host operations around admitted mutation,
  realization, and rollback execution

That means the current boundary is now stronger than the earlier
resident-state-plus-host-verification split, but it is still narrower than
a broad computational document transplant.

The key final verification host-shell closeout references are:

- [COMPUTATIONAL_SUBSTRATE_FINAL_VERIFICATION_HOST_SHELL_REASSESSMENT_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FINAL_VERIFICATION_HOST_SHELL_REASSESSMENT_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_FINAL_VERIFICATION_HOST_SHELL_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FINAL_VERIFICATION_HOST_SHELL_CONTRACT.md)
- [COMPUTATIONAL_SUBSTRATE_FINAL_VERIFICATION_HOST_SHELL_SCHEMA.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FINAL_VERIFICATION_HOST_SHELL_SCHEMA.md)
- [COMPUTATIONAL_SUBSTRATE_FINAL_VERIFICATION_HOST_SHELL_OBSERVATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FINAL_VERIFICATION_HOST_SHELL_OBSERVATION.md)
- [COMPUTATIONAL_SUBSTRATE_FINAL_VERIFICATION_HOST_SHELL_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FINAL_VERIFICATION_HOST_SHELL_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_FINAL_VERIFICATION_HOST_SHELL_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FINAL_VERIFICATION_HOST_SHELL_EVIDENCE.md)
- [COMPUTATIONAL_SUBSTRATE_FINAL_VERIFICATION_HOST_SHELL_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FINAL_VERIFICATION_HOST_SHELL_DECISION_RECORD.md)

## Primitive Execution Host Outcome

The primitive execution host reassessment is now complete.

It justifies one additional bounded boundary shift:

- the engine now owns the explicit admitted primitive execution plan on the
  bounded slice
- applied admitted mutation-entry results now carry explicit primitive
  execution observation instead of relying only on the narrower raw
  document mutation, primitive realization or rollback, and final
  verification surfaces
- dirty-baseline rollback admitted mutation-entry results now also carry
  explicit primitive execution observation on the bounded slice

It still does not justify broad host independence. Calc still owns:

- the primitive host calls that execute admitted low-level document
  mutation, realization, and rollback work

That means the current boundary is now stronger than the earlier
engine-authored-state-plus-host-primitive split, but it is still narrower
than a broad computational document transplant.

The key primitive execution host closeout references are:

- [COMPUTATIONAL_SUBSTRATE_PRIMITIVE_EXECUTION_HOST_REASSESSMENT_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PRIMITIVE_EXECUTION_HOST_REASSESSMENT_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_PRIMITIVE_EXECUTION_HOST_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PRIMITIVE_EXECUTION_HOST_CONTRACT.md)
- [COMPUTATIONAL_SUBSTRATE_PRIMITIVE_EXECUTION_HOST_SCHEMA.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PRIMITIVE_EXECUTION_HOST_SCHEMA.md)
- [COMPUTATIONAL_SUBSTRATE_PRIMITIVE_EXECUTION_HOST_OBSERVATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PRIMITIVE_EXECUTION_HOST_OBSERVATION.md)
- [COMPUTATIONAL_SUBSTRATE_PRIMITIVE_EXECUTION_HOST_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PRIMITIVE_EXECUTION_HOST_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_PRIMITIVE_EXECUTION_HOST_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PRIMITIVE_EXECUTION_HOST_EVIDENCE.md)
- [COMPUTATIONAL_SUBSTRATE_PRIMITIVE_EXECUTION_HOST_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PRIMITIVE_EXECUTION_HOST_DECISION_RECORD.md)

## Admitted Slice Ownership Closeout Outcome

The admitted-slice ownership closeout is now complete.

It justifies one additional bounded boundary shift:

- the engine now owns the explicit admitted primitive host-call executor
  plan on the bounded slice
- applied admitted mutation-entry results now carry explicit primitive
  host-call executor observation instead of relying only on the narrower raw
  document mutation, primitive execution, realization or rollback, and
  final verification surfaces
- dirty-baseline rollback admitted mutation-entry results now also carry
  explicit primitive host-call executor observation on the bounded slice
- the verified admitted mutation-entry result now consumes that host-call
  executor observation as part of the bounded exactness gate

This closes the remaining substantive ownership seam on the current
admitted slice. Calc still executes the low-level primitive calls on that
slice, but it no longer retains hidden authority over the identity of that
executor shell.

The key admitted-slice ownership closeout references are:

- [ADMITTED_SLICE_OWNERSHIP_CLOSEOUT_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/ADMITTED_SLICE_OWNERSHIP_CLOSEOUT_PLAN.md)
- [ADMITTED_SLICE_OWNERSHIP_CLOSEOUT_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/ADMITTED_SLICE_OWNERSHIP_CLOSEOUT_CONTRACT.md)
- [ADMITTED_SLICE_OWNERSHIP_CLOSEOUT_SCHEMA.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/ADMITTED_SLICE_OWNERSHIP_CLOSEOUT_SCHEMA.md)
- [ADMITTED_SLICE_OWNERSHIP_CLOSEOUT_OBSERVATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/ADMITTED_SLICE_OWNERSHIP_CLOSEOUT_OBSERVATION.md)
- [ADMITTED_SLICE_OWNERSHIP_CLOSEOUT_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/ADMITTED_SLICE_OWNERSHIP_CLOSEOUT_IMPLEMENTATION.md)
- [ADMITTED_SLICE_OWNERSHIP_CLOSEOUT_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/ADMITTED_SLICE_OWNERSHIP_CLOSEOUT_EVIDENCE.md)
- [ADMITTED_SLICE_OWNERSHIP_CLOSEOUT_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/ADMITTED_SLICE_OWNERSHIP_CLOSEOUT_DECISION_RECORD.md)

## Shared-Group Widening Outcome

The shared-group widening workstream is now complete through the broader
non-structural regroup closeout.

It justified additional bounded live rollout expansions and kept the rest of
the shared-group boundary explicit:

- explicit shared-group preserve, rebuild, split, and none classification
  through the workbook facade
- a dedicated admitted shared-group structural preserve/split/rebuild lane
  for bounded same-sheet shareable exact-topology cases
- a dedicated admitted non-structural member-exit lane for scalar, formula,
  and clear
- a further admitted non-structural same-text preserve `SetFormula` lane on
  already-shared same-sheet shareable members
- a further admitted non-structural edge-regroup `SetFormula` lane on
  already-shared same-sheet shareable edge members
- a further admitted non-structural gap-closing merge `SetFormula` lane on
  blank-gap insertion between two same-sheet shareable groups
- a further admitted non-structural edge replacement-merge `SetFormula`
  lane on already-shared same-sheet shareable edge members that absorb one
  adjacent prior shared group
- a further admitted non-structural one-sided adjacent insertion
  `SetFormula` lane on blank cells that extend exactly one same-sheet
  shareable prior group
- deterministic reject and repair-detected handling for classes that stay
  outside that pilot

The key shared-group closeout references are:

- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_WIDENING_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_WIDENING_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_CONTRACT.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_SCENARIO_MATRIX.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_SCENARIO_MATRIX.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_MAPPING_RULES.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_MAPPING_RULES.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_WIDENING_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_WIDENING_EVIDENCE.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_WIDENING_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_WIDENING_DECISION_RECORD.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_ADMISSION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_ADMISSION_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_EVIDENCE.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_DECISION_RECORD.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_FRONTIER_ADMISSION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_FRONTIER_ADMISSION_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_FRONTIER_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_FRONTIER_CONTRACT.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_FRONTIER_SCENARIO_MATRIX.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_FRONTIER_SCENARIO_MATRIX.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_FRONTIER_MAPPING_RULES.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_FRONTIER_MAPPING_RULES.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_FRONTIER_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_FRONTIER_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_FRONTIER_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_FRONTIER_EVIDENCE.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_FRONTIER_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_FRONTIER_DECISION_RECORD.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REGROUP_ADMISSION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REGROUP_ADMISSION_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REGROUP_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REGROUP_CONTRACT.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REGROUP_SCENARIO_MATRIX.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REGROUP_SCENARIO_MATRIX.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REGROUP_MAPPING_RULES.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REGROUP_MAPPING_RULES.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REGROUP_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REGROUP_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REGROUP_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REGROUP_EVIDENCE.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REGROUP_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REGROUP_DECISION_RECORD.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_ADMISSION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_ADMISSION_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_CONTRACT.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_SCENARIO_MATRIX.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_SCENARIO_MATRIX.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_MAPPING_RULES.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_MAPPING_RULES.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_EVIDENCE.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_DECISION_RECORD.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_ADMISSION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_ADMISSION_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_CONTRACT.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_SCENARIO_MATRIX.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_SCENARIO_MATRIX.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_MAPPING_RULES.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_MAPPING_RULES.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_EVIDENCE.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_DECISION_RECORD.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_COMPLETION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_COMPLETION_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_COMPLETION_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_COMPLETION_CONTRACT.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_COMPLETION_SCENARIO_MATRIX.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_COMPLETION_SCENARIO_MATRIX.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_COMPLETION_MAPPING_RULES.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_COMPLETION_MAPPING_RULES.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_COMPLETION_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_COMPLETION_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_COMPLETION_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_COMPLETION_EVIDENCE.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_COMPLETION_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_COMPLETION_DECISION_RECORD.md)

## Next Roadmap Category

The next explicit roadmap category is:

- widening the admitted slice on top of an ownership-complete boundary

The most actionable remaining work is now narrower than another broad
workbook-class push.

The exact regroup, merge, replacement-merge, and merge-completion cycles are
now complete.

The next adjacent concern is:

- whether multi-group collapse, named-range-combined,
  repair-sensitive, off-sheet, or broader non-edge regroup and merge
  shared-group classes can each produce their own exact engine-authored
  promotion family

The most recent closeout references are:

- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REGROUP_ADMISSION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REGROUP_ADMISSION_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REGROUP_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REGROUP_DECISION_RECORD.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_ADMISSION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_ADMISSION_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_DECISION_RECORD.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_ADMISSION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_ADMISSION_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_DECISION_RECORD.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_COMPLETION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_COMPLETION_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_COMPLETION_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_COMPLETION_DECISION_RECORD.md)

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
- plus a successful bounded admitted-slice final-rollback proof
- plus a successful bounded admitted-slice raw-mutation proof
- plus a successful bounded admitted-slice live-apply-shell proof
- plus a successful bounded admitted-slice raw-document-mutation proof
- plus a successful bounded admitted-slice primitive-realization-and-rollback proof
- plus a successful bounded admitted-slice primitive-execution proof
- plus a successful admitted-slice ownership-closeout proof
- plus a successful bounded shared-group structural authority widening proof
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
  slice, admitted object-realization slice, admitted live-apply-shell
  slice, admitted raw-document-mutation slice, and admitted primitive-
  realization-and-rollback slice, admitted primitive-execution slice, and
  admitted ownership-complete slice

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
- [COMPUTATIONAL_SUBSTRATE_RAW_DOCUMENT_MUTATION_API_MIGRATION_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_RAW_DOCUMENT_MUTATION_API_MIGRATION_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_RAW_DOCUMENT_MUTATION_API_DECISION_RECORD.md](architecture/COMPUTATIONAL_SUBSTRATE_RAW_DOCUMENT_MUTATION_API_DECISION_RECORD.md)
- [COMPUTATIONAL_SUBSTRATE_PRIMITIVE_REALIZATION_ROLLBACK_REASSESSMENT_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_PRIMITIVE_REALIZATION_ROLLBACK_REASSESSMENT_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_PRIMITIVE_REALIZATION_ROLLBACK_DECISION_RECORD.md](architecture/COMPUTATIONAL_SUBSTRATE_PRIMITIVE_REALIZATION_ROLLBACK_DECISION_RECORD.md)
- [COMPUTATIONAL_SUBSTRATE_PRIMITIVE_EXECUTION_HOST_REASSESSMENT_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_PRIMITIVE_EXECUTION_HOST_REASSESSMENT_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_PRIMITIVE_EXECUTION_HOST_DECISION_RECORD.md](architecture/COMPUTATIONAL_SUBSTRATE_PRIMITIVE_EXECUTION_HOST_DECISION_RECORD.md)
- [ADMITTED_SLICE_OWNERSHIP_CLOSEOUT_PLAN.md](architecture/ADMITTED_SLICE_OWNERSHIP_CLOSEOUT_PLAN.md)
- [ADMITTED_SLICE_OWNERSHIP_CLOSEOUT_DECISION_RECORD.md](architecture/ADMITTED_SLICE_OWNERSHIP_CLOSEOUT_DECISION_RECORD.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_WIDENING_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_WIDENING_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_WIDENING_DECISION_RECORD.md](architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_WIDENING_DECISION_RECORD.md)
- [README.md](architecture/README.md)

For completed plans, closeout records, and historical extraction context, see:

- [archive/](archive/)
- [extraction-history/](extraction-history/)

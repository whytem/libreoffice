# Engine Entrypoint Adoption Plan

## Purpose

The extraction, replay-promotion, recalc-orchestration, execution-shell,
engine-first adoption, host-boundary consolidation, and token-boundary
reduction programs are complete.

The next implementation stream is to move a bounded set of production Calc
evaluation paths from "Calc orchestrates many engine helpers" to "Calc invokes
engine entry points directly through thin host adapters".

This is not a storage migration, a recalc-boundary rewrite, or a wholesale
replacement of `ScInterpreter`. It is a production-path adoption program.

## What This Workstream Is For

This stream is successful when more Calc production call sites execute
spreadsheet semantics by entering `spreadsheet_engine` at stable, bounded
evaluation entry points while Calc continues to own:

- document storage and mutation
- formula-cell lifecycle and host-side side effects
- document/session services and external-reference cache integration
- UI, import/export, UNO, rendering, shell, and persistence
- threaded and OpenCL backend execution

In concrete terms, the focus is:

- inventorying which production Calc paths are now safe for direct engine entry
  rather than helper-by-helper adoption
- defining thin host adapters that package workbook/document context for those
  entry points
- piloting the first pure-computation and reference-safe direct-entry slices
- removing superseded Calc-local orchestration in the touched scope

## Out Of Scope

This plan does not:

- move `ScDocument` storage or document mutation into the engine
- reopen the recalc authority or queue-construction boundary
- move token ownership or stack mutation out of Calc wholesale
- migrate host-only services such as `INFO(...)`, printer/path inspection, or
  document null-date and holiday expansion into the engine
- redesign threaded or OpenCL backends
- change the zero-fallback replay contract

## Definition Of Done

This workstream is complete when all of the following are true:

1. the in-scope production Calc call sites selected for this stream invoke
   bounded engine evaluation entry points directly by default
2. the required host adapters package context, workbook access, and value-shape
   inputs through named seams rather than repeated ad hoc orchestration
3. superseded Calc-local orchestration in the touched scope is removed or
   isolated behind clearly host-only wrappers
4. the touched retained Calc code reads as host lifecycle, storage, or service
   access rather than duplicate spreadsheet semantics
5. the promoted replay corpus remains at `0` cached fallback under the strict
   summary assertion gate
6. Calc and standalone validation lanes remain green for every adopted slice

## Boundary Rules

Every slice in this stream must continue to respect the current project
boundary:

- keep storage, mutation, and formula-cell lifetime in Calc
- keep host services explicit and localized at the adapter edge
- prefer thin entry adapters over broad new cross-layer plumbing
- prefer one engine-owned semantic implementation for spreadsheet behavior
- do not widen public API only to mirror Calc internals
- do not introduce a second Calc-local semantic implementation as a safety net

## Workstreams

### 1. Freeze The Entrypoint Adoption Inventory

Build and maintain an explicit inventory of production Calc entry points that
are candidates for direct engine entry rather than further helper-level
adoption.

For each candidate, record:

- Calc entry point and owning file
- closest current engine runtime, evaluator, or compat entry surface
- whether the gap is packaging, lifecycle, host service dependency, or true
  semantic missing piece
- whether the correct outcome is `ready now`, `needs small seam`, `host-only`,
  or `defer`
- required validation lanes

Primary target files:

- [interpre.hxx](/home/ubuntu/repos/libreoffice/sc/source/core/inc/interpre.hxx)
- [interpr1.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr1.cxx)
- [interpr2.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr2.cxx)
- [interpr4.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr4.cxx)
- [FormulaEvaluator.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/FormulaEvaluator.hxx)
- compat headers under
  [spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice)

Completion criteria:

- there is no unclassified in-scope direct-entry candidate in the touched
  surface
- each candidate has an owning later slice or an explicit retain/defer reason

### 2. Define Stable Host Adapters For Direct Entry

Converge the minimal host-side context packaging needed to call engine entry
points directly.

Target seam types:

- workbook/document context packaging
- scalar/reference/value-shape adaptation for direct evaluator entry
- explicit host service handoff for the still-host-owned services needed by the
  chosen slice
- result projection from engine values back to Calc stack/result shapes

Implementation rules:

- one explicit entry adapter is better than repeated inline orchestration
- adapters should be thin, named, and vocabulary-stable
- Calc-only types should stop at the adapter edge where practical

Validation:

- focused Calc Cppunit coverage for touched adapter entry points
- standalone coverage if shared helper surfaces change
- `git diff --check`

Completion criteria:

- the chosen direct-entry slices no longer require repeated inline context
  packaging
- the host adapter surface is small enough to audit easily

### 3. Pilot Pure-Computation Direct Entrypoint Adoption

Move the highest-confidence production paths first: bounded pure-computation
surfaces that already have proven engine semantics and low host-service
dependency.

Target categories:

- scalar function families already proven through engine runtime/evaluator
- production Calc call sites that currently still orchestrate these paths
  locally even though engine evaluation entry is now feasible

Implementation rules:

- prefer direct engine entry over adding another Calc-local wrapper
- keep Calc-side argument validation and error/result shape compatible
- do not widen into host-heavy services in this slice

Validation:

- focused Calc formula lanes for the touched functions
- standalone evaluator/runtime coverage for the same semantic surface
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`

Completion criteria:

- the chosen pure-computation production paths route through engine entry
  points by default
- the touched local orchestration tail is removed or reduced to thin adapters

### 4. Pilot Reference-Safe Direct Entrypoint Adoption

After the first pure-computation slice is stable, adopt one bounded
reference-safe slice that uses the already-extracted compat/value-shape
machinery without reopening broad token ownership.

Target categories:

- bounded lookup/reference or inspection paths whose reference-shape and
  host-service dependencies are already explicit
- call sites that already depend on extracted compat seams and now only need a
  thin direct-entry wrapper

Implementation rules:

- keep token ownership and stack mutation in Calc
- reuse existing compat seams rather than adding overlapping ones
- keep host-only lookups explicit where still needed

Validation:

- focused Calc Cppunit coverage on the touched reference paths
- standalone lookup/reference/evaluator coverage
- strict replay summary assertion

Completion criteria:

- at least one bounded reference-safe production path uses direct engine entry
- retained Calc code in the touched area is clearly host orchestration only

### 5. Retire Superseded Calc Orchestration In Scope

Once direct entry is the default path for the selected slices, remove or
isolate the old Calc-local orchestration.

Allowed outcomes:

- delete the local orchestration tail entirely
- keep only a thin host-only wrapper
- retain a helper only if its host-owned dependency is explicit and documented

Not acceptable:

- leaving full duplicate orchestration in place after the engine entry path is
  proven
- keeping ambiguous helper layers whose ownership cannot be explained

Validation:

- targeted Calc coverage on the former caller sites
- standalone coverage if shared entry surfaces changed
- diff hygiene and replay baseline check

Completion criteria:

- there are no silent duplicate orchestration paths in the touched scope
- any retained helper has an explicit host-only rationale

### 6. Lock The Baseline And Close The Stream

Turn the direct-entry adoption result into a stable standing contract.

Required guardrails:

- one-shot `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- focused Calc lanes for every adopted direct-entry slice
- standalone evaluator/runtime lanes for the same surfaces
- doc/status updates that describe the widened direct-entry boundary in present
  tense
- clean diff hygiene

Completion criteria:

- validation is documented and repeatable
- the status docs describe the widened direct-entry boundary as current state
- any further widening work is clearly a new frontier rather than unfinished
  direct-entry adoption cleanup

## Phased Implementation Approach

### Phase 1. Freeze And Publish The Direct-Entry Inventory

Create the explicit candidate table for production Calc entry points that are
now plausible direct engine-entry adopters.

Initial inventory targets:

- pure-computation paths already backed by engine runtime and compat seams
- bounded reference-safe paths already using extracted lookup/reference helpers
- call sites whose remaining gap is mostly host packaging rather than missing
  spreadsheet semantics
- host-heavy services that should be retained and marked as such

Closeout standard:

- the inventory is concrete, classified, and linked from the status docs
- every later phase has a bounded landing surface

### Phase 2. Land The First Entry Adapter Surface

Start with the smallest viable adapter seam that packages Calc state for a
direct engine call.

Target shape:

- one bounded entry adapter
- no ownership transfer
- clear reduction in repeated local orchestration

Closeout standard:

- the touched call sites no longer package entry context inline
- the adapter edge is explicit and easy to trace

### Phase 3. Adopt The First Pure-Computation Production Slice

Move one ready-now pure-computation slice onto direct engine entry.

Target shape:

- bounded function family or entry cluster
- no host-heavy service dependency
- clear before/after removal of local orchestration

Closeout standard:

- the chosen production path routes through engine entry by default
- the touched Calc-local semantic tail is gone or reduced to a thin wrapper

### Phase 4. Adopt The First Reference-Safe Production Slice

Move one bounded reference-safe production slice onto direct engine entry using
the existing compat/value-shape seams.

Target shape:

- one bounded reference-safe cluster
- no token ownership transfer
- no reopening of broad interpreter shell questions

Closeout standard:

- the chosen reference-safe path now uses direct engine entry
- retained local code is visibly host-oriented

### Phase 5. Remove Superseded Local Orchestration

Delete or isolate the old orchestration layers in the touched scope once the
new direct-entry path is proven.

Target shape:

- fewer local wrapper layers
- clearer host-only ownership after removal
- no ambiguous "just in case" fallback layer

Closeout standard:

- no known duplicate orchestration remains in the touched scope
- retained helpers are explicitly host-only

### Phase 6. Re-Run The Full Baseline And Close The Stream

Re-run the standing validation contract, update the status and architecture
docs in present tense, and close the stream.

Closeout standard:

- replay baseline remains at zero fallback
- docs describe the widened direct-entry boundary as stable
- the next frontier is clearly defined

## Validation Contract

Every implementation slice in this stream should validate as applicable with:

- `CppunitTest_sc_ucalc_formula2`
- `CppunitTest_sc_ucalc_shared_cases`
- `CppunitTest_sc_ucalc_dependency_shadow`
- `CppunitTest_sc_ucalc_workbook_facade`
- focused add-in or Calc lanes for touched entry points
- standalone evaluator/runtime/reference tests for touched shared surfaces
- `spreadsheetengine_fods_evaluator_tests`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

## Exit Criteria

This stream can be closed when:

- the selected direct-entry production surfaces are inventoried and classified
- the touched production Calc paths invoke bounded engine entry points by
  default
- the touched host adapters are narrow, explicit, and vocabulary-stable
- no known duplicate orchestration remains in the adopted scope
- the zero-fallback promoted replay baseline is still intact

At that point, the project can reassess the next frontier from a production
Calc that relies more directly on engine entry points instead of helper-level
adoption alone.

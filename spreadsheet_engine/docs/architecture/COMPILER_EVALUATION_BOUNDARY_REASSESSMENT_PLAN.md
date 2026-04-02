# Compiler And Evaluation Boundary Reassessment Plan

Status: active implementation-ready plan

## Purpose

The major extraction, closeout, and boundary-tightening streams are complete:

- replay promotion and zero-fallback closeout
- recalc orchestration extraction
- execution-shell extraction
- engine-first Calc adoption
- host-boundary and token-boundary consolidation
- first-wave and second-wave direct engine-entry widening
- production-boundary tightening
- host-service facade narrowing

What remains in Calc is now mostly intentional host ownership plus a smaller
set of production compiler/evaluation call paths whose best next treatment is
not obvious without a fresh inventory.

The next useful stream is therefore not another historical cleanup pass. It is
an explicit reassessment of the remaining compiler/evaluation boundary so the
project can choose the next bounded production moves from current facts rather
than from legacy phase debt.

## What This Workstream Is For

This stream is successful when the remaining production Calc compiler and
evaluation paths are re-inventoried and each touched candidate is moved into
one of four clearly explained states:

- `ready for direct engine entry`
- `ready for bounded extraction`
- `intentionally host-only`
- `defer`

In practical terms, this stream is for:

- auditing the remaining production compiler/evaluation call paths
- identifying where spreadsheet semantics are already engine-owned but still
  packaged locally in Calc
- tightening production compiler-path use where a shared compiler entry is now
  safe and clearer
- widening direct engine entry for any newly safe bounded slices
- removing superseded caller-side orchestration in the touched scope

## Out Of Scope

This plan does not:

- move `ScDocument` storage or mutation into the engine
- reopen the recalc authority or queue-construction boundary
- move token ownership or stack mutation out of Calc wholesale
- rewrite `ScInterpreter`
- move host-only services such as `INFO(...)`, printer/path inspection, or
  external-reference cache ownership into the engine
- change the zero-fallback replay contract

## Definition Of Done

This workstream is complete when all of the following are true:

1. the remaining in-scope production compiler/evaluation candidates are
   explicitly inventoried and classified
2. at least one compiler-path tightening slice and one direct-entry widening
   slice are either landed or explicitly ruled out by the reassessment
3. the touched Calc callers use named engine-entry or compiler-entry adapters
   rather than repeated inline orchestration
4. superseded local packaging in the touched scope is removed or reduced to
   thin host-only wrappers
5. the promoted replay corpus remains at `0` cached fallback under
   `--assert-zero-fallback`
6. the status docs describe the resulting boundary in present tense and make
   the next frontier explicit

## Boundary Rules

Every slice in this stream must continue to respect the settled project
boundary:

- keep storage, mutation, token ownership, and stack mutation in Calc
- keep host services explicit and localized at the adapter edge
- prefer thin compiler/evaluator entry adapters over broad new plumbing
- prefer one engine-owned semantic implementation for spreadsheet behavior
- do not widen engine API only to mirror Calc internals
- do not introduce a second Calc-local semantic implementation as a fallback

## Workstreams

### 1. Freeze The Remaining Compiler/Evaluation Inventory

Build and maintain an explicit inventory of production Calc compiler and
evaluation call paths whose next treatment is not yet settled.

For each candidate, record:

- Calc entry point and owning file
- closest current engine runtime, evaluator, compiler, or compat entry surface
- whether the remaining gap is packaging, lifecycle, token ownership, host
  service dependency, or a true semantic missing piece
- whether the correct outcome is `ready for direct engine entry`,
  `ready for bounded extraction`, `intentionally host-only`, or `defer`
- required validation lanes

Primary target files:

- [interpre.hxx](/home/ubuntu/repos/libreoffice/sc/source/core/inc/interpre.hxx)
- [interpr1.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr1.cxx)
- [interpr2.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr2.cxx)
- [interpr4.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr4.cxx)
- compiler/host files under
  [spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice)
- compiler tests under
  [spreadsheet_engine/tests/unit](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit)

Completion criteria:

- there is no unclassified in-scope production compiler/evaluation candidate
  in the touched surface
- each candidate has an owning later slice or an explicit retain/defer reason

### 2. Classify The Remaining Boundary By End-State

Turn the inventory into an actionable map of the remaining boundary.

Classification goals:

- separate true host-only services from spreadsheet-semantic packaging still
  happening in Calc
- separate `compiler-path tightening` candidates from `direct-entry widening`
  candidates
- identify any candidates whose only remaining blocker is a small adapter seam
- explicitly mark anything that should stay in Calc for now

Implementation rules:

- use current boundary vocabulary, not historical phase names
- prefer end-state labels over temporary tactical labels
- document why each retained host-owned candidate remains there

Completion criteria:

- the inventory can be read as a next-action map rather than a raw audit log
- there is no ambiguous touched candidate that lacks an end-state label

### 3. Tighten Production Compiler-Path Adoption

Take the first bounded slice where Calc still packages or routes production
compiler work more locally than necessary even though the shared compiler model
is already authoritative.

Target categories:

- direct uses of Calc-local compile orchestration that can instead use a
  shared compile-host entry or adapter
- repeated production compiler setup around external names, string-reference
  handling, or workbook-context packaging
- callers that still stop short of the now-settled shared compiler surface

Implementation rules:

- keep token ownership and final storage updates in Calc
- narrow only the compiler/evaluation entry packaging
- prefer one named compiler-entry adapter over repeated inline setup

Completion criteria:

- the chosen production compiler slice routes through the shared compiler
  surface by default
- the touched local compiler packaging tail is measurably smaller

### 4. Widen Newly Safe Direct Engine Entry

After the reassessment, adopt the highest-confidence newly safe direct-entry
candidate that is not already covered by the completed widening streams.

Target categories:

- production Calc paths whose spreadsheet semantics are already engine-owned
  and whose remaining host packaging is now thin enough to audit
- bounded reference-safe or pure-computation slices discovered by the fresh
  inventory
- add-in or interpreter call sites where Calc now contributes only lifecycle
  or host-service acquisition

Implementation rules:

- prefer direct engine entry over another Calc-local helper layer
- keep host-only services explicit at the adapter edge
- do not widen into heavy token ownership or environment-service surfaces in
  this slice

Completion criteria:

- at least one newly safe direct-entry slice routes through engine entry by
  default
- superseded local orchestration in the touched scope is removed or thinned

### 5. Retire Superseded Caller Orchestration

Once the selected compiler and direct-entry slices land, remove the duplicate
caller-side setup they replaced.

Allowed outcomes:

- delete the superseded wrapper entirely
- keep only a thin host-only facade
- retain a helper only if its host-only reason is explicit and documented

Not acceptable:

- leaving both old and new packaging layers active in the same touched scope
- keeping ambiguous wrappers whose ownership cannot be explained

Completion criteria:

- there is one clear path per touched compiler/evaluation boundary cluster
- no silent duplicate caller orchestration remains in the touched scope

### 6. Lock The Baseline And Close The Stream

Turn the reassessed compiler/evaluation boundary into a stable standing
contract.

Required closeout work:

- rerun focused Calc/add-in and standalone lanes for every touched slice
- rerun one-shot replay with `--assert-zero-fallback`
- update
  [PROJECT_STATUS.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/PROJECT_STATUS.md)
  and [README.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/README.md)
- record what remains intentionally host-only or deferred after the stream

Completion criteria:

- the reassessed compiler/evaluation boundary is documented as current state
- future work is clearly a new bounded frontier rather than unfinished
  reassessment cleanup

## Phased Implementation Approach

### Phase 1. Freeze And Publish The Reassessment Inventory

Build the explicit candidate table for the remaining production
compiler/evaluation boundary.

Frozen Phase 1 inventory:

| Candidate | Owning file(s) | Current boundary question | Frozen inventory read | Validation lanes | Later phase |
| --- | --- | --- | --- | --- | --- |
| bounded formula-string packaging for `CELL(...,"ADDRESS")` and external `CELL(...)` address/file projection | `spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/CellInspectionExecution.hxx` | repeated `ScTokenArray` plus `ScCompiler` setup still exists inside an otherwise named compat seam; should that packaging move behind a dedicated compiler adapter? | spreadsheet semantics are already shared; the remaining gap is compiler-entry packaging, not host service ownership | `CppunitTest_sc_ucalc_formula2`, `spreadsheetengine_fods_evaluator_tests`, replay summary | Phase 3 |
| retained compile-and-lower packaging for `INDIRECT` structured references and quoted external names | `spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/IndirectExecution.hxx` | does the remaining `CompileString()` / `CompileTokenArray()` setup belong in a small compile helper instead of living inline in the evaluation seam? | candidate is in scope for compiler tightening; token ownership still remains in Calc | `CppunitTest_sc_ucalc_formula2`, `spreadsheetengine_reference_tests`, replay summary | Phase 3 |
| date-mode financial add-in callers that still invoke engine semantics through `evaluateFinancialWithDateMode(...)` instead of the direct adapter surface | `scaddins/source/analysis/financial.cxx`, `spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/FinancialAddInExecution.hxx`, `scaddins/source/analysis/analysisdefs.hxx` | is the remaining Calc/add-in work only host context acquisition, making direct adapter adoption the clearer production path? | newly safe direct-entry widening candidate; the adapter surface already exists but is not yet the default path for most date-mode functions | `CppunitTest_scaddins_analysis`, replay zero-fallback lane | Phase 4 |
| retained host-heavy inspection and environment projection | `sc/source/core/tool/interpr1.cxx`, `sc/source/core/tool/interpr5.cxx`, `spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/CellInspectionExecution.hxx`, `spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/InfoInspectionExecution.hxx` | after the previous narrowing streams, is any of the remaining local work still spreadsheet semantic rather than truly host-only? | likely intentional host ownership, but requires an explicit end-state explanation in the reassessment map | `CppunitTest_sc_ucalc_formula2`, `CppunitTest_sc_ucalc_shared_cases`, replay summary | Phase 2 |
| retained external-reference cache and session-backed fetch/projection packaging | `sc/source/core/tool/interpr4.cxx`, `spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ExternalReferenceExecution.hxx`, `spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/CellInspectionExecution.hxx` | can any remaining local packaging be narrowed further without moving cache ownership or document-session services? | expected to stay host-owned unless a very small projection seam emerges; needs explicit classification rather than implicit carry-forward | `CppunitTest_sc_ucalc_formula2`, `CppunitTest_sc_ucalc_shared_cases` | Phase 2 |

Completion criteria:

- the touched candidate set is explicit and current
- every initial candidate has an end-state owner in a later phase

### Phase 2. Classify Ready-Now, Host-Only, And Deferred Paths

Turn the raw inventory into a stable decision map.

Preferred execution order:

1. classify truly host-only paths
2. mark compiler-tightening candidates
3. mark newly safe direct-entry candidates
4. mark any valid but out-of-scope defers

### Phase 3. Land The First Compiler-Path Tightening Slice

Take the highest-confidence production compiler caller whose remaining local
packaging can now route through a shared compiler entry or adapter.

Preferred execution order:

1. converge compiler-host context packaging
2. adopt the tightened production caller
3. add focused compiler/evaluation coverage for the touched path

### Phase 4. Land The First Newly Safe Direct-Entry Slice

Take the best direct-entry candidate surfaced by the reassessment.

Preferred execution order:

1. add or normalize the minimal host adapter
2. switch the production caller onto direct engine entry
3. remove the replaced local orchestration tail

### Phase 5. Remove Superseded Packaging In Scope

Delete or isolate the old orchestration layer in the touched compiler/evaluator
clusters.

Target outcome:

- one named entry path per touched boundary cluster
- no ambiguous dual-path packaging in the touched scope

### Phase 6. Revalidate And Close The Stream

After the selected slices land:

- rerun the standing Calc/add-in and standalone lanes
- rerun one-shot replay with `--assert-zero-fallback`
- update the status and architecture docs in present tense
- record what remains intentionally host-only or deferred after the stream

## Validation Contract

Per-slice default contract:

- focused Calc or add-in coverage for the touched production path
- standalone coverage if shared compiler or evaluator helpers changed
- `spreadsheetengine_fods_evaluator_tests` if evaluator-facing behavior changed
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
  whenever the touched slice changes shared evaluation behavior
- `git diff --check`

Recommended high-signal lanes:

- `CppunitTest_sc_ucalc_formula2`
- `CppunitTest_sc_ucalc_compile_diff`
- `CppunitTest_sc_ucalc_shared_cases`
- `CppunitTest_scaddins_analysis`
- `spreadsheetengine_fods_evaluator_tests`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`

## Exit Criteria

This stream should only be considered complete when:

1. the remaining in-scope production compiler/evaluation boundary is
   explicitly inventoried and classified
2. at least one compiler-path tightening slice and one direct-entry widening
   slice were closed out or explicitly ruled out by current evidence
3. the touched Calc callers use named compiler/evaluator entry adapters rather
   than repeated inline orchestration
4. no known ambiguous caller-side orchestration tail remains in the touched
   scope
5. the promoted replay corpus still reports:
   - `workbooks=500`
   - `formula_cells=50661`
   - `parsed_formulas=50652`
   - `cached_fallback_cells=0`
   - `cached_fallback_rate=0`
6. the status docs describe the reassessed boundary as current state and make
   the next frontier explicit

At that point, the project can decide from current facts whether the next
frontier is:

- another bounded direct-entry widening stream
- another production compiler tightening stream
- or continued maintenance of the now-smaller compiler/evaluation boundary

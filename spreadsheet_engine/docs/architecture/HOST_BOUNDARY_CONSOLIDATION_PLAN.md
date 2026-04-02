# Host Boundary Consolidation Plan

## Purpose

The extraction, replay-promotion, recalc-orchestration, execution-shell, and
bounded engine-first adoption programs are complete.

The next implementation stream is to make the remaining Calc-owned surface
cleaner, narrower, and easier to explain as a true host boundary.

This is not a new replay-promotion program, a storage migration, or a
wholesale interpreter rewrite. It is a boundary-quality program.

## What This Workstream Is For

This plan is successful when the remaining Calc-owned execution-adjacent code:

- is obviously host-only rather than a second spreadsheet-semantics layer
- uses engine-owned vocabulary and compat seams where practical
- carries less Calc-local token/container coupling in shared-semantic paths
- is easier to audit, validate, and extend without reopening settled
  architecture boundaries

In concrete terms, the focus is:

- `ScTokenArray` and token-plumbing boundaries
- Calc/LibreOffice type vocabulary leaking across the engine boundary
- scattered compat logic that should live in narrow adapters
- explicit documentation of what is still intentionally host-owned

## Out Of Scope

This plan does not:

- move document storage out of Calc
- reopen the recalc authority or queue-construction boundary
- replace `ScInterpreter` wholesale
- move UI, UNO, import/export, rendering, or backend execution into the engine
- change the zero-fallback replay contract

## Definition Of Done

This workstream is complete when all of the following are true:

1. the in-scope token/container seams still owned by Calc are explicitly
   classified as `host-only`, `move behind compat`, or `defer`
2. repeated Calc-to-engine vocabulary and adapter patterns in the touched
   surface have been collapsed into named seams
3. in-scope shared-semantic paths no longer depend on ad hoc `ScTokenArray` or
   Calc-local token-plumbing decisions where an engine-owned representation is
   feasible
4. the touched Calc-owned code reads as host infrastructure rather than as a
   shadow spreadsheet-semantics implementation
5. the promoted replay corpus remains at `0` cached fallback under the strict
   summary assertion gate
6. Calc and standalone validation lanes stay green for every slice

## Boundary Rules

Every implementation slice in this stream must continue to respect the settled
project boundary:

- keep `ScDocument`, sheet/cell storage, listener wiring, and document mutation
  in Calc
- keep token-container mutation and formula-tree ownership in Calc unless a
  narrow non-owning seam is being extracted
- prefer thin compat helpers over inlined cross-layer translation
- prefer engine-owned vocabulary for spreadsheet semantics
- keep host service lookups explicit and close to Calc call sites
- avoid introducing new duplicate semantic implementations

## Workstreams

### 1. Freeze The Remaining Host-Boundary Inventory

Build and maintain an explicit inventory of the remaining Calc-owned,
execution-adjacent surface that still matters to the broader extraction
objective.

For each candidate, record:

- file and entry point
- whether it is spreadsheet semantics, token/container glue, or host service
- whether an engine-owned type or helper already exists
- whether the right outcome is `host-only`, `compat seam`, or `defer`
- required validation lanes

Primary target files:

- [interpre.hxx](/home/ubuntu/repos/libreoffice/sc/source/core/inc/interpre.hxx)
- [interpr1.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr1.cxx)
- [interpr2.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr2.cxx)
- [interpr4.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr4.cxx)
- [interpr8.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr8.cxx)
- [PROJECT_STATUS.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/PROJECT_STATUS.md)

Completion criteria:

- there is no unclassified “miscellaneous Calc tail” left in the touched scope
- each inventory item has an owning later slice or an explicit retain/defer
  reason

### 2. Normalize Engine-Facing Vocabulary

Reduce direct exposure of Calc- or LibreOffice-specific vocabulary in surfaces
that are really engine semantics or compat contracts.

Target categories:

- type aliases and value-shape concepts that should use engine-owned API types
- helper signatures that expose Calc-specific types more broadly than needed
- spreadsheet-semantic helpers that can take engine vocabulary plus a narrow
  host adapter instead of Calc-native structures

Implementation rules:

- use engine-owned public or detail types where semantics are shared
- keep Calc-native types at the adapter boundary when host ownership is real
- do not widen public API just to mirror Calc internals

Validation:

- focused Calc coverage for touched call sites
- standalone unit coverage if a shared helper surface changes
- `git diff --check`

Completion criteria:

- the touched surfaces are easier to read without Calc-internal knowledge
- engine-facing helpers no longer expose unnecessary Calc-only vocabulary

### 3. Collapse Token And Container Adapter Sprawl

Concentrate repeated token/container translation patterns into named compat
seams instead of allowing them to stay duplicated across interpreter call
sites.

Target categories:

- repeated `ScTokenArray` inspection patterns
- range/union/container adaptation that feeds shared-semantic helpers
- repeated token-to-engine value-shape translation logic

Implementation rules:

- do not move ownership or mutation out of Calc
- extract only the reusable planning/translation layer
- keep mutation and cursor advancement in Calc if they are still host-shaped

Likely file targets:

- [interpre.hxx](/home/ubuntu/repos/libreoffice/sc/source/core/inc/interpre.hxx)
- [interpr2.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr2.cxx)
- [interpr4.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr4.cxx)
- compat headers under
  [spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice)

Validation:

- `CppunitTest_sc_ucalc_formula2`
- `CppunitTest_sc_ucalc_shared_cases`
- zero-fallback replay summary

Completion criteria:

- the touched translation patterns exist in one named seam rather than at many
  interpreter call sites
- remaining Calc-local code is clearly about ownership/mutation, not semantic
  interpretation

### 4. Isolate Host Service Access

Make document/session/environment service usage explicit and localized.

Target categories:

- document null-date and number-format lookups
- printer/path/file/environment inspection
- external-reference cache and session-backed lookups
- live document services that are intentionally not standalone semantics

Implementation rules:

- host service access should be explicit in naming and file placement
- avoid hiding host lookups inside generic semantic helpers
- if a helper remains Calc-owned, its host-only reason should be obvious

Validation:

- focused Calc Cppunit coverage for touched services
- doc consistency check against touched code
- `git diff --check`

Completion criteria:

- host service access is easier to audit
- touched helpers are clearly host-owned rather than ambiguous utility code

### 5. Retire Residual Duplicate Or Ambiguous Helpers

After compat seams and vocabulary cleanup land, remove or isolate any
remaining in-scope helpers that still duplicate shared behavior or blur the
boundary.

Allowed outcomes:

- delete the local helper
- keep only a thin host wrapper
- retain a helper only if its host-only dependency is explicit

Not acceptable:

- leaving a second semantic implementation behind after the seam is proven
- keeping ambiguous utility helpers whose ownership cannot be explained

Validation:

- focused Calc coverage on the former caller sites
- standalone coverage if the shared helper changed
- zero-fallback replay summary

Completion criteria:

- no known silent duplicate helper remains in the touched scope
- retained helpers in the touched areas have explicit host-only rationale

### 6. Lock The Boundary And Close The Stream

Turn the cleaned host boundary into a stable standing contract.

Required guardrails:

- one-shot `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- focused Calc lanes for every touched seam
- standalone coverage for shared helper changes
- doc/status updates that describe the boundary in present tense
- clean diff hygiene

Completion criteria:

- validation is documented and repeatable
- the status docs describe the remaining Calc-owned surface as an intentional
  host boundary
- any further widening work is clearly a new frontier rather than unfinished
  cleanup from this stream

## Phased Implementation Approach

### Phase 1. Freeze And Publish The Host-Boundary Inventory

Status: complete

Create the explicit inventory table for the remaining token/container,
vocabulary, and host-service boundary.

Initial inventory targets:

- `ScTokenArray`-shaped interpreter glue that is still shared-semantic in
  practice
- Calc-specific vocabulary leaking into compat/helper signatures
- repeated token/container translation code in `interpr2.cxx` and
  `interpr4.cxx`
- host-heavy service helpers that should be explicitly marked as such

Closeout standard:

- the inventory is concrete, classified, and linked from the status docs
- every later phase has a bounded landing surface

Frozen inventory:

| Candidate surface | Current file area | Boundary type | Planned outcome | Validation lanes | Owning later phase |
| --- | --- | --- | --- | --- | --- |
| Bounded `CELL(...)` semantic helper still taking Calc-native address/name inputs | `spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/CellInspectionExecution.hxx`, `sc/source/core/tool/interpr1.cxx` | vocabulary leakage | move helper request surface toward engine-owned API types while keeping Calc grammar/service access in compat | `CppunitTest_sc_ucalc_formula2`, `spreadsheetengine_fods_evaluator_tests`, replay zero-fallback | Phase 2 |
| Ref-list materialization from interpreter stack into single ref or column vector | `sc/source/core/tool/interpr4.cxx` | token/container translation | move planning/materialization seam behind named compat helper; keep stack mutation in Calc | `CppunitTest_sc_ucalc_formula2`, `CppunitTest_sc_ucalc_shared_cases`, replay zero-fallback | Phase 3 |
| Local and external `CELL(...)` host property lookups (`FILENAME`, `FORMAT`, `WIDTH`, `PREFIX`, `PROTECT`, `COLOR`, `PARENTHESES`) | `sc/source/core/tool/interpr1.cxx` | host service access | isolate into explicit host-service compat helpers | `CppunitTest_sc_ucalc_formula2`, replay zero-fallback, diff hygiene | Phase 4 |
| Calc-side document cell-to-engine value conversion duplicated across host bridge and interpreter call sites | `spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/Host.hxx`, `sc/source/core/tool/interpr1.cxx` | ambiguous helper duplication | centralize in one compat helper and remove local duplicate semantics | `CppunitTest_sc_ucalc_formula2`, `spreadsheetengine_fods_evaluator_tests`, replay zero-fallback | Phase 5 |
| `ScRangeFunc`, `ScUnionFunc`, `ScMultiArea`, raw `ScTokenArray` ownership/mutation | `sc/source/core/tool/interpr2.cxx`, `sc/source/core/tool/interpr1.cxx` | Calc token-container ownership | keep host-only and document explicitly; not in scope for this stream | Calc formula Cppunit plus replay baseline only | retain/defer |
| External-reference cache/session lookup services and heavy environment inspection (`INFO(...)`) | `sc/source/core/tool/interpr1.cxx`, external-ref plumbing | host service | keep host-only and make boundary explicit in docs | focused Calc coverage plus replay baseline | Phase 4 / final docs |

### Phase 2. Adopt Engine-Owned Vocabulary In The Easiest Shared Seams

Status: complete

Start with the safest surfaces where semantics are already shared and the
remaining problem is mostly vocabulary/adapter shape.

Target shape:

- low-risk type and helper signature cleanups
- narrow compat wrappers
- no behavior change beyond clearer ownership

Closeout standard:

- touched helper surfaces read in engine vocabulary first
- Calc-specific types are pushed outward toward the adapter edge

Closeout result:

- the bounded `CELL(...)` compat request now crosses the helper boundary as an
  engine-owned `api::CellAddress`, `api::CellValue`, and optional engine
  string sheet token instead of Calc-native address and sheet-name types
- Calc now does the address/name conversion at the call edge in
  `interpr1.cxx`, while `CellInspectionExecution.hxx` reads primarily in
  engine vocabulary

### Phase 3. Land The First Token/Container Compat Seams

Extract the first repeated token/container translation patterns into named
compat helpers while keeping ownership and mutation in Calc.

Target shape:

- one or two bounded translation clusters, not a broad token rewrite
- clear before/after reduction in duplicated interpreter adapter logic

Closeout standard:

- the touched translation logic exists in one seam
- Calc callers are thinner and more obviously host-owned

### Phase 4. Make Host Services Explicit

Rename, document, and isolate the touched host-service helpers so their
ownership is unmistakable.

Target shape:

- explicit host-service naming
- clearer file placement and comments
- doc updates that match the code reality

Closeout standard:

- touched helpers can be explained quickly as host infrastructure
- no semantic/host ambiguity remains in the touched areas

### Phase 5. Remove Residual Duplicate Helpers In Scope

With the seams proven, delete or isolate the no-longer-needed local helpers in
the touched scope.

Target shape:

- fewer duplicate utilities
- clearer ownership after removal

Closeout standard:

- the touched scope has no known silent duplicate helper implementations

### Phase 6. Close The Boundary Stream

Re-run the full standing validation contract, update the status and architecture
docs in present tense, and mark the stream complete.

Closeout standard:

- replay baseline remains at zero fallback
- docs describe a stable host boundary
- the next frontier is clearly defined

## Validation Contract

Every implementation slice in this stream should validate as applicable with:

- `CppunitTest_sc_ucalc_formula2`
- `CppunitTest_sc_ucalc_shared_cases`
- `CppunitTest_sc_ucalc_dependency_shadow`
- `CppunitTest_sc_ucalc_workbook_facade`
- focused add-in or Calc test lanes for touched host services
- standalone helper/runtime tests for touched shared seams
- `spreadsheetengine_fods_evaluator_tests`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

## Exit Criteria

This stream can be closed when:

- the remaining Calc-owned execution-adjacent surface is inventoried and
  classified
- the touched boundary areas use narrower compat seams and clearer engine-owned
  vocabulary
- the touched Calc code is plainly host-only
- no known duplicate helper remains in the in-scope touched boundary
- the zero-fallback promoted replay baseline is still intact

At that point, the project can reassess the next frontier from a cleaner
boundary, rather than carrying forward legacy coupling as implicit technical
debt.

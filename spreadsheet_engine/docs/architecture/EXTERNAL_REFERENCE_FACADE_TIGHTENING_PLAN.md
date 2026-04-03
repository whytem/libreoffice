# External Reference Facade Tightening Plan

Status: implementation-ready plan

## Purpose

The major extraction, replay-promotion, recalc, execution-shell, engine-first
adoption, token-boundary, and production-boundary streams are complete.

The remaining Calc-owned surface is now mostly intentional host ownership.
Within that retained boundary, the clearest next bounded candidate is the
external-reference path:

- external cache/session lookup still belongs to Calc
- returned token containers still belong to Calc
- but some fetch/projection and `CELL(...)`-specific shaping still sits close
  to production callers

This plan narrows that external-reference facade without trying to move cache
ownership or token ownership into `spreadsheet_engine/`.

## What This Workstream Is For

This stream is successful when the retained external-reference execution path
is easier to explain as:

- Calc owns external-reference cache/session access
- Calc owns token-container lifetime and stack mutation
- compat seams own the repeated fetch/projection packaging
- engine/shared helpers continue to own spreadsheet semantics where they
  already do

In practical terms, the focus is:

- external `CELL(...)` inspection and projection
- external single-ref and double-ref fetch/projection setup
- repeated external value/address/file/format shaping
- explicit retain/defer decisions for any external path that remains fully
  host-owned

## Out Of Scope

This plan does not:

- move external-reference cache ownership into the engine
- move document/session lookup ownership into the engine
- move `ScTokenArray` ownership, stack mutation, or token cursor ownership out
  of Calc
- reopen the recalc authority boundary
- attempt a broad `ScInterpreter` rewrite
- change the zero-fallback replay contract

## Definition Of Done

This workstream is complete when all of the following are true:

1. the in-scope external-reference production paths are explicitly inventoried
   and classified
2. repeated external fetch/projection packaging in the touched scope is
   collapsed into named compat facades or clearly retained host-only wrappers
3. the external `CELL(...)` path is either narrowed further behind a named seam
   or explicitly retained with a documented host-only reason
4. touched external single-ref and double-ref callers stop mixing cache access
   with ad hoc projection logic when a named seam exists
5. the promoted replay corpus still reports `0` cached fallback under the
   strict summary assertion gate
6. Calc and standalone validation lanes stay green for every touched slice

## Boundary Rules

Every slice in this stream must respect the settled project boundary:

- keep external-reference cache/session ownership in Calc
- keep token-container ownership and stack mutation in Calc
- narrow packaging and projection only
- prefer one named compat facade per external-reference cluster
- do not create Calc-local semantic fallbacks
- document every retained host-only wrapper explicitly

## Workstreams

### 1. Freeze The Remaining External-Reference Inventory

Build an explicit inventory of the retained external-reference production paths
whose packaging still looks broader than necessary.

For each candidate, record:

- owning Calc file and entry point
- whether the path is primarily cache access, projection, token adaptation, or
  spreadsheet semantics
- closest existing compat/helper surface
- whether the right outcome is `ready now`, `needs small seam`,
  `intentionally host-only`, or `defer`
- required validation lanes

Primary target files:

- [interpr1.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr1.cxx)
- [interpr4.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr4.cxx)
- [CellInspectionExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/CellInspectionExecution.hxx)
- [ExternalReferenceExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ExternalReferenceExecution.hxx)

Completion criteria:

- there is no unclassified in-scope external-reference candidate in the
  touched surface
- each candidate has an owning later slice or an explicit retain/defer reason

Initial inventory table:

| Candidate | Primary location | Current issue | Likely outcome | Validation lanes |
| --- | --- | --- | --- | --- |
| external `CELL(...)` caller and projection tail | [interpr1.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr1.cxx) | external cache access, `CELL(...)` request assembly, and property/result projection still meet in one production caller | Phase 3 narrowing candidate | `CppunitTest_sc_ucalc_formula2`, `CppunitTest_sc_ucalc_shared_cases`, replay summary |
| external single-ref fetch plus token/format packaging | [interpr4.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr4.cxx) | fetch setup is named, but the returned token/format packaging is still shaped directly around interpreter methods | Phase 4 narrowing candidate | `CppunitTest_sc_ucalc_formula2`, replay summary |
| external double-ref fetch plus matrix/token-array packaging | [interpr4.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr4.cxx) | token-array fetch is named, but matrix extraction and final caller shaping still live inline in Calc | Phase 4 narrowing candidate | `CppunitTest_sc_ucalc_formula2`, `spreadsheetengine_reference_tests`, replay summary |
| external filename/address/value-shape helper tail | [CellInspectionExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/CellInspectionExecution.hxx) | most semantics are already compat-owned, but the remaining external property split is not yet the smallest practical seam | Phase 3 / 5 cleanup candidate | `CppunitTest_sc_ucalc_formula2`, replay summary |

### 2. Classify The External-Reference Boundary Map

Turn the inventory into an actionable boundary map.

Required outcomes:

- separate true cache/session ownership from fetch/projection packaging
- identify the best next extraction target in the external `CELL(...)` path
- identify the best next target in the single-ref/double-ref fetch shell
- mark any path whose remaining value is too host-shaped as `intentionally
  host-only` or `defer`

Expected candidate classes:

- external `CELL(...)` projection paths in [interpr1.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr1.cxx)
- external single-ref token fetch in [interpr4.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr4.cxx)
- external double-ref token-array fetch/projection in [interpr4.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr4.cxx)
- any remaining format/file/address projection helpers adjacent to the above

Completion criteria:

- each touched candidate has one explicit disposition
- the next implementation slices are chosen from the highest-confidence
  packaging candidates, not from broad host-owned internals

### 3. Narrow External `CELL(...)` Projection Packaging

Take the external `CELL(...)` path first if the boundary map confirms that the
remaining gap is mostly result projection and not cache/session ownership.

Likely focus:

- external filename/address/value-shape projection
- external format/color/parentheses shaping
- caller-side assembly in [ScCellExternal()](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr1.cxx#L2518)

Implementation rules:

- keep external cache lookup and document/session access in Calc
- move repeated projection logic behind a named compat seam where practical
- retain any remaining host-only tail explicitly if it still depends on live
  Calc-owned cache/document services

Validation:

- `CppunitTest_sc_ucalc_formula2`
- `CppunitTest_sc_ucalc_shared_cases`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

Completion criteria:

- the touched external `CELL(...)` caller is smaller and more obviously
  host-owned
- projection semantics are no longer duplicated inline in the caller

### 4. Narrow External Single-Ref And Double-Ref Fetch/Projection Packaging

Target the external reference fetch shell around `PopExternalSingleRef`,
`PopExternalDoubleRef`, and `GetExternalDoubleRef`.

Likely focus:

- repeated error handling and projection setup
- repeated conversion between external cache fetches and Calc-owned token
  results
- any shared fetch/projection rules that belong in
  [ExternalReferenceExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ExternalReferenceExecution.hxx)

Implementation rules:

- keep cache lookup and token-array ownership in Calc
- extract only the repeated packaging/projection layer
- keep remaining Calc code obviously about cache/session ownership and final
  token/container handling

Validation:

- `CppunitTest_sc_ucalc_formula2`
- `CppunitTest_sc_ucalc_shared_cases`
- `spreadsheetengine_reference_tests`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

Completion criteria:

- the touched external fetch callers use named compat seams for packaging
- remaining Calc-local code reads as cache/session/token ownership only

### 5. Retire Superseded External Wrappers In Scope

Once the new seams are in place, remove or isolate the old packaging tail.

Allowed outcomes:

- delete the superseded wrapper entirely
- keep only a thin host-only wrapper
- retain a helper only if its host-only rationale is explicit and documented

Not acceptable:

- leaving both old and new external packaging layers active in the same scope
- keeping ambiguous wrappers whose ownership cannot be explained

Completion criteria:

- there is one clear facade path per touched external-reference cluster
- no silent duplicate external packaging remains in the touched scope

### 6. Lock The Baseline And Close The Stream

Turn the narrowed external-reference boundary into a stable standing contract.

Required closeout work:

- rerun focused Calc coverage for every touched external-reference slice
- rerun standalone reference/evaluator lanes if shared helpers change
- rerun one-shot replay with `--assert-zero-fallback`
- update status and architecture docs in present tense
- record explicit retains and defers for any remaining external host-only tail

Completion criteria:

- the zero-fallback replay baseline remains intact
- docs describe the remaining external-reference boundary as a deliberate host
  seam rather than an open cleanup tail

## Recommended Execution Order

Run the stream in this order:

1. freeze the external-reference inventory
2. classify the external boundary map
3. narrow external `CELL(...)` projection if it remains the highest-confidence
   packaging candidate
4. narrow single-ref and double-ref fetch/projection packaging
5. retire superseded wrappers
6. rerun the full validation contract and close the stream

## Validation Contract

Every implementation slice in this stream should, at minimum, run the focused
lanes that cover the touched surface plus diff hygiene:

- `CppunitTest_sc_ucalc_formula2`
- `CppunitTest_sc_ucalc_shared_cases`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

When shared reference helpers or projection logic change, also run:

- `spreadsheetengine_reference_tests`
- `spreadsheetengine_fods_evaluator_tests`

If the add-in or broader host-service surface is touched incidentally, also
run:

- `CppunitTest_scaddins_analysis`

## Exit Criteria

This stream is fully closed when:

1. the remaining external-reference production boundary has been explicitly
   classified into `compat seam`, `host-only`, or `defer`
2. the selected external packaging slices are moved behind named compat seams
3. remaining Calc-local external-reference code is plainly about
   cache/session/token ownership
4. any retained external `CELL(...)` tail has an explicit host-only rationale
5. the promoted replay baseline remains exactly:
   - `500` workbooks
   - `50,661` formula cells
   - `50,652` parsed formulas
   - `0` cached-fallback cells
   - `0` cached-fallback rate
6. status and architecture docs describe the external-reference boundary in
   present tense rather than as implied unfinished cleanup

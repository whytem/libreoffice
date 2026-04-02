# Deepen Engine-First Execution Inside Calc

## Purpose

The extraction, replay-promotion, recalc-orchestration, execution-shell, and
initial engine-first adoption programs are complete.

The next implementation stream is to widen production Calc use of the
engine-owned computation surface that is already proven by:

- standalone evaluator/runtime validation
- Calc-side bridge and add-in tests
- the promoted replay corpus with zero cached fallback

This document is the detailed implementation plan for that work.

## What "Deeper Engine-First Execution" Means

This program is successful when more Calc production call sites execute
spreadsheet semantics through `spreadsheet_engine` by default, while Calc
continues to own:

- storage and mutation
- formula-cell lifecycle
- document/session services
- UI, UNO, import/export, rendering, and shell integration

This is not a storage migration, a recalc-boundary rewrite, or a wholesale
`ScInterpreter` rewrite.

## Definition Of Done

The program is complete when all of the following are true:

1. the remaining non-host spreadsheet-semantic call sites selected for this
   stream route through engine-owned runtime or compat helpers by default
2. any Calc-local code retained in the touched areas is explicitly host-shaped
   and easy to explain
3. legacy Calc-local copies of touched spreadsheet semantics are removed or
   clearly isolated behind host-only seams
4. the promoted replay corpus remains at `0` cached fallback under the strict
   summary assertion gate
5. Calc and standalone validation lanes remain green for every widened
   engine-first slice

## Workstreams

### 1. Freeze The Remaining Adoption Inventory

Build and maintain a concrete inventory of the production Calc entry points
that still execute spreadsheet semantics through Calc-local code even though a
shared engine-owned path already exists or is close enough to adopt.

For each candidate, capture:

- Calc entry point and owning file
- existing engine-owned runtime or compat helper, if any
- whether the remaining gap is semantic, adapter-related, or purely host-owned
- required validation lanes
- whether the candidate is safe for default-path adoption

Priority buckets:

- `Ready now`: shared path already exists and is validated
- `Needs small seam`: shared path exists but Calc adapter shape is too noisy
- `Host-only`: retained in Calc on purpose
- `Defer`: valid future work, but too broad for this stream

Completion criteria:

- every candidate in the chosen adoption surface is classified
- every in-scope item has an owning slice and validation contract
- every touched defer item is documented explicitly, not left implicit

### 2. Expand Engine-Owned Runtime Adoption In Production Calc Paths

Use the inventory to move more real Calc call sites onto engine-owned runtime
helpers by default.

Target categories:

- remaining add-in financial/date/time/information paths that already have
  engine runtime equivalents
- Calc interpreter call sites that already have compat bridges but still keep
  broader local branch copies than necessary
- bounded inspection and helper paths where standalone semantics are already
  authoritative

Implementation rules:

- prefer changing the production call site to use the shared helper rather than
  adding a second wrapper layer
- do not keep both a Calc-local semantic copy and an engine-owned semantic
  implementation in the same adopted slice unless one is clearly host-only
- keep argument validation behavior Calc-shaped

Per-slice validation:

- focused Calc Cppunit coverage for the touched entry points
- standalone evaluator/runtime coverage for the touched helper surface
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`

Completion criteria:

- all `Ready now` inventory items in the chosen surface use engine-owned paths
- touched slices have fixed-result tests on both Calc and standalone sides
- no replay regressions

### 3. Concentrate Compat And Adapter Seams

Where Calc still reaches shared behavior through multiple ad hoc translation
patterns, converge those paths onto smaller compat seams.

Target seam types:

- null-date, basis, locale, and calendar normalization
- scalar/reference/value-shape adaptation between Calc and engine types
- document-service lookup handoff for host-owned services
- workbook/service context packaging for engine entry points

Implementation rules:

- one explicit seam is better than repeated inline translation
- keep adapters thin and vocabulary-stable
- do not let Calc-only types bleed deeper into engine-owned code

Per-slice validation:

- focused Calc/add-in tests for the adapted entry points
- standalone regression tests if the shared seam is also used there
- `git diff --check`

Completion criteria:

- repeated host-to-engine translation patterns in the touched surface are
  collapsed into named helpers
- compat seams are narrower and easier to trace than before

### 4. Retire Legacy Calc-Local Semantic Copies

Once a shared path is the default production path, remove or isolate the old
Calc-local semantic copy.

Allowed outcomes:

- delete the old local path entirely
- keep only a thin host-only wrapper
- retain a clearly marked fallback only if a proven host-only dependency makes
  it necessary

Not acceptable:

- leaving a full Calc-local semantic copy in place "just in case" after the
  shared path is proven
- keeping duplicate validation burden for the same spreadsheet semantics

Per-slice validation:

- targeted Calc unit coverage on the former local path callers
- replay baseline check
- diff hygiene check

Completion criteria:

- there are no silent duplicate implementations in the touched scope
- remaining local code has an explicit host-only reason

### 5. Make The Host Boundary Explicit In Code And Docs

For anything still retained in Calc, make the boundary obvious in both code and
architecture docs.

The retained Calc-owned surface should be limited to things like:

- document/session lookup services
- token-container ownership and mutation
- external-reference cache integration
- host-heavy inspection/environment services
- printer/path/number-format and similar document-service integrations

Implementation rules:

- use names/comments/helpers that signal host-only ownership
- move ambiguous helper names toward explicit host-service naming
- update architecture docs whenever a boundary line changes

Per-slice validation:

- code review pass on touched boundary helpers
- Calc test coverage where host-only seams are exercised
- doc consistency with code reality

Completion criteria:

- retained Calc-local logic in the touched scope reads as host-only
- no touched helper remains ambiguous about whether it is semantic or host
  infrastructure

### 6. Lock The Baseline And Close The Stream

Turn the zero-fallback replay baseline and the widened engine-first path into a
stable standing contract.

Required guardrails:

- strict replay summary assertion:
  `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- focused Calc lanes for the widened engine-owned surfaces
- standalone evaluator/runtime lanes for the same surfaces
- clean diff hygiene

Completion criteria:

- validation is documented and repeatable
- status/architecture docs describe the widened engine-first boundary in
  present tense
- remaining work is clearly a new frontier, not unfinished adoption cleanup

## Phased Implementation Approach

### Phase 1. Freeze And Publish The Adoption Inventory

Status: complete

The first phase is to freeze the remaining candidate surface and make the
adoption ranking explicit in this document before implementation continues.

Initial candidate inventory:

| Candidate | Calc entry point(s) | Shared helper status | Classification | Validation lanes |
| --- | --- | --- | --- | --- |
| Analysis add-in `WORKDAY` / `NETWORKDAYS` | `scaddins/source/analysis/analysis.cxx` | Engine `api::workday` helpers already exist | `Ready now` | `CppunitTest_scaddins_analysis`, `spreadsheetengine_calendar_tests`, replay zero-fallback |
| Analysis add-in `WEEKNUM` | `scaddins/source/analysis/analysis.cxx` | Engine `api::calendar::weeknumOOo` already exists | `Ready now` | `CppunitTest_scaddins_analysis`, `spreadsheetengine_calendar_tests`, replay zero-fallback |
| Analysis add-in `EDATE` / `EOMONTH` | `scaddins/source/analysis/analysis.cxx` | Runtime month-shift helper exists but needs API seam | `Needs small seam` | `CppunitTest_scaddins_analysis`, `spreadsheetengine_calendar_tests`, replay zero-fallback |
| Add-in TBILL guard logic | `scaddins/source/analysis/financial.cxx` | Engine `api::calendar::diffDate360` exists | `Ready now` | `CppunitTest_scaddins_analysis`, replay zero-fallback |
| Add-in holiday/null-date normalization | `scaddins/source/analysis/analysisdefs.hxx` | Repeated inline translation remains | `Needs small seam` | `CppunitTest_scaddins_analysis`, diff hygiene |
| Legacy analysis helper financial/date copies | `scaddins/source/analysis/analysishelper.[ch]xx` | Many touched semantics already run through engine runtime | `Ready after adoption` | `CppunitTest_scaddins_analysis`, replay zero-fallback |
| Holiday expansion from live document inputs | `scaddins/source/analysis/analysisdefs.hxx` | Requires document-facing add-in services | `Host-only` | `CppunitTest_scaddins_analysis` |
| Null-date lookup from live document properties | `scaddins/source/analysis/analysisdefs.hxx` | Requires live host document state | `Host-only` | `CppunitTest_scaddins_analysis` |
| External-reference cache, token containers, formula-tree ownership | Calc core | No engine-owned replacement for this stream | `Defer` | none in this stream |

Completion target for this phase:

- the inventory above matches the actual next landing surface
- each in-scope item is assigned to one of the phases below
- host-only and defer items are explicit rather than implicit

### Phase 2. Adopt Ready-Now Runtime Paths

Status: pending

Land the highest-confidence production-path adoptions first:

- move analysis add-in `WORKDAY` / `NETWORKDAYS` onto engine `api::workday`
- move analysis add-in `WEEKNUM` onto engine `api::calendar::weeknumOOo`
- replace the local TBILL day-count guard with engine `api::calendar::diffDate360`

Closeout standard:

- Calc production callers use the engine-owned path by default
- focused add-in tests prove the production path
- strict replay summary stays at zero fallback

### Phase 3. Add The Missing Small Seams

Status: pending

Close the small adapter gaps exposed by Phase 2:

- publish a clean engine-facing month-shift seam for `EDATE` / `EOMONTH`
- add named helpers for holiday-list conversion and weekend-mask packaging
- reduce repeated null-date to `DateParts` translation in touched add-in code

Closeout standard:

- repeated translation patterns collapse into named helpers
- add-in date functions can use shared helpers without reintroducing local
  semantic code

### Phase 4. Remove Superseded Local Semantic Copies

Status: pending

Once the shared paths are the default production path, delete or isolate the
now-obsolete local copies in `analysishelper.[ch]xx`.

Expected landing scope:

- dead `GetYearFrac` / `GetDiffDate360` wrappers if no longer referenced
- dead local financial helpers already superseded by `FinancialRuntime`
- dead local month/date helpers superseded by the new shared seams

Closeout standard:

- no touched local helper remains only as a silent semantic duplicate
- any retained helper has a concrete host-only or deferred reason

### Phase 5. Make The Remaining Host Boundary Explicit

Status: pending

After local semantic copies are retired, make the retained Calc/add-in code read
clearly as host-only infrastructure.

Expected landing scope:

- explicit helper naming for live-document null-date lookup
- explicit helper naming for holiday expansion from UNO/add-in inputs
- doc wording that matches the retained code boundary

Closeout standard:

- the remaining local helpers in the touched surface are obviously host-shaped
- code and docs describe the same boundary

### Phase 6. Revalidate And Close The Stream

Status: pending

Run the full standing validation contract, then close the stream in the docs as
completed rather than active.

Closeout standard:

- full validation contract is green
- zero-fallback replay remains enforced
- the docs describe the deepened engine-first boundary in present tense
- any remaining work is a genuinely new frontier

## Recommended Execution Order

1. Freeze the inventory and rank candidates.
2. Land the highest-confidence runtime adoption slices.
3. Collapse the adapter seams those slices expose.
4. Remove the superseded Calc-local semantic copies.
5. Mark retained local code as host-only where appropriate.
6. Re-run the full validation contract and close the stream in docs.

This order matters. Inventory first prevents random opportunistic edits, and
legacy-path deletion should happen only after the shared path is proven in
production callers.

## Slice Template

Every implementation slice in this stream should answer these questions before
it lands:

- What Calc production entry point is changing?
- What engine-owned helper becomes the default path?
- What remains host-only after this slice?
- What legacy local path is removed or narrowed?
- What focused Calc tests were added or updated?
- What standalone tests were added or updated?
- Did the strict replay summary stay at zero fallback?

If a slice cannot answer those questions clearly, it is probably too broad or
insufficiently scoped.

## Validation Contract

### Per-Slice Minimum

- the smallest relevant Calc Cppunit lane for the touched area
- the smallest relevant standalone evaluator/runtime lane for the touched area
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

### Recommended Focused Calc Lanes

Use the narrowest lanes that match the touched area:

- `CppunitTest_scaddins_analysis`
- `CppunitTest_sc_ucalc_formula2`
- `CppunitTest_sc_ucalc_shared_cases`
- `CppunitTest_sc_ucalc_dependency_shadow`
- `CppunitTest_sc_ucalc_workbook_facade`

### Recommended Standalone Lanes

- `spreadsheetengine_fods_evaluator_tests`
- `spreadsheetengine_execution_tests`
- `spreadsheetengine_lookup_tests`
- `spreadsheetengine_reference_tests`

Only run the lanes that materially cover the slice, but always keep the strict
replay-summary gate.

### Final Closeout Validation

Before calling this stream complete, run:

- all touched Calc Cppunit lanes
- all touched standalone lanes
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

Record the exact replay baseline at closeout.

## Exit Criteria

The stream is ready to close when all of the following are true:

- the chosen in-scope production Calc call sites route through engine-owned
  helpers by default
- no touched area still contains an unexplained Calc-local semantic duplicate
- retained Calc-local code in the touched areas is explicitly host-only
- the strict replay-summary gate remains green at zero cached fallback
- status and architecture docs describe the new boundary cleanly

## Explicit Defer List

The following are not required to close this stream:

- moving document storage into the engine
- changing the settled recalc authority boundary
- broad `ScInterpreter` rewrites
- UI, UNO, import/export, rendering, or threaded/OpenCL migration
- host-heavy services that remain document- or application-specific

Those are valid future topics, but they are not needed to finish deepening
engine-first execution inside Calc.

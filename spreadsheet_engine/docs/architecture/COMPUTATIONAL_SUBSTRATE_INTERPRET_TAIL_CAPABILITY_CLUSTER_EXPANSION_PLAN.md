# Computational Substrate InterpretTail Capability-Cluster Expansion Plan

Status: completed closeout for the second live evaluator switchover wave

## Purpose

This plan defines the next evaluator-migration wave after the completed
literal-only `InterpretTail` switchover pass.

The goal is to turn the first narrow live delegation seam into a materially
useful production evaluation lane by expanding it cluster-by-cluster rather
than function-by-function or substrate-conjunction-by-conjunction.

This is the next real authority-transfer step, not another verification-only
closeout cycle.

## Closeout Result

This pass is now complete.

The second live evaluator wave widened the real `InterpretTail` delegation
surface from literal-only text parsing to a bounded host-backed capability
cluster:

- `VALUE`, `DATEVALUE`, `TIMEVALUE`, and `NUMBERVALUE` now accept bounded
  host-backed scalar inputs:
  - literals
  - single-cell references
  - single-cell global and sheet-local names
  - simple scalar expression trees built from concatenation and bounded
    unary/binary scalar operators
- the first bounded lookup/index authority cluster now routes through the
  same live seam for:
  - `MATCH`
  - `XMATCH`
  - `LOOKUP`
  - `VLOOKUP`
  - `HLOOKUP`
  - `INDEX`
- authoritative projection now carries string results in addition to numeric
  and error outputs
- per-function observe, shadow, authoritative, and fallback counters now
  exist on the live seam
- fallback reasons now distinguish unsupported host-surface restrictions from
  unsupported formula shape and unsupported function

The real Calc-path retirement bar for this pass is met in the bounded sense
defined by the plan:

- in `authority` mode, the promoted text-parsing and bounded lookup/index
  clusters are now engine-first and explicit-Calc-fallback inside
  [ScFormulaCell::InterpretTail](/home/ubuntu/repos/libreoffice/sc/source/core/data/formulacell.cxx)

The retained deferred boundary for this wave is:

- `XLOOKUP`
- matrix or spill-returning lookup/index shapes
- multi-cell or slice-valued lookup results
- external references, add-ins, macros, DDE, and volatile environment
  surfaces
- workbook-wide default-on delegation

The immediate follow-on pass for this completed wave is:

- [COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_CORPUS_AUTHORITATIVE_USAGE_PLAN.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_CORPUS_AUTHORITATIVE_USAGE_PLAN.md)

That follow-on is not another capability-wave expansion yet.
The Calc-backed replay runner showed that the current corpus still records
`0` authoritative routes and `303` `parse_failure`, so the next job is to
make the existing promoted cluster produce measurable authoritative corpus
usage first.

## Why This Wave Next

The first switchover pass already proved the hard foundational facts:

- real AutoCalc `Observe` now runs inside
  [ScFormulaCell::InterpretTail](/home/ubuntu/repos/libreoffice/sc/source/core/data/formulacell.cxx)
- `ShadowCompare` can compare engine and Calc on that same live seam
- `AuthoritativeWithFallback` can bypass `ScInterpreter` and project engine
  results directly into `ScFormulaCell`
- support, fallback, and mismatch telemetry already exists

The current blocker is no longer “there is no live evaluator seam.”
The blocker is that the delegated family is still too narrow to matter.

The next move should therefore expand by capability cluster:

- host-backed scalar materialization
- workbook-local references and names
- bounded lookup and index-style reads

That has more leverage than another bounded substrate widening pass.

## Strategic Objective

Turn the current literal-only text-parsing lane into a broader, host-backed
interactive evaluation cluster that can plausibly retire real Calc-owned
evaluation logic.

This wave should be considered successful only if it materially increases the
share of real `InterpretTail` invocations that can:

- classify as engine-supported
- run in `ShadowCompare`
- run authoritatively with explicit fallback

## Target Capability Envelope

This plan is intentionally ambitious. The target is not one extra formula
shape. The target is a whole capability wave.

### Primary Target: Host-Backed Scalar Input Expansion

Expand the current text-parsing family from literal-only arguments to
workbook-local scalar inputs:

- single-cell references
- workbook-global and sheet-local names that resolve to one cell
- simple scalar expression trees that collapse to one scalar text or number
  input before the runtime function call
- direct scalar coercions already representable in shared engine runtime

The first function cluster remains:

- `VALUE`
- `DATEVALUE`
- `TIMEVALUE`
- `NUMBERVALUE`

But the supported argument surface should widen substantially.

### Secondary Target: Bounded Lookup/Index Cluster

Add the first bounded lookup-read family to the same live `InterpretTail`
seam using existing host-backed runtime helpers.

Recommended bounded lookup target:

- workbook-local same-workbook references only
- single-area ranges only
- no external references
- no DDE, macros, add-ins, or volatile environment surfaces
- no matrix/spill-only semantics

Recommended function set:

- `MATCH`
- `XMATCH`
- `LOOKUP`
- `INDEX`
- `VLOOKUP` and `HLOOKUP`

Optional stretch target if the same host-backed model closes cleanly:

- `XLOOKUP`

### Tertiary Target: First Calc Path Retirement

This pass should not close as “runtime seam expanded, docs updated” only.

At least one of the following must happen before the pass can be called
complete:

- one Calc evaluator family is permanently bypassed in `authority` mode
  across the whole bounded cluster
- one Calc evaluation branch becomes hard-quarantined behind the fallback
  boundary rather than the primary path
- one meaningful piece of duplicated Calc-side evaluator glue is deleted or
  demoted because the engine path is now authoritative for that cluster

## Non-Goals

Still out of scope for this wave:

- workbook-wide default-on authority
- broad matrix/spill behavior
- add-ins, macros, DDE, WebService, or printer/path/document-service
  integrations
- external-reference evaluation
- broad shared-group mutation authority
- workbook-wide dependency or storage relocation

## Current Technical Starting Point

The current live seam is:

- [InterpretTailEngineEvaluator.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/InterpretTailEngineEvaluator.hxx)
- [formulacell.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/data/formulacell.cxx)

The key host-backed evaluation surfaces already available are:

- [Host.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/Host.hxx)
- [FormulaEvaluator.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/FormulaEvaluator.hxx)
- [ReferenceExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ReferenceExecution.hxx)
- [LookupExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/LookupExecution.hxx)
- [TextParsingExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/TextParsingExecution.hxx)

The gap is that the live seam still hand-classifies only literal-only AST
shapes instead of delegating through a broader host-backed evaluator surface.

## Migration Strategy

Keep fallback-first delegation.

The engine should become authoritative for supported formulas in this wave.
Calc remains the fallback for:

- unsupported formula shapes
- unsupported host surfaces
- mismatch-producing routes in `shadow`
- temporarily deferred runtime families

This wave should expand support clusters aggressively, but it should not
require exact-or-rollback equivalence before any new authority lands.

## Workstreams

### 1. Freeze A Capability-Cluster Contract

Define the new delegated families in cluster terms, not narrow conjunctions.

The contract should describe:

- scalar input classes:
  - literal
  - single-cell reference
  - single-cell name
  - simple scalar expression
- bounded lookup classes:
  - scalar lookup key
  - single-area search range
  - same-workbook result range
- authoritative fallback conditions
- mismatch classes
- measurable retirement criteria

Required result:

- one frozen cluster contract for text-parsing expansion
- one frozen bounded lookup contract
- one explicit statement of what counts as real Calc-path retirement

### 2. Build A Host-Backed Scalar Materializer

Introduce a reusable live materialization layer for scalar function arguments.

That layer should be able to:

- resolve single-cell references through host access
- resolve single-cell names through the host-backed evaluation model
- evaluate bounded scalar expression trees that compose literals, references,
  names, concatenation, and simple unary/binary coercions
- return a normalized scalar input object suitable for the delegated runtime
  family

Primary code surfaces:

- [InterpretTailEngineEvaluator.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/InterpretTailEngineEvaluator.hxx)
- [Host.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/Host.hxx)
- [FormulaEvaluator.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/FormulaEvaluator.hxx)
- [ReferenceExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ReferenceExecution.hxx)

Required result:

- literal-only classification disappears as the main architectural boundary
- the current text-parsing family can consume host-backed scalar inputs

### 3. Expand The Text-Parsing Authority Cluster

Promote `VALUE`, `DATEVALUE`, `TIMEVALUE`, and `NUMBERVALUE` from
literal-only routing to bounded host-backed scalar routing.

Target supported examples:

- `=DATEVALUE(A1)`
- `=TIMEVALUE(MyTimeName)`
- `=NUMBERVALUE(A1;B1;C1)` when each argument is a bounded scalar source
- `=DATEVALUE("1954-"&A1)`
- `=VALUE(+A1)` or `=VALUE(-A1)` where the scalar coercion stays inside the
  bounded host-backed materializer

Required result:

- a meaningful jump in engine-supported `InterpretTail` coverage on normal
  workbook-local formulas
- real authoritative delegation on the widened text-parsing cluster

### 4. Add A Bounded Lookup/Index Delegation Cluster

Use the same seam to add the first lookup-read family.

This workstream should:

- reuse host-backed range and scalar access
- keep the lookup surface single-area and workbook-local
- avoid matrix/spill-only semantics
- start in `Observe` and `ShadowCompare`
- promote to `AuthoritativeWithFallback` once mismatch buckets stabilize

Primary code surfaces:

- [LookupExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/LookupExecution.hxx)
- [Host.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/Host.hxx)
- [FormulaEvaluator.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/FormulaEvaluator.hxx)
- [formulacell.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/data/formulacell.cxx)

Required result:

- the live `InterpretTail` seam now covers both scalar parsing and bounded
  range lookup capabilities
- the switchover becomes a real capability-cluster migration, not just a
  text-parsing special case

### 5. Unify Projection, Telemetry, And Fallback Plumbing

The current pass uses a local authoritative-projection path in
`formulacell.cxx`. This wave should harden that into reusable migration
infrastructure.

Required work:

- move authoritative projection into reusable compat helpers where practical
- add per-family or per-capability telemetry, not only coarse global counts
- distinguish fallback because of unsupported shape from fallback because of
  host-surface restrictions and from fallback because of shadow mismatch
- keep the live seam observable enough to support later production-default
  experiments

Required result:

- the next waves do not need to keep rebuilding one-off telemetry and
  projection code

### 6. Retire Or Demote Real Calc Evaluation Paths

This wave must include at least one concrete path-retirement outcome.

Acceptable forms:

- a bounded Calc-side evaluation branch becomes engine-first and
  fallback-second
- a duplicated Calc helper used only for the promoted bounded family is
  deleted
- a Calc branch is permanently reduced to fallback handling for the promoted
  cluster

Required result:

- the pass yields measurable Calc-owned authority reduction, not only broader
  comparison coverage

## Rollout Order

Recommended order:

1. keep the current literal-only text-parsing cluster in `authority`
2. land host-backed scalar materialization behind `observe`
3. promote widened text-parsing family to `shadow`
4. switch widened text-parsing family to `authority`
5. land bounded lookup cluster in `observe`
6. promote bounded lookup cluster to `shadow`
7. switch the stabilized subset of the lookup cluster to `authority`
8. then retire or demote the corresponding Calc path

## Metrics

Track at least:

- percentage of `InterpretTail` invocations classified as engine-supported
- percentage routed authoritatively through the engine
- per-function support rate for the widened text-parsing cluster
- per-function support rate for the bounded lookup cluster
- fallback reasons by function and capability class
- shadow mismatches by function and capability class
- number of Calc-evaluator branches bypassed, demoted, or deleted
- replay baseline stability

Stretch metrics:

- support-rate growth on representative interactive documents
- support-rate growth on imported real-world workbooks used in scripted Calc
  sessions

## Validation

Minimum validation for each workstream:

- targeted `sc/qa/unit/` tests for delegated, fallback, and mismatch cases
- standalone evaluator or runtime tests where the shared runtime is reused
- replay baseline
- `git diff --check`

Additional validation required for this wave:

- scripted AutoCalc sessions that exercise widened reference and lookup
  routes
- direct differential assertions for value, error, and key format metadata
- explicit telemetry assertions for support, fallback, mismatch, and
  authoritative-route counters

## Exit Criteria

This plan is complete only when all of the following are true:

- the text-parsing family no longer stops at literal-only inputs
- workbook-local single-cell references and names are supported on the live
  delegated seam for that family
- at least one bounded lookup capability family reaches authoritative routing
  with explicit fallback
- support-rate and authoritative-route metrics increase materially over the
  first-pass baseline
- at least one real Calc evaluation path is deleted, quarantined, or
  permanently demoted because the engine route is authoritative

If the pass cannot meet that retirement bar, it should close honestly as
expanded migration infrastructure rather than claim a substantive authority
shift.

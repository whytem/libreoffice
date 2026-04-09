# Computational Substrate InterpretTail -> Engine Evaluator Switchover Plan

Status: completed closeout for the first live evaluator switchover pass

## Purpose

This plan defines the first concrete live authority transfer target:

- route real Calc formula evaluation from
  [ScFormulaCell::InterpretTail](/home/ubuntu/repos/libreoffice/sc/source/core/data/formulacell.cxx)
  to the engine evaluator in
  [FormulaEvaluator.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/FormulaEvaluator.hxx)

The goal is not to prove another bounded substrate slice. The goal is to move
real production evaluation authority into `spreadsheet_engine/`, with the
substrate acting as comparison and fallback support during the migration.

## Closeout Result

This pass landed the first real `ScFormulaCell::InterpretTail` delegation
slice.

The live delegated family is intentionally narrow but real:

- literal-only `VALUE`
- literal-only `DATEVALUE`
- literal-only `TIMEVALUE`
- literal-only `NUMBERVALUE`

The live routing seam now runs in real AutoCalc sessions behind the
`SPREADSHEET_ENGINE_INTERPRET_TAIL_ENGINE_EVALUATOR` env var with the
following rollout modes:

- `observe`
- `shadow` or `shadowcompare`
- `authority`

In `authority` mode, supported formulas bypass `ScInterpreter` and project
their result directly into `ScFormulaCell`. Unsupported or out-of-contract
formulas fall back explicitly to Calc and record a fallback reason.

This is the first completed migration phase, not the end-state evaluator
transplant.

## Why This Target

This is the highest-leverage remaining authority transfer because:

- production Calc evaluation still enters through
  [ScFormulaCell::InterpretTail](/home/ubuntu/repos/libreoffice/sc/source/core/data/formulacell.cxx)
- that path still constructs and drives `ScInterpreter`
- the engine already contains a substantial standalone evaluator family
- moving this seam creates measurable production delegation instead of more
  verifier-only widening

It is a better next target than further conjunction-based substrate expansion.

## Strategy

Use fallback-first delegation, not exact-or-rollback authority.

The engine becomes authoritative for supported formulas. Calc remains the
fallback path for unsupported, high-risk, or mismatch-producing formulas.

The substrate, replay corpus, and targeted differential tests become the
migration guardrail rather than the main product.

## Initial Authoritative Family

The first delegated production family should be narrow but real:

- workbook-local formulas only
- no external references
- no macros, add-ins, DDE, or WebService dependencies
- no database/query-runtime surfaces that still rely on Calc document
  iterators
- no matrix/spill-specific behavior that still depends on Calc-only runtime
  policy
- no printer/path/environment-sensitive functions
- formulas whose function bodies already live in shared engine runtime or in
  the standalone evaluator with stable replay coverage

This first family should be large enough to matter in normal interactive
documents, not just in synthetic tests.

The landed first family is smaller than the long-range goal, but it satisfies
the first migration bar:

- it runs inside the real `InterpretTail` production seam
- it supports `Observe`, `ShadowCompare`, and `AuthoritativeWithFallback`
  behavior under AutoCalc
- it records supported, fallback, and mismatch outcomes
- it bypasses `ScInterpreter` for a real live function family

## Non-Goals

This plan does not attempt to solve all remaining authority questions at once.

Out of scope for the first switchover:

- workbook-wide default-on delegation
- sheet mutation authority
- broad storage migration
- listener/broadcaster residency transfer
- shared-group structural relocation
- off-sheet named-range structural relocation
- full `ScInterpreter` replacement in one pass

## Core Technical Gap

The standalone evaluator currently runs against a workbook model, while live
Calc evaluation runs against `ScDocument`.

The switchover therefore needs an adapter-first phase:

- either extend the evaluator to execute against
  [api::EvaluationHost](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/api/Host.hxx)
- or introduce a host-backed evaluator companion that reuses the existing
  compiled-token and runtime logic while reading values, references, names,
  and formats from live Calc through
  [DocumentEvaluationHost](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/Host.hxx)

This is the first mandatory engineering step. Without it, the migration
remains workbook-snapshot-only.

The completed first pass used an adapter-first companion instead of a full
host-backed transplant of `FormulaEvaluator.hxx`.

That landed seam is:

- [InterpretTailEngineEvaluator.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/InterpretTailEngineEvaluator.hxx)

It reuses engine parser and text-parsing execution helpers directly against
live `ScDocument` and `ScInterpreterContext` for the first delegated family.

## Rollout Modes

Replace the current blanket AutoCalc veto with evaluator-specific rollout
modes:

- `Off`
- `Observe`
- `ShadowCompare`
- `AuthoritativeWithFallback`

Rules:

- `Observe` may run the engine and record support/fallback classification only
- `ShadowCompare` runs both engine and Calc, compares outputs, and records
  mismatch reasons
- `AuthoritativeWithFallback` uses engine results for supported formulas and
  falls back to Calc otherwise

The switchover plan is not complete until at least `Observe` and
`ShadowCompare` are allowed in real AutoCalc sessions.

That bar is now met for the landed first family.

## Workstreams

### 1. Freeze The Migration Contract

Define:

- the first delegated production family
- the fallback reasons
- the mismatch taxonomy
- the allowed delta policy
- the success metrics

Required result:

- one frozen migration contract in this plan
- one measurable definition of “delegated evaluation”

### 2. Build A Host-Backed Evaluator Entry

Add the live entry point that can evaluate against `ScDocument` through
host interfaces.

Primary code surfaces:

- [FormulaEvaluator.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/FormulaEvaluator.hxx)
- `FormulaEvaluator*.cxx`
- [api::Host.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/api/Host.hxx)
- [Host.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/Host.hxx)

Required result:

- a host-backed engine evaluation API that accepts the live formula/address
  context needed by Calc
- no workbook-model materialization requirement on the hot path

### 3. Wire Observe And ShadowCompare Into InterpretTail

Add a routing seam in
[ScFormulaCell::InterpretTail](/home/ubuntu/repos/libreoffice/sc/source/core/data/formulacell.cxx)
that can:

- classify whether a formula is inside the first delegated family
- run the engine evaluator in `Observe`
- run engine plus Calc in `ShadowCompare`
- record support, fallback, and mismatch reasons

Required result:

- real interactive-session signal for the first delegated family
- no value-authority change yet

### 4. Authoritative With Fallback For The First Family

Once shadow mismatches are acceptably understood, switch the first family to
authoritative engine evaluation with fallback.

That means:

- engine result populates `ScFormulaCell` result state for supported formulas
- unsupported or mismatch-producing cases fall back to Calc evaluation
- fallback is explicit and measured, not silent

Required result:

- a production Calc family whose evaluation authority now lives in the engine

### 5. Expand By Capability Cluster

Expand delegation family-by-family, not conjunction-by-conjunction.

Recommended order:

1. pure scalar runtime-function families already converged in shared engine
2. workbook-local reference and named-range families
3. lookup families that can reuse host-backed range access
4. broader planner/dependency delegation once evaluation authority is stable

Required result:

- routing percentage increases by meaningful family waves
- duplicated Calc-only logic begins to retire

### 6. Retire Calc Paths And Rebaseline The Program

Once authoritative routing is stable for a family:

- delete or hard-quarantine duplicated Calc paths when practical
- move related substrate passes into the role of regression guardrail rather
  than primary roadmap
- update current-state docs to report production delegation, not only replay
  equivalence

Required result:

- measurable reduction of Calc-owned evaluation authority

For the completed first pass, that reduction is:

- bounded `InterpretTail` bypass for supported literal-only text-parsing
  formulas in `authority` mode

## Metrics

This plan is governed by production-facing metrics, not only replay metrics.

Track at least:

- percentage of `InterpretTail` invocations classified as engine-supported
- percentage routed through engine in authoritative mode
- fallback reason histogram
- shadow mismatch histogram
- number of Calc evaluation entry points deleted or bypassed
- replay baseline stability

## Validation

Minimum validation for each migration phase:

- targeted `sc/qa/unit/` coverage for delegated and fallback families
- standalone evaluator tests
- replay baseline
- `git diff --check`

Additional phase-specific validation:

- `Observe` and `ShadowCompare` need interactive-session or scripted Calc
  coverage, not only narrow unit tests
- `AuthoritativeWithFallback` needs differential assertions that engine and
  Calc agree on result value, error, and key metadata for the delegated family

## Exit Criteria

This plan is complete only when all of the following are true:

- real AutoCalc sessions can run at least `Observe` and `ShadowCompare`
  without the blanket substrate veto
- a measurable production family is evaluated authoritatively by the engine
  with explicit fallback
- the program can report live delegation percentage, fallback reasons, and
  mismatch reasons
- at least one meaningful Calc evaluation path is deleted, bypassed, or
  permanently demoted because the engine path is authoritative

If the project cannot satisfy those criteria, then the program should stop
describing itself as authority relocation and should formally revert to a
verifier-only goal.

## Next Expansion Target

The next evaluator-migration step should expand by capability cluster rather
than by substrate conjunction. The closest next family is:

- single-cell workbook-local reference arguments for the same text-parsing
  function cluster

That would widen the first live `InterpretTail` delegation slice without
opening external references, add-ins, matrix policy, or workbook-wide
authority.

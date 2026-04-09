# Computational Substrate InterpretTail Corpus Authoritative Usage Plan

Status: active next-pass plan for first non-zero authoritative replay-corpus
usage

## Purpose

This plan defines the next focused evaluator-migration pass after the
completed capability-cluster expansion wave.

The immediate goal is not "support one more function family."
The immediate goal is to turn the newly added Calc-backed replay-corpus
runner from pure fallback signal into the first measurable authoritative
usage on the real `ScFormulaCell::InterpretTail` seam.

In short: get some authoritative usage on the board.

## Why This Pass Next

The current live delegation seam is real and the promoted capability cluster
is already larger than the literal-only first pass.

But the new replay-corpus runner showed that the seam is still failing too
early to matter on the corpus:

- `500` workbooks
- `50,661` formula cells
- `0` authoritative routes
- `303` authoritative fallbacks
- `303` `parse_failure`
- `0` attempts classified to the already-implemented function families

That means the next blocker is no longer "there is no live seam" and not
"the current cluster is too small."
The next blocker is that the live seam is being reached, but the evaluator is
failing before it can even classify the promoted function families.

This is the smallest pass that can produce real visible migration progress
without broadening semantics first.

## Frozen Starting Point

The measured baseline comes from:

- [interpret_tail_corpus.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/interpret_tail_corpus.cxx)
- `SPREADSHEET_ENGINE_INTERPRET_TAIL_ENGINE_EVALUATOR=authority`
- `SPREADSHEET_ENGINE_INTERPRET_TAIL_CORPUS_STATS=1`

The frozen baseline is:

- `interpret_tail_corpus_workbooks=500`
- `interpret_tail_corpus_formula_cells=50661`
- `interpret_tail_authoritative_total=0`
- `interpret_tail_authoritative_fallback_total=303`
- `interpret_tail_fallback_parse_failure=303`

Per-function authoritative usage is currently `0` for:

- `VALUE`
- `DATEVALUE`
- `TIMEVALUE`
- `NUMBERVALUE`
- `MATCH`
- `XMATCH`
- `LOOKUP`
- `VLOOKUP`
- `HLOOKUP`
- `INDEX`

The full frozen stats log is:

- [/home/ubuntu/repos/libreoffice/workdir/CppunitTest/sc_interpret_tail_corpus.test.log](/home/ubuntu/repos/libreoffice/workdir/CppunitTest/sc_interpret_tail_corpus.test.log)

## Current Technical Read

The most likely current blocker is formula-source bridging on the live seam.

Today:

- [formulacell.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/data/formulacell.cxx)
  obtains the live source with `GetFormula(FormulaGrammar::GRAM_PODF, &rContext)`
- [InterpretTailEngineEvaluator.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/InterpretTailEngineEvaluator.hxx)
  normalizes only very simple prefixes before calling the engine parser
- the corpus result shows the seam is reached but dies at
  `FallbackReason::ParseFailure` before promoted-function classification

So the most likely interpretation is:

- the live formula text currently handed to the engine parser is not in a
  reliably parseable canonical form for the existing engine parser path
- as a result, the promoted function families are not even getting a chance
  to classify or route authoritatively

## Strategic Objective

Make the current promoted evaluator cluster measurable on the replay corpus
before widening function coverage again.

This pass should convert at least part of the existing `parse_failure`
surface into:

- classified promoted-function attempts
- explicit authoritative routes on the existing supported families
- a more truthful fallback histogram for the remaining unsupported shapes

## Success Criteria

This pass is complete only if all of the following are true:

- `interpret_tail_authoritative_total > 0`
- at least one currently implemented function family shows non-zero
  authoritative usage on the replay corpus
- `interpret_tail_fallback_parse_failure` is lower than the frozen `303`
  baseline
- the standing replay baseline remains exact:
  - `500` workbooks
  - `50,661` formula cells
  - `50,652` parsed formulas
  - `0` cached-fallback cells
  - `0` cached-fallback rate
- the existing live `InterpretTail` evaluator unit coverage stays green

Stretch outcome:

- more than one promoted function family shows non-zero authoritative usage
- the seam reports meaningful function-attempt counts instead of only raw
  parse failures

## Non-Goals

Out of scope for this pass:

- adding broad new evaluator families
- default-on delegation
- workbook-wide authority transfer
- substrate widening unrelated to live evaluator fallback reasons
- full parser replacement or grammar unification across Calc

## Preferred Strategy

Prefer a reusable live formula-source bridge over ad hoc per-function
special-casing.

The best outcome is not "teach the current parser one more string quirk."
The best outcome is to ensure the live `InterpretTail` seam feeds a stable,
canonical source representation into the already-landed evaluator cluster.

Preferred order:

1. canonicalize live formula source for the current seam
2. recover promoted-function classification and telemetry
3. land the first non-zero authoritative corpus routes
4. only then broaden function coverage further

## Workstreams

### 1. Freeze And Expose The Parse-Failure Surface

Add bounded diagnostics so the corpus runner can show representative raw
formula-source samples for `parse_failure` cases.

The instrumentation should stay opt-in and bounded:

- cap the number of printed samples
- record workbook and cell location
- record the raw formula text entering the evaluator seam
- record the normalized text, if different

Required result:

- one reproducible sample set showing what the parser is actually failing on

### 2. Build A Live Formula-Source Bridge

Introduce a bounded bridge that turns the live Calc formula surface into a
parser-compatible canonical source for the promoted evaluator seam.

Acceptable implementation directions:

- use a more parser-compatible grammar export at the seam if one is stable
  enough for the promoted families
- extend normalization to cover the actual emitted namespace and array forms
  seen in the frozen corpus samples
- if needed, add a bounded token-to-canonical-source bridge for the promoted
  families rather than relying on raw text export alone

Required result:

- the promoted families no longer fail wholesale at `parseFormula()` on the
  replay corpus

### 3. Recover Pre-Authority Function Classification

Make sure the seam can classify the already-promoted functions before
authority routing decisions are made.

That may mean:

- classifying from the canonicalized parse tree
- or, if needed, classifying directly from the tokenized live formula shape
  when raw source is still too lossy

Required result:

- the replay runner reports non-zero attempts for at least part of the
  already implemented function set

### 4. Add Focused Live Proof

Add or widen tests that prove the bridge on real Calc-backed examples.

Minimum proof set:

- one targeted source-normalization test for a corpus-derived raw formula
  shape
- one Calc-side authoritative test that now bypasses `ScInterpreter`
  successfully
- one retained-fallback test showing unsupported shapes still fall back
  cleanly

Required result:

- live proof covers both the positive authoritative path and the retained
  fallback boundary

### 5. Re-Run The Corpus And Freeze The New Stats

Re-run:

- `CppunitTest_sc_interpret_tail_corpus` with
  `SPREADSHEET_ENGINE_INTERPRET_TAIL_CORPUS_STATS=1`

Close the pass only if the rerun proves the pass achieved real authoritative
usage.

Required result:

- a new frozen stats snapshot with non-zero authoritative usage
- updated master/status docs that report the new baseline honestly

## Recommended Sequencing

### Phase 0. Baseline Freeze

- preserve the `0 authoritative / 303 parse_failure` starting point

### Phase 1. Diagnostic Capture

- capture representative raw formula-source failures from the corpus

### Phase 2. Formula-Source Bridge

- land the smallest reusable bridge that converts those failures into
  parser-compatible source

### Phase 3. Live Proof

- add targeted authoritative and fallback tests for the repaired seam

### Phase 4. Corpus Rerun And Closeout

- rerun the corpus
- freeze the new stats
- update roadmap docs

## Completion Bar

This plan should not close as "telemetry improved" or "parser diagnostics are
better."

It closes only when the replay corpus shows real non-zero authoritative
delegation on the existing live `InterpretTail` cluster.

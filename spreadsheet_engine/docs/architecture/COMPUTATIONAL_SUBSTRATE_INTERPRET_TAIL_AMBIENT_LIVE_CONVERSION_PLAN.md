# InterpretTail Ambient Live Conversion Plan

Status: completed slice closeout for full-corpus live-routing conversion

## Purpose

This document records the ambient live-conversion slice that followed the
initial `InterpretTail` capability and hotspot passes.

The goal of the slice was not another promoted-family probe win.
The goal was to improve the full replay corpus denominator so the live seam
starts converting natural traffic, not just curated promoted-family cells.

## Closeout Result

The slice closed successfully.

Final full replay-corpus live metrics:

- `interpret_tail_live_formula_cells=50661`
- `interpret_tail_live_supported_total=598`
- `interpret_tail_live_fallback_total=8`
- `interpret_tail_live_seen_total=606`
- `interpret_tail_live_unseen_formula_cells=50055`
- `interpret_tail_live_supported_rate=1.18`
- `interpret_tail_live_seen_rate=1.20`

Final dominant ambient fallback reasons:

- `unsupported_formula_shape=4`
- `unsupported_function=4`
- `parse_failure=0`

Promoted-family probe guardrails held:

- `interpret_tail_probe_formula_cells=1488`
- `interpret_tail_authoritative_total=1379`
- `interpret_tail_authoritative_fallback_total=109`

Important interpretation:

- the slice materially widened the full replay-corpus live denominator
- almost all newly supported ambient traffic is error-literal traffic routed
  as `FunctionKind::Unknown`, not yet promoted-family function traffic
- the retained ambient blocker set is now small and explicit:
  localized named-error roots such as `of:chyba:511` / `of:chyba:504`, plus
  four unsupported ambient function cells

## Starting Baseline

Starting full replay-corpus live metrics:

- `interpret_tail_live_formula_cells=50661`
- `interpret_tail_live_supported_total=0`
- `interpret_tail_live_fallback_total=303`
- `interpret_tail_live_seen_total=303`
- `interpret_tail_live_unseen_formula_cells=50358`
- `interpret_tail_live_supported_rate=0.00`
- `interpret_tail_live_seen_rate=0.60`

Starting dominant ambient fallback reasons:

- `unsupported_formula_shape=253`
- `parse_failure=48`
- `unsupported_function=2`

Starting promoted-family probe metrics, retained as non-regression guardrails:

- `interpret_tail_probe_formula_cells=1488`
- `interpret_tail_authoritative_total=1379`
- `interpret_tail_authoritative_fallback_total=109`
- promoted-family authoritative rate: `92.67%`

## Core Thesis

The slice thesis was ambient live conversion:

- make more naturally occurring replay-corpus formulas reach the seam
- make at least some of those formulas classify as supported on the full
  corpus denominator
- do that before another broad capability-wave expansion

This slice succeeds only if the all-formula live metrics improve
meaningfully, not merely the promoted-family probe.

## Scope

In scope:

- full-corpus live diagnostic inventory for the top ambient contributors
- live formula-source normalization that directly removes `parse_failure`
- bounded ambient wrapper or scalar-shape conversion that maps onto already
  promoted evaluator families
- bounded ambient shape conversion for the highest-volume
  `unsupported_formula_shape` contributors
- CI-readable reporting for the top ambient contributor buckets

Out of scope:

- new substrate widening work that does not remove a live evaluator blocker
- broad new evaluator capability families unrelated to the top ambient
  contributors
- broad release-default rollout
- deletion of a second Calc path before this ambient slice lands

## Workstreams

### 1. Freeze Ambient Contributor Inventory

Add or improve full-corpus diagnostics so the pass closes on real top
contributors, not assumptions.

Required output:

- top ambient root kinds or function families behind
  `unsupported_formula_shape`
- top ambient normalized-source patterns behind `parse_failure`
- sample cells and workbook locations for each retained top bucket

### 2. Remove Ambient Parse-Failure Waste

The starting `parse_failure=48` bucket was pure waste on the live seam.

Target:

- normalize or bridge the relevant live formula-source shapes so these cells
  either parse and classify or are excluded for a principled reason

### 3. Convert Ambient Shape Waste

The largest starting ambient blocker was `unsupported_formula_shape=253`.

Target only the highest-volume contributor shapes that:

- already reduce to promoted evaluator roots, or
- require only bounded scalarization, wrapper flattening, or
  source-materialization adjustments

The pass should not open a broad new capability cluster unless the ambient
inventory shows it is the dominant real blocker.

### 4. Preserve Probe Strength

The existing promoted-family probe must not regress materially.

Non-regression bar:

- no significant drop in `interpret_tail_authoritative_total`
- no significant increase in promoted-family fallback totals

### 5. Re-run and Freeze the New Baseline

Re-run the full replay corpus and freeze:

- all-formula live metrics
- top ambient fallback buckets
- promoted-family probe metrics
- top retained ambient blockers

## Completion Bars

This slice is complete only if all of these are true:

1. `interpret_tail_live_seen_total` increases materially from `303`
2. `interpret_tail_live_supported_total` becomes non-zero on the full replay
   corpus
3. `parse_failure` drops materially from `48`
4. `unsupported_formula_shape` drops materially from `253`
5. promoted-family probe metrics do not regress materially

Recommended numeric targets:

- `interpret_tail_live_seen_total >= 450`
- `interpret_tail_live_supported_total >= 25`
- `parse_failure <= 10`
- `unsupported_formula_shape <= 175`

Closeout status:

- all recommended numeric targets were met or exceeded

## Validation

Minimum validation for closeout:

- targeted `InterpretTail` unit coverage for each converted ambient shape
- `CppunitTest_sc_interpret_tail_corpus` with
  `SPREADSHEET_ENGINE_INTERPRET_TAIL_CORPUS_STATS=1`
- `CppunitTest_sc_ucalc_formula2`
- `CppunitTest_sc_ucalc_shared_cases`
- `CppunitTest_sc_ucalc_compile_diff`
- `spreadsheetengine_fods_evaluator_tests`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

## Exit Interpretation

The next slice should be chosen from the retained ambient blockers, not from
a generic wishlist of new functions.

Recommended next target:

1. normalize the retained localized named-error roots that still surface as
   `NamedReference`
2. identify and convert the four retained ambient unsupported-function cells
3. then increase ambient promoted-family penetration, because the current
   full-corpus supported traffic is still mostly non-function error-literal
   traffic

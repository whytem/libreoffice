# InterpretTail Ambient Residual Blocker Conversion Plan

Status: completed

## Purpose

This pass converts the tiny retained blocker surface left after the ambient
live-conversion slice.

It is intentionally narrow:

- clear the localized root-error literals that still fall back as
  `NamedReference`
- clear the retained ambient unsupported-function traffic
- do it without widening the evaluator into a new broad capability class

## Starting Baseline

From the current full replay-corpus ambient live observe run:

- `interpret_tail_live_formula_cells=50661`
- `interpret_tail_live_supported_total=598`
- `interpret_tail_live_fallback_total=8`
- `interpret_tail_live_seen_total=606`
- `interpret_tail_live_unseen_formula_cells=50055`
- `interpret_tail_live_supported_rate=1.18`
- `interpret_tail_live_seen_rate=1.20`

Retained fallback reasons:

- `unsupported_formula_shape=4`
- `unsupported_function=4`
- `parse_failure=0`

Diagnostic inventory:

- localized root error literals:
  - `=of:chyba:511`
  - `=of:chyba:504`
- retained ambient unsupported-function cells:
  - `=of:TRUE()`

## Scope

This pass includes:

- root-level normalization of localized or alternate ODF error spellings into
  the canonical engine-supported error literal shape
- bounded zero-argument logical-literal support for `TRUE()` and `FALSE()`
- focused Calc-side proof and corpus verification

This pass excludes:

- new lookup or text-parsing families
- broad logical-function support beyond `TRUE()` / `FALSE()`
- broader namespace or localization normalization outside root error literals

## Runtime Targets

1. Extend `normalizeFormulaSource(...)` so root-level `of:<error-word>:<digits>`
   spellings normalize onto the existing canonical error-literal path.
2. Add a bounded logical-literal family for `TRUE()` / `FALSE()` with Calc-safe
   result projection and logical format typing.
3. Preserve the current promoted-family probe metrics while clearing the ambient
   residual blockers.

## Proof Targets

- source-normalization proof for localized error roots
- direct helper proof for `TRUE()` / `FALSE()`
- live authority proof for localized error roots and logical literals
- full replay-corpus ambient rerun with updated totals

## Completion Bar

This pass is complete when all of the following are true:

- `interpret_tail_live_supported_total >= 606`
- `interpret_tail_live_fallback_total = 0`
- `unsupported_formula_shape = 0`
- `unsupported_function = 0`
- promoted-family probe metrics do not materially regress

## Outcome

This pass completed successfully.

Final full replay-corpus ambient live observe metrics:

- `interpret_tail_live_formula_cells=50661`
- `interpret_tail_live_supported_total=606`
- `interpret_tail_live_fallback_total=0`
- `interpret_tail_live_seen_total=606`
- `interpret_tail_live_unseen_formula_cells=50055`
- `interpret_tail_live_supported_rate=1.20`
- `interpret_tail_live_seen_rate=1.20`

Final ambient fallback reasons:

- `unsupported_formula_shape=0`
- `unsupported_function=0`
- `parse_failure=0`

Interpretation:

- the retained ambient blocker tail is now closed
- the newly supported ambient surface is still mostly root-error traffic plus
  bounded logical literals
- the next value is ambient promoted-family function routing, not more cleanup
  of the old residual blocker tail

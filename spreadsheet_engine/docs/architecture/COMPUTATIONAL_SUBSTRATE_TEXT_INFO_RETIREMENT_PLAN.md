# Text/Info Retirement Wave Plan

Status: active focused retirement plan

## Purpose

Shrink the remaining interpreter-resident legacy surface by retiring the
highest-leverage non-batch cluster left in
[interpr4.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr4.cxx):

- pure text/info subset
- parsing/inspection subset

This is the next practical step after the recent Batch-5 sweep. The honest
baseline on the current tree is:

- `legacy_interpreter_subroutine_count=51`
- `interp4_dispatch_legacy_lambda_count=25`
- `interpret_tail_live_authoritative_match_rate=99.4690%`
- `interpret_tail_live_unique_unseen_formula_cells=21`
- `interpret_tail_live_unique_fallback_formula_cells=0`
- `interpret_tail_live_unique_unsupported_function_formula_cells=0`

The key point is that this wave is attractive because much of the engine-side
logic already exists; what remains is mostly dispatch cleanup, scoped
admission widening, and retirement discipline.

## Scope

### Wave A: Pure Text/Info Subset

Primary retirement targets:

- `CODE`
- `TRIM`
- `CLEAN`
- `LEN`
- `CHAR`
- `UNICODE`
- `UNICHAR`
- `ASC`
- `JIS`
- `ISBLANK`
- `ISTEXT`
- `ISNONTEXT`
- `ISNUMBER`
- `ISNA`
- `ISERR`
- `ISERROR`

Relevant Calc-side legacy surface:

- direct text/info cases in
  [interpr4.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr4.cxx)
- `pushLegacyIsEmpty`
- `pushLegacyIsValue`
- `pushLegacyIsNA`
- `pushLegacyIsErrLike`

Existing engine/shared assets:

- [InterpretTailEngineEvaluator.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/InterpretTailEngineEvaluator.hxx)
  `evaluateTextUtilityFunction`
- [InterpretTailEngineEvaluator.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/InterpretTailEngineEvaluator.hxx)
  `FunctionKind::InformationPredicate`
- [TextScalar.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/runtime/TextScalar.hxx)
- [TextWidth.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/runtime/TextWidth.hxx)
- [TextServices.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/TextServices.hxx)

Wave-A goal:

- replace the remaining Calc-side direct/lambda execution for these functions
  with engine-first dispatch
- retire the corresponding `pushLegacy*` helpers once exercised runtime and
  parity gates are met

### Wave B: Parsing/Inspection Subset

Primary retirement targets:

- `VALUE`
- `NUMBERVALUE`
- `DATEVALUE`
- `TIMEVALUE`
- `FORMULA`
- `ISFORMULA`

Relevant Calc-side legacy surface:

- `pushLegacyValue`
- `pushLegacyNumberValue`
- `pushLegacyDateOrTimeValue`
- `pushLegacyFormulaText`
- `pushLegacyIsFormula`

Existing engine/shared assets:

- [InterpretTailEngineEvaluator.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/InterpretTailEngineEvaluator.hxx)
  text parsing / formula inspection paths
- [TextParsingExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/TextParsingExecution.hxx)
- [FormulaInspectionExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/FormulaInspectionExecution.hxx)

Wave-B goal:

- widen engine-first handling to the remaining root and scalarized-reference
  shapes these functions still decline on
- retire the lambda fallbacks once exercised runtime and parity gates are met

## Explicit Deferrals

These should stay out of this wave because they still depend on host-sensitive
formatting, collation, byte-width, or service behavior:

- `T`
- `ISLOGICAL`
- `ISREF`
- `TEXT`
- `LEFT` / `RIGHT` / `MID`
- `TEXTBEFORE` / `TEXTAFTER`
- `SEARCH` / `FIND`
- `REGEX`
- `SUBSTITUTE`
- `REPT`
- `CONCATENATE` / `CONCAT` / `TEXTJOIN`
- `DOLLAR` / `FIXED` / `REPLACE`
- `BAHTTEXT`
- `LENB` / `LEFTB` / `RIGHTB` / `MIDB` / `REPLACEB` / `FINDB` / `SEARCHB`
- `ENCODEURL`

Those can follow as a separate host-sensitive text wave after the pure and
parsing/inspection subsets are retired.

## Required Gates

Every retirement in this wave should satisfy all of the following:

1. Engine/shared implementation already exists for the admitted shape.
2. A focused Calc lane proves the engine-first path is exercised for the
   function, not just statically wired.
3. A deliberate-decline test exists for at least one shape that remains out of
   scope.
4. The standing corpus keeps:
   - `fallback = 0`
   - `unsupported_function = 0`
   - `known_regressions_baseline = 0`
5. The retirement deletes the corresponding `pushLegacy*` helper or direct
   Calc-host path rather than merely relocating it.

## Authoritative-Match Gap Closure

This wave must also own the part of the remaining authoritative-match gap that
belongs to text/info and parsing/inspection semantics.

Current standing-corpus gap signals:

- `interpret_tail_live_unique_unseen_formula_cells=21`
- top unseen unknown-surface root remains `parse_failure` with `12` unseen cells
- second visible unseen root is `operator:+` with `6` unseen cells
- the expanded unknown-surface census now reports only `3` additional unseen
  cells outside the top printed root list

Required step before any fallback deletion:

1. Mine the current `interpret_tail_live_unique_top_unknown_surface_root_*`
   samples and classify which of the remaining unseen cells belong to:
   - text/info syntax or semantics
   - parsing/inspection syntax or semantics
   - out-of-scope non-text families
2. Close every wave-owned gap before retirement.
3. For out-of-scope residuals, document the ownership boundary explicitly so
   the wave does not claim a misleading parity win.

This keeps the wave honest: it is not enough to reduce lambda count if the
same functions still own a meaningful share of the remaining live
authoritative-match gap.

### Phase 0 Inventory Result

Phase 0 is now complete on the current standing baseline.

Classification result:

- Wave A owned live-unique unseen cells: `0`
  - `interpret_tail_live_unique_function_text_utility_unseen_formula_cells=0`
  - `interpret_tail_live_unique_function_information_predicate_unseen_formula_cells=0`
- Wave B owned live-unique unseen cells: `0`
  - `interpret_tail_live_unique_function_value_unseen_formula_cells=0`
  - `interpret_tail_live_unique_function_datevalue_unseen_formula_cells=0`
  - `interpret_tail_live_unique_function_timevalue_unseen_formula_cells=0`
  - `interpret_tail_live_unique_function_numbervalue_unseen_formula_cells=0`
  - `interpret_tail_live_unique_function_formula_text_unseen_formula_cells=0`

The remaining `21` live-unique unseen cells are therefore out of scope for
this retirement wave on the standing corpus:

- `12` are `parse_failure` rows from malformed logical formulas in
  `logical/not.fods`, for example `=NOT(0)NOT(0)` and its siblings
- `6` are `operator:+` rows currently rooted in non-wave shapes such as
  `ORG.OPENOFFICE.CURRENT()`, `ORG.OPENOFFICE.STYLE(...)`, and range/matrix
  arithmetic like `=[.I5:.I6]+1`
- `3` remain in the long unknown-surface tail beyond the top printed roots;
  by current function-family accounting they are also outside Wave A / Wave B

Operational consequence:

- this wave does not need additional authoritative-match widening before
  retirement to close a standing corpus parity gap
- the next blocking work for Wave A / Wave B is exercised engine-first runtime
  on the relevant text/info and parsing/inspection dispatch paths, followed by
  fallback deletion

## Execution Order

### Phase 0: Gap Inventory

- refresh the standing corpus samples for the remaining `21` unseen cells
- classify which misses belong to Wave A / Wave B
- add or tighten focused tests for those shapes

Status:

- complete on the current standing baseline; no live-unique unseen cells are
  owned by Wave A / Wave B

### Phase 1: Pure Text/Info Admission Tightening

- route the Wave-A direct cases through the engine path first
- keep legacy fallback only where the engine explicitly declines
- add runtime/accounting checks for exercised engine-first paths

### Phase 2: Pure Text/Info Retirement

- delete `pushLegacyIsEmpty`
- delete `pushLegacyIsValue`
- delete `pushLegacyIsNA`
- delete `pushLegacyIsErrLike`
- remove any remaining Calc-only direct execution for the admitted pure text
  utility cases

### Phase 3: Parsing/Inspection Admission Tightening

- widen `VALUE`, `NUMBERVALUE`, `DATEVALUE`, `TIMEVALUE`, `FORMULA`,
  `ISFORMULA` to the remaining safe root / scalarized-reference shapes
- explicitly defer matrix and other host-heavy shapes that remain out of scope

### Phase 4: Parsing/Inspection Retirement

- delete `pushLegacyValue`
- delete `pushLegacyNumberValue`
- delete `pushLegacyDateOrTimeValue`
- delete `pushLegacyFormulaText`
- delete `pushLegacyIsFormula`

## Expected Outcome

If this wave lands cleanly:

- `interp4_dispatch_legacy_lambda_count` should drop materially from `25`
- the remaining interpreter-side text/info surface becomes mostly the
  host-sensitive tail, which is a much cleaner next target
- the remaining authoritative-match gap becomes smaller and better-classified
  instead of being mixed together with already-ownable text/info misses

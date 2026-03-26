## Basic FODS Support

This document outlines the smallest practical implementation that would let
`spreadsheet_engine/` replay the same raw Calc function workbooks that live
under `sc/qa/unit/data/functions/**/fods/`, without pulling in LibreOffice's
full document shell, UNO importer stack, rendering model, or broader workbook
feature surface.

### Goal

Add enough standalone infrastructure to:

- load raw `.fods` (Flat OpenDocument Spreadsheet) function workbooks
- build a minimal in-memory workbook model
- evaluate the subset of worksheet formulas needed by the basic function
  families
- reuse the same pass/fail workbook convention that Calc's FODS harness uses

### Explicit Non-Goals

The first implementation stages must not try to support:

- rendering, layout, styles, or page model fidelity
- charts, drawing objects, annotations, or UI metadata
- pivot tables, autofilters, database ranges, or conditional formatting
- macros, UNO services, or document shells
- save/export
- full ODS package support beyond read-only FODS parsing
- richer workbook behaviors that are not needed for basic worksheet-function
  replay

### Why A Minimal Loader Still Needs Some Structure

The Calc function corpus is not just a bag of formulas. Many of the raw FODS
files also contain:

- named ranges
- copied-in reference sheets via `table:table-source`
- cached formula values
- workbook scaffolding formulas like `AND`, `ROUND`, `FORMULA`, `ISERROR`,
  `NA`, `TRUE`, and `FALSE`

That means a useful standalone replay path needs a very small workbook runtime,
not just an XML-to-TSV converter.

### Minimal Architecture

#### 1. Internal workbook model

Add internal-only workbook structures under `inc/spreadsheetengine/detail/`:

- `Workbook`
- `Sheet`
- `Cell`
- `NamedRange`
- `SheetSource`

These should store:

- sheet names
- sparse cell storage
- raw formula text
- cached scalar values from FODS
- named range definitions
- read-only imported-sheet metadata from `table:table-source`

This model is intentionally internal for now.

#### 2. Minimal FODS loader

Add a standalone-only FODS loader backed by `libxml2` that understands only:

- `office:body/office:spreadsheet`
- `table:table`
- `table:table-row`
- `table:table-cell`
- `table:covered-table-cell`
- repeated row and column attributes
- `table:formula`
- `office:value-type` and the corresponding cached value attributes
- `text:p`
- `table:named-expressions/table:named-range`
- `table:table-source`

The loader should skip unsupported document features while tracking counts for
ignored categories like charts, draw objects, and content validations.

#### 3. Minimal evaluator

Once the loader exists, add a narrow evaluator that supports:

- scalar values
- range references
- named ranges
- ODF formulas with `of:=`
- basic arithmetic, comparison, concatenation, and parentheses
- function calls with `;`
- lazy recalculation with memoization and cycle detection

#### 4. Raw workbook replay harness

After the evaluator reaches the needed function subset, add a standalone replay
harness that mirrors Calc's existing workbook convention:

- scan a FODS directory
- load each workbook
- recalc formulas
- assert `Sheet1.B3 == 1`
- on failure, inspect the `Expected`, `Correct`, and `FunctionString` columns

### Phased Implementation

#### Pass 1: workbook model plus loader

Deliverables:

- internal workbook model
- minimal FODS loader
- ignored-feature summary
- standalone tests with small local FODS fixtures

Out of scope for pass 1:

- formula parsing
- workbook recalculation
- external sheet import resolution

#### Pass 2: formula AST and parser

Deliverables:

- minimal ODF formula parser
- AST for scalar literals, references, ranges, calls, and operators
- standalone parser tests using raw formula strings from the FODS corpus

#### Pass 3: evaluator core

Deliverables:

- scalar and range evaluation
- dependency graph / memoized evaluation
- cycle detection
- named range lookup
- cached-value fallback behavior where appropriate

#### Pass 4: external copied result sheets

Deliverables:

- read-only support for `table:table-source` with `mode="copy-results-only"`
- relative link resolution
- minimal imported-sheet loading

#### Pass 5: raw FODS replay for basic families

Initial target families:

- logical
- mathematical
- text
- date_time

Secondary target families:

- selected spreadsheet functions, especially `MATCH`, `INDEX`, and related
  lookup/reference workbooks

### Detailed Implementation Checklist

#### Foundation

- [x] Add a new architecture note here and keep it updated as the FODS work
      progresses.
- [x] Add an internal workbook model header under
      `inc/spreadsheetengine/detail/`.
- [x] Add a standalone-only FODS loader header under
      `inc/spreadsheetengine/detail/`.
- [x] Add a standalone-only loader source under `source/core/`.
- [x] Wire `libxml2` into the standalone CMake build.
- [x] Add a dedicated standalone test target for FODS support.
- [x] Add local FODS fixture files under `tests/data/fods/`.

#### Workbook model

- [x] Store sparse cell data by sheet / row / column.
- [x] Preserve raw formula text for formula cells.
- [x] Preserve cached scalar values for formula cells.
- [x] Preserve raw FODS value-type information where it is needed for later
      evaluator work, especially date/time/error cells.
- [x] Represent named ranges, including sheet-local scope.
- [x] Represent imported-sheet metadata from `table:table-source`.
- [x] Add convenience lookup helpers for sheets, cells, and named ranges.

#### Loader

- [x] Parse the spreadsheet root and all table nodes.
- [x] Parse repeated rows.
- [x] Parse repeated cells.
- [x] Parse covered cells well enough to preserve coordinate progress.
- [x] Parse cached scalar values for number, string, boolean, and error cells.
- [x] Preserve raw value type and raw lexical value for date/time cells.
- [x] Parse root-level named ranges.
- [x] Parse sheet-local named ranges.
- [x] Parse `table:table-source` metadata without resolving it yet.
- [x] Count ignored charts, draw objects, validations, and annotations.
- [x] Fail cleanly on malformed XML and missing spreadsheet bodies.

#### Parser

- [x] Add a minimal ODF formula tokenizer.
- [x] Add a minimal AST.
- [x] Support string, number, boolean, and error literals.
- [x] Support cell and range references.
- [x] Support named range references.
- [x] Support unary `+` and `-`.
- [x] Support binary `+ - * / ^ &`.
- [x] Support comparisons `= <> < <= > >=`.
- [x] Support function calls with `;`.

#### Evaluator

- [x] Add per-cell lazy evaluation with memoization.
- [x] Add cycle detection and cycle reporting.
- [x] Support scalar arithmetic and comparison semantics.
- [x] Support range materialization needed by the early function families.
- [x] Support named range resolution.
- [x] Support `FORMULA()` using preserved raw formula text.
- [x] Reuse existing extracted engine helpers wherever possible.

#### Imported copied sheets

- [x] Resolve `table:table-source` links relative to the containing workbook.
- [x] Add a suffix-trimming fallback for chained relative hrefs when the direct
      relative path does not exist.
- [x] Load referenced `.fods` workbooks for `mode="copy-results-only"` when the
      source file is available.
- [x] Copy referenced source-sheet cells into the imported sheet as a read-only
      snapshot.
- [x] Preserve imported-sheet metadata and leave sheets empty when the source
      file cannot be resolved.

#### Function-family enablement

- [x] Implement the workbook scaffolding functions that the harness itself uses
      for the initial logical-family replay path.
- [x] Enable the logical FODS family.
- [x] Enable the mathematical FODS family.
      The default standalone replay lane now scans the full raw mathematical
      FODS directory. That includes the earlier scalar/arithmetic workbooks and
      `aggregate.fods`, with live evaluator support for prefixed one-range
      `AGGREGATE` forms, hidden/error option filtering, and nested
      `SUBTOTAL`/`AGGREGATE` skipping for options `0-3`.
- [x] Enable the text FODS family.
      The default standalone replay lane now also scans the full raw text FODS
      directory. The text-family enablement work added live standalone support
      for `CLEAN()`, `UNICHAR()`, scalar and first-element range `EXACT()`, and
      `1x1` array-constant parsing/evaluation, plus loader support for ODF text
      markup like `text:s` and `text:tab` so raw expected cells replay
      correctly. It also keeps the narrow `Â`+`C2 xx` cached-string mojibake
      repair used by the Calc FODS corpus for `UNICHAR(128..191)`.
- [x] Enable the date_time FODS family.
      The default standalone replay lane now also scans the full raw date-time
      FODS directory. The date-time enablement work added live standalone
      support for `DATE()`, `TIME()`, `VALUE()`, `DATEVALUE()`, `TIMEVALUE()`,
      `ORG.OPENOFFICE.DAYSINMONTH()`, `ORG.OPENOFFICE.DAYSINYEAR()`,
      `ORG.OPENOFFICE.ISLEAPYEAR()`, `ISOWEEKNUM()`, `EDATE()`, `EOMONTH()`,
      and `ORG.OPENOFFICE.WEEKS()`, plus the narrow
      `ORG.LIBREOFFICE.RAWSUBTRACT()` helper used by some workbook self-checks.
      The evaluator now also materializes stored typed date cells as numeric
      serials and understands ODF `office:time-value="PT..."` durations for
      typed stored time cells.
- [ ] Expand to selected spreadsheet lookup/reference workbooks.

#### Replay harness

- [x] Add a standalone raw-FODS replay binary.
- [x] Mirror Calc's `Sheet1.B3 == 1` success convention.
- [x] Add failure diagnostics that report the first incorrect function row.
- [x] Add directory filtering so early runs can target one family at a time.
- [x] Add a maintenance lane for the standalone FODS replay subset.

### Recommended First Slice

Start with pass 1 only:

- internal workbook model
- minimal FODS loader
- local fixture-driven tests

That gives the project a concrete workbook substrate immediately, without
committing yet to the larger parser/evaluator work.

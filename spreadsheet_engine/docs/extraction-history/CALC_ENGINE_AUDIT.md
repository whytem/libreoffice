# Calc Calculation Engine Audit

This is a curated guide to the key source files that make up LibreOffice Calc's
spreadsheet calculation engine.

## Scope

- Included: workbook / sheet / cell storage, formula parsing and compilation,
  token model, formula execution, matrix / array evaluation, dependency
  tracking, reference updates, named ranges, lookup / query helpers, and
  grouped formula execution.
- Excluded: UI (`sc/source/ui`), file filters (`sc/source/filter`), rendering,
  drawing, UNO wrappers, and tests.
- Note: several major classes are split across numbered implementation files.
  Those are listed as file families below.

## Engine Flow

At a high level the engine is structured as:

1. `ScDocument` / `ScTable` / `ScColumn` store workbook, sheet, and cell data.
2. `ScCompiler` and `formula::FormulaCompiler` turn formula text into token
   arrays.
3. `ScFormulaCell` owns compiled code, result state, and dependency links.
4. `ScInterpreter` executes token arrays using `ScInterpreterContext`,
   `ScMatrix`, lookup / query helpers, and caches.
5. Broadcaster / listener and reference-update code keeps the dependency graph
   and recalculation state correct after edits.

## Key Source Files

| Source file(s) | Main types / subsystem | Key function |
| --- | --- | --- |
| `sc/source/core/data/document.cxx`, `documen2.cxx`-`documen9.cxx`, `document10.cxx` | `ScDocument` | Workbook-level core model. Owns sheets, calculation state, caches, named expressions, parser pool, and cross-sheet / cross-column recalculation orchestration. |
| `sc/source/core/data/table1.cxx`-`table7.cxx` | `ScTable` | Sheet-level engine. Implements row / column operations, cell access, sheet-local recalculation behavior, and sheet-scoped formula maintenance. |
| `sc/source/core/data/column.cxx`-`column4.cxx` | `ScColumn` | Column-level storage and execution support. Handles cell blocks, insert/delete/copy/move operations, formula grouping, and fast iteration over column data. |
| `sc/source/core/data/cellvalue.cxx`, `cellvalues.cxx`, `mtvelements.cxx`, `mtvcellfunc.cxx` | Cell payload and block storage | Typed cell values and the multi-type-vector-backed storage primitives used by columns and sheets. |
| `sc/source/core/data/formulacell.cxx` | `ScFormulaCell`, `ScFormulaCellGroup` | Central formula cell implementation. Owns compiled code, cached result, dirty state, shared-formula membership, dependency listeners, and recalc triggering. |
| `sc/source/core/data/formulaiter.cxx`, `sc/source/core/data/dociter.cxx` | Formula / document iterators | Iteration helpers used to walk formulas, references, and document cell ranges efficiently during recalculation and analysis. |
| `sc/source/core/tool/compiler.cxx` | `ScCompiler` | Calc-specific formula compiler. Parses user-visible Calc / ODF / Excel formula text into Calc token arrays with references, names, separators, and syntax conventions resolved. |
| `formula/source/core/api/FormulaCompiler.cxx` | `formula::FormulaCompiler` | Shared formula compiler infrastructure used by Calc and formula services: opcode maps, name lookup support, and grammar-aware token handling. |
| `formula/source/core/api/grammar.cxx` | `formula::FormulaGrammar` | Maps formula language and addressing conventions between native Calc, ODF, API, and Excel-style grammars. |
| `formula/source/core/api/token.cxx` | `formula::FormulaToken` | Base token model for compiled formulas: opcodes, parameter counts, reference-vs-function classification, and generic token behavior. |
| `sc/source/core/tool/token.cxx` | Calc token extensions | Calc-specific token implementations for sheet references, matrices, named ranges, table references, and UNO/API token conversion. |
| `formula/source/core/api/vectortoken.cxx` | Vector tokens | Token types used by grouped / vectorized formula execution paths to expose columnar numeric and string arrays. |
| `sc/source/core/tool/interpr1.cxx`-`interpr8.cxx` | `ScInterpreter` | The formula execution engine proper. Implements the built-in spreadsheet operators and functions, stack evaluation, branching, lookups, text/math/date logic, and array semantics. |
| `sc/source/core/tool/interpretercontext.cxx` | `ScInterpreterContext`, `ScInterpreterContextPool` | Per-execution and per-thread runtime context: token cache, formatter state, lookup cache hookup, RNG state, and other execution-local caches. |
| `sc/source/core/tool/formularesult.cxx` | `ScFormulaResult` | Result container used by formula cells and interpreter code for scalar, string, error, token, and matrix results. |
| `sc/source/core/tool/scmatrix.cxx`, `jumpmatrix.cxx`, `matrixoperators.cxx` | `ScMatrix`, `ScJumpMatrix`, matrix ops | Array / matrix evaluation substrate. Stores matrix values, supports array formula execution, and handles branch-aware jump matrices used by conditional evaluation. |
| `sc/source/core/tool/formulagroup.cxx` | Formula groups | Vectorized / grouped formula execution support, including cached column arrays and selection of optimized calculation backends. |
| `sc/source/core/opencl/formulagroupcl.cxx` | OpenCL formula groups | Optional OpenCL-backed grouped execution path for supported formula groups. |
| `sc/source/core/tool/sharedformula.cxx` | Shared formula maintenance | Maintains and splits shared formula groups when edits invalidate the previous grouping layout. |
| `sc/source/core/tool/calcconfig.cxx` | `ScCalcConfig` | Calculation engine configuration: threading, OpenCL selection, string-ref syntax, and other runtime execution policies. |
| `sc/source/core/tool/refdata.cxx` | `ScSingleRefData`, `ScComplexRefData` | Core representation of absolute / relative cell and range references, including conversion between stored and absolute addresses. |
| `sc/source/core/tool/refupdat.cxx` | Reference update algorithms | Updates references after insert/delete/move/copy/reorder operations so compiled formulas keep pointing at the right cells. |
| `sc/source/core/tool/rangenam.cxx` | `ScRangeData`, `ScRangeName` | Named expressions and named ranges: compile, store, update, and reserialize formulas that are bound to workbook or sheet scope. |
| `sc/source/core/data/bcaslot.cxx`, `broadcast.cxx`, `listenercontext.cxx` | Broadcaster / listener graph | Dependency graph plumbing. Registers formula listeners on cells and ranges, propagates invalidation, and cleans up broadcaster state after edits. |
| `sc/source/core/tool/recursionhelper.cxx` | `ScRecursionHelper` | Cycle detection and iterative recalculation support for recursive / circular formulas and formula-group dependency checks. |
| `sc/source/core/tool/lookupcache.cxx` | `ScLookupCache` | Cache for repeated lookup criteria in functions like `VLOOKUP`, `MATCH`, and `XLOOKUP`, reducing repeated scans of the same lookup range. |
| `sc/source/core/data/queryevaluator.cxx`, `queryiter.cxx` | Query / lookup evaluation | Shared query engine used by lookup-style formulas, database formulas, and range scans that need typed comparison, wildcard, regex, or sorted-search behavior. |
| `sc/source/core/data/simpleformulacalc.cxx` | `ScSimpleFormulaCalculator` | Lightweight compile-and-evaluate helper for standalone formula evaluation outside a persisted cell object. |
| `sc/source/core/tool/formulaparserpool.cxx` | `ScFormulaParserPool` | Registry and factory for external filter formula parsers used when Calc must interpret formulas from foreign namespaces / formats. |

## Best Entry Points For Reading

If you want to understand the engine quickly, start in roughly this order:

1. `sc/inc/document.hxx` and the `document*.cxx` family
2. `sc/inc/table.hxx` and `table1.cxx`-`table7.cxx`
3. `sc/inc/column.hxx` and `column.cxx`-`column4.cxx`
4. `sc/inc/formulacell.hxx` and `formulacell.cxx`
5. `sc/inc/compiler.hxx` and `sc/source/core/tool/compiler.cxx`
6. `sc/source/core/inc/interpre.hxx` and `interpr1.cxx`-`interpr8.cxx`
7. `sc/source/core/tool/scmatrix.cxx`
8. `sc/source/core/data/bcaslot.cxx` and `sc/source/core/tool/refupdat.cxx`

## Adjacent But Not Core

These areas are important nearby code, but they are not the core calculation
engine itself:

- `sc/source/filter/*`: import / export of spreadsheet files
- `sc/source/ui/*`: Calc UI, document shell commands, dialogs, view logic
- `sc/source/core/data/dp*`: Pivot table engine
- `sc/source/core/data/drwlayer.cxx`, `drawpage.cxx`: drawing-layer integration
- `sc/qa/*`: tests

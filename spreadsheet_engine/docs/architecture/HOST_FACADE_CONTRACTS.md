# Host Facade Contracts

Phase 2 deliverable for the
[Computational Substrate Authority Transfer Pivot Plan](COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_PIVOT_PLAN.md).
This document is the explicit contract inventory for the remaining
Calc-resident evaluator surface: it records which host-facing services are
already real, which execution shells are now only adapter residue, and which
surfaces are intentionally staying on the host side rather than being
absorbed into the standalone engine.

## How to read this doc

- **Current contracts** — primitives already in production on the
  engine side and the `libreoffice` compat adapter. Each entry lists
  signature, semantics, preconditions, error modes, thread safety,
  and representative callers.
- **Transition notes** — formerly pending surfaces and still-unbound
  adapter steps retained for current-state context.
- **Principles** — policy for when to add a new primitive vs.
  extend one, how contracts are versioned, and what level of tests
  an admission needs before it is marked stable.

The primary source of truth is always the code. If a contract below
disagrees with the header, the header wins and this document must be
updated in the same commit.

---

## Phase 2 Inventory Snapshot

This matrix is paired with the exhaustive symbol-level inventory in
[COMPUTATIONAL_SUBSTRATE_RPN_HOST_BOUNDARY_AUDIT.md](COMPUTATIONAL_SUBSTRATE_RPN_HOST_BOUNDARY_AUDIT.md).
The corpus test harness now checks that every remaining `Sc*` method and
`pushLegacy*` lambda is named in one of these two documents, so Phase 2 cannot
quietly drift out of date.

### Status legend

- `already exposed`: the contract is already available through `api/Host.hxx`
  or a production compat helper and is considered stable enough for further
  migration work.
- `exposed but too broad`: the engine can reach the service today, but the
  surface is fragmented, stopgap, or too tied to Calc internals to treat as
  the final contract. This label is retained for transition-era context.
- `missing`: a transition-era label for behavior that had not yet been
  formalized as a stable host contract. No live row currently uses it.
- `intentionally unsupported`: the capability should remain an explicit
  host-owned terminal and is not part of the engine-native evaluator contract.

### Contract status matrix

| Service category | Primary current contract(s) | Status | Representative remaining legacy surface | Ownership note |
| --- | --- | --- | --- | --- |
| Scalar cell read / visible value materialization | `CellReader`, `readMaterializedHostCellValue`, `CellValueView` | `already exposed` | text/info predicates, VALUE/DATEVALUE/TIMEVALUE, DB aggregate admissions, matrix consumers | Host owns cell read mechanics; engine owns coercion, aggregation, and error propagation |
| Basic reference resolution | `RangeResolver::resolveRange`, `ReferenceResolver::resolveReference`, external-ref fetch helpers | `already exposed` | host-only union/intersection/range terminals, external add-in terminals | Compile-time and evaluation-time reference resolution now share one contract; the remaining Calc sites are explicit host terminals rather than duplicate evaluator logic |
| Named / external / database / structured range resolution | compile-host lookup resolvers, `RangeResolver::resolveRange`, `DocumentRangeResolver` | `already exposed` | query-family range walking, structured-reference materialization follow-ups | One evaluation-time resolver now covers named, DB, external, and INDIRECT-driven range binding; structured references that cannot flatten still round-trip as `TokenBackedSymbol` |
| Matrix materialization | `materializeHostRangeToMatrixOperand`, `CellValueView::matrixReference`, matrix operand bridges | `already exposed` | matrix-form admissions in surviving `ScMat*` / regression-family consumers | Phase 6 retired `ExecuteMatValueTerminal`, `ExecuteMatRefTerminal`, `ExecuteFrequencyTerminal`, `ExecuteForecastEtsTerminal`, `ExecuteFourierTerminal`, and `ExecuteSumXMY2Terminal`; the remaining Calc matrix surface is direct host-backed matrix consumption rather than dedicated wrapper terminals |
| Criteria / range iteration | `RangeIterator`, `CriteriaAggregateMaterializer::iterate` | `already exposed` | DB-family tails, COUNTBLANK widening, future streaming criteria work | A standalone row-major walker now lives on `EvaluationHost`; production query/countblank scans route through it instead of baking iteration into Calc-local walkers |
| Formula text / inspection | `runtime::formulainspection::Provider`, `DirectFormulaInspectionAdapter` | `already exposed` | no Calc-owned inspection wrappers remain | Formula-presence and formula-text reads now flow through an explicit inspection provider instead of ad hoc document peeks |
| Format / type inspection | `runtime::cellinspection::*`, `DirectCellInspectionAdapter`, `DirectHostCellInspectionAdapter` | `already exposed` | no Calc-owned inspection wrappers remain | The bounded/value vs. host-property split is now explicit, and Phase 5 retired the remaining `TYPE`, `CELL`, `INFO`, `CURRENT`, `STYLE`, and `N` Calc entrypoints |
| Locale / calendar / date / search policy | `RuntimeEnvironment::getNullDate()`, `RuntimeEnvironment::getLocaleTag()`, `RuntimeEnvironment::getSearchType()`, `RuntimeEnvironment::sampleUniformReal()`, `TextCoercion` | `already exposed` | host-sensitive text/search terminals, date/time parsing tails | Locale, null-date, search-mode, and random-source policy now flow through explicit host contracts rather than Calc-local evaluator code |
| Spill allocation | `SpillRangeAllocator` | `already exposed` | dynamic-array reshaping / spill-shaping tails | Contract is fixed; binding into `EvaluationHost` remains an adapter step, not a contract-definition gap |
| Control-flow / interpreter state | engine substrates (`RpnValue`, `RpnControlFlow`), no host method by design | `intentionally unsupported` | `ScLet`, `ScIfJump*`, `ScChooseJump`, jump-matrix state | This state should live inside the engine evaluator, not inside the Host facade |
| External computation terminals | no engine contract by design | `intentionally unsupported` | `ScMacro`, `ScDde`, `ScWebservice`, `ScFilterXML`, `ScGetPivotData`, `ScHyperLink` | These remain explicit host terminals unless the project makes a separate product decision |

## Remaining legacy surface mapped to host services

### Surviving `pushLegacy*` clusters

Phase 6 retired the remaining `pushLegacy*` lambda surface from
`interpr4.cxx`. Host-sensitive text / formatting / search work still
exists, but it now hangs off explicitly named dispatch terminals plus
the `FormulaInspection`, `CellInspection`, and `RuntimeEnvironment`
contracts above rather than an ad hoc `pushLegacy*` inventory.

Phase 2 of the relocation backlog also retired the Calc-owned
operator/control `Execute*` kernel surface. `HS11` remains relevant only as
structural shell debt in `ScInterpreter::Interpret()` and the classic stack
helpers, not as a surviving host-contract row.

Phase 3 retired the remaining Calc-owned reference/lookup/addressing
`Execute*` wrappers plus `ScMatchOp`. The one exception is
`ExecuteExternalTerminal`, which is now classified with the intentionally
host-owned external-computation terminals rather than treated as relocation
debt.

Phase 7 retired the remaining random wrapper surface. No relocation-relevant
engine-backed `Execute*` clusters remain on the current tree; the surviving
rows below are intentionally retained host-owned utilities.

### Surviving `Sc*` clusters

| Legacy cluster | Representative surviving surface | Required host-service categories | Contract status summary |
| --- | --- | --- | --- |
| Host/debug utilities | `ScTableOp`, `ScTTT`, `ScDebugVar` | document mutation / repeated-operation state, debug-only projection | Phase 1 of the relocation backlog classified these as explicit host/debug utilities rather than active relocation debt |
| External computation terminals | `ExecuteExternalTerminal`, `ScMacro`, `ScDde`, `ScWebservice`, `ScFilterXML`, `ScGetPivotData`, `ScHyperLink` | external computation | Intentionally host-owned and outside the engine-native evaluator contract |

### Formerly missing or fragmented contracts now closed

| Contract | Current status | Why it mattered |
| --- | --- | --- |
| `RangeIterator` | `already exposed` | Phase 7 closed the final open query/database host-surface gap; production criteria/countblank iteration now routes through the host contract rather than ad hoc Calc loops |

This matrix is the Phase 2 completion artifact the pivot plan refers to. New
Host-facing work should extend one of the rows above rather than inventing an
ad hoc compat helper without classifying it here first.

---

## Current contracts (stable)

### Address resolution

#### `resolveIndirectReference` — INDIRECT-family resolver

Declared in
[`IndirectExecution.hxx`](../../inc/spreadsheetengine/compat/libreoffice/IndirectExecution.hxx)
under `spreadsheetengine::compat::libreoffice::indirectexecution`.

```cpp
std::optional<IndirectExecutionResult> resolveIndirectReference(
    ScDocument& rDocument,
    const ScAddress& rPosition,
    const svl::SharedString& rReferenceText,
    formula::FormulaGrammar::AddressConvention eConvention,
    bool bTryXlA1);
```

`IndirectExecutionResult::meKind` discriminates five shapes:

1. `SingleRef` — local single-cell reference.
2. `DoubleRef` — local range reference.
3. `ExternalSingleRef` / `ExternalDoubleRef` — external reference
   carrying `(mnFileId, maTabName)`.
4. `Token` — a generic `FormulaConstTokenRef` used for
   structured-table references and external names that the compiler
   cannot flatten to a plain `ScRefAddress`. The caller is
   responsible for running the token through the normal dispatch
   loop.

**Semantics.** Given a textual reference (`"Sheet1.A1"`,
`"MyRange"`, `"Table[[#Data],[Col]]"`, `"'ext.xlsx'#Name"`, etc.),
run the same ordered search the legacy `ScInterpreter::ScIndirect`
used:

1. `ScRangeName` (local sheet names, then global names) via
   `ScRangeStringConverter::GetRangeDataFromString`.
2. `ScDBCollection::getNamedDBs` database-range lookup — header /
   totals rows stripped automatically so the indirect target points
   at data.
3. `ConvertSingleRef` / `ConvertDoubleRef` with the requested
   convention; retry under `CONV_XL_A1` if `bTryXlA1` is true.
4. Compile-and-lower fallback for structured-table references and
   quoted external-name references.

**Preconditions.** `rDocument` must be mutable (the range-data
validation path calls `ValidateTabRefs`). `rReferenceText` is the
literal text returned by argument evaluation — not pre-trimmed or
case-folded. `bTryXlA1` reflects
`ScCalcConfig::mbStringRefAddressSyntax`.

**Error modes.** All failure paths return `std::nullopt`: empty
reference text, compile error in the fallback branch, or ambiguous /
malformed text. The dispatch bridge translates `std::nullopt` to
`#REF!` (`FormulaError::NoRef`). The helper itself never produces a
Calc error code — it says "I could not resolve this to a reference
shape."

**Thread safety / performance.** Not thread-safe; the helper mutates
`ValidateTabRefs` state and consults `ScCompiler`. Callers already
hold the interpreter lock. Cost is O(named-range count + DB-range
count) per call, plus one compile in the fallback. Pattern: one
call per INDIRECT invocation.

**Callers.** `ScInterpreter::ScIndirect` via the indirect admission
in
[`InterpretTailEngineEvaluator.hxx`](../../inc/spreadsheetengine/compat/libreoffice/InterpretTailEngineEvaluator.hxx).

### Range resolution

Range resolution now has two explicit layers:

- **Compile-time binding** via `DocumentCompileHost`, which resolves
  names and table metadata into opaque compiler tokens.
- **Evaluation-time dereferencing** via `RangeResolver`, which turns
  direct references, named/database symbols, and INDIRECT text into
  materialized local/external bindings or a token-backed symbol that
  the evaluator can continue lowering.

#### `DocumentCompileHost::lookupRangeName` — named range resolver

Declared in
[`CompileHost.hxx`](../../inc/spreadsheetengine/compat/libreoffice/CompileHost.hxx),
implementing
`spreadsheetengine::detail::compiler::NameResolver`.

```cpp
std::optional<detail::token::NameData> lookupRangeName(
    StringView rName,
    std::optional<SheetId> onSheet,
    const CompileContext&) const override;
```

**Semantics.** Case-fold `rName`, look it up in the per-sheet
`ScRangeName` (sheet = `onSheet` if set, otherwise the formula's
base sheet), fall through to the workbook's global `ScRangeName` on
miss. Return an opaque `NameData { sheet, index }` into the host's
range-name table; the engine does not inspect the name's token
array.

**Error modes / thread safety.** Name miss → `std::nullopt` →
compiler emits `NameError`. Read-only against `ScRangeName`
snapshots; safe under the host lock during compile.

#### `DocumentCompileHost::lookupDatabaseRange` / `lookupTableReference` — DB / structured-table resolvers

```cpp
std::optional<DatabaseRangeData> lookupDatabaseRange(
    StringView rName, const CompileContext&) const override;

std::optional<TableRefData> lookupTableReference(
    StringView rTableName, StringView rItemName,
    const CompileContext&) const override;
```

**Semantics.**

- `lookupDatabaseRange` resolves a bare DB-range name
  (`Database1`, etc.) through `ScDBCollection::getNamedDBs` and
  returns an opaque `DatabaseRangeData { index }`. **No header /
  totals stripping** at this layer — stripping happens at
  dereference time inside the DB aggregate path.
- `lookupTableReference` resolves `TableName[Column]` /
  `TableName[[#Item]]` pairs. `rItemName` is the text inside the
  inner brackets (empty means "the whole table"). The item tag is
  resolved through `ScCompiler::GetOpCodeMap` / `TableRefItem`, so
  the grammar's localized `#Headers` / `#Data` / `#Totals` /
  `#This Row` tokens are recognized.

**Error modes.** Name miss or unknown item tag → `std::nullopt`.

#### `DocumentCompileHost::lookupColRowName` — implicit column/row name resolver

`ScDocOptions::IsLookUpColRowNames()` gated. Walks `GetColNameRanges`
/ `GetRowNameRanges` label ranges first, then the auto-name cache
(or a full `ScCellIterator` scan when the cache is absent). Returns
a `SingleRefData` anchored at the chosen neighbour cell. Miss →
`std::nullopt`.

#### `DocumentCompileHost::lookupExternalName` — external-name / external-range resolver

```cpp
std::optional<ExternalNameData> lookupExternalName(
    StringView rSymbol, const CompileContext& rContext) const override;
```

**Semantics.**

1. Consult the built-in external-name catalog (analysis add-in,
   etc.). Hit → return `{ kBuiltinExternalNameCatalogId, alias }`.
2. Return `std::nullopt` if the compile context disallows external
   references or the document has no `ScExternalRefManager`.
3. Run the convention's `parseExternalName` to split
   `(fileName, name)`. Reject either component exceeding `MAXSTRLEN`.
4. Absolute-qualify `fileName` via `convertToAbsName`, allocate /
   reuse an `nFileId`, validate `name` via `isValidRangeName`,
   return `{ nFileId, realName }`.

**Error modes / thread safety.** Any failure → `std::nullopt`.
`convertToAbsName` / `getExternalFileId` mutate the ref-manager's
`fileId` cache; the compiler lock serializes access.

#### External-ref fetch at evaluation time

The compile-time resolver gives the engine opaque
`(nFileId, tabName, …)` keys. At evaluation time, the host
dereferences them through
[`ExternalReferenceExecution.hxx`](../../inc/spreadsheetengine/compat/libreoffice/ExternalReferenceExecution.hxx):

```cpp
ExternalSingleRefFetch fetchExternalSingleRef(
    const ScDocument&, const ScAddress& rFormulaPos,
    sal_uInt16 nFileId, const OUString& rTabName,
    const ScSingleRefData&);

ExternalDoubleRefFetch fetchExternalDoubleRef(
    const ScDocument&, const ScAddress& rFormulaPos,
    sal_uInt16 nFileId, const OUString& rTabName,
    const ScComplexRefData&);
```

Errors map directly to `FormulaError`:

- `NoName` — file not registered with the ref manager.
- `NoRef` — tab-relative reference, out-of-range address, or a
  cache miss (`getSingleRefToken` / `getDoubleRefTokens` returned
  null or empty).
- `IllegalArgument` — cached-token shape is not the expected
  `svMatrix` / scalar.
- A propagated `svError` token carries its own inner error code.

The companion `projectExternalDoubleRefMatrix` extracts an
`ScMatrixRef` from the cache array for callers that want the
double-ref as a matrix.

#### `RangeResolver::resolveRange` / `DocumentRangeResolver` — unified evaluation-time range resolver

Declared in
[`api/RangeResolver.hxx`](../../inc/spreadsheetengine/api/RangeResolver.hxx)
and implemented for the LibreOffice host in
[`compat/libreoffice/RangeResolver.hxx`](../../inc/spreadsheetengine/compat/libreoffice/RangeResolver.hxx).

```cpp
enum class RangeResolutionKind : std::uint8_t {
    DirectReference,
    NamedReference,
    IndirectText
};

struct RangeResolutionRequest {
    RangeResolutionKind meKind;
    CellAddress maBaseAddress;
    String maPrimaryText;
    String maSecondaryText;
    AddressConvention meConvention;
    bool mbTryXlA1;
};

class RangeResolver {
public:
    virtual ValueResult<ResolvedRangeBinding> resolveRange(
        const RangeResolutionRequest& rRequest) const = 0;
};
```

`ResolvedRangeBinding::meKind` distinguishes three evaluation-time
results:

1. `LocalRange` — a concrete in-document `CellRange`.
2. `ExternalRange` — an external reference carrying
   `(mnFileId, maTabName, maRange)`.
3. `TokenBackedSymbol` — a symbol that still needs token-level
   materialization (for example an external name or a structured
   reference that does not flatten directly to a plain range).

**Semantics.**

- `DirectReference` routes through `ConvertSingleRef` /
  `ConvertDoubleRef`, returning either `LocalRange` or
  `ExternalRange`.
- `NamedReference` checks local/global `ScRangeName`, strips header /
  totals rows for named DB ranges, then falls through to compile-based
  symbol resolution so external names and structured references share
  the same evaluation-time contract.
- `IndirectText` delegates to `resolveIndirectReference`, but normalizes
  the result onto the same `ResolvedRangeBinding` surface used by the
  direct/named paths.

**Error modes.** Misses return `ValueResult::failure(...)` with the
appropriate engine error (`NoName` for unresolved named symbols,
`IllegalArgument` for malformed direct/INDIRECT text, etc.). The
resolver is intentionally "range-shaped": it never materializes a cell
value itself.

**Current production callers.** `InterpretTailEngineEvaluator.hxx`
uses `DocumentRangeResolver` in `resolveReferenceRangeNode`,
`tryParseExternalSingleRefNode`, `tryParseExternalDoubleRefNode`,
`tryParseExternalNamedRefNode`, and the authoritative `ADDRESS` /
`INDIRECT` lookup handling. That is the Phase 4 closeout: lookup and
addressing formulas no longer need duplicate Calc-side resolver logic.

### Matrix materialization

#### `materializeHostRangeToMatrixOperand`

Declared in
[`InterpretTailEngineEvaluator.hxx`](../../inc/spreadsheetengine/compat/libreoffice/InterpretTailEngineEvaluator.hxx)
under
`spreadsheetengine::compat::libreoffice::interprettaileval::detail`.

```cpp
std::optional<spreadsheetengine::core::rpn::MatrixOperand>
materializeHostRangeToMatrixOperand(
    const ScRange& rAbsRange,
    const ScDocument& rDoc,
    ScInterpreterContext& rContext);
```

**Semantics.**

- Reads the closed block `[rAbsRange.aStart, rAbsRange.aEnd]` from
  the host document in row-major order and returns a
  densely-populated `MatrixOperand`.
- Each cell is read via the canonical
  `readMaterializedHostCellValue` pipeline (imported-cache fallback,
  bounded referenced-formula materialization, display-string
  recovery, error propagation). One canonical cell-read path — no
  duplicated iteration logic.
- Empty cells are preserved as `CellValue::empty()` rather than
  coerced to zero-doubles, matching the legacy matrix-frame read.
- Error / text / boolean / number cells are preserved as their
  `api::CellValue` variants; downstream planners (determinant,
  transpose, etc.) coerce or reject them.
- The returned `MatrixOperand` carries
  `MatrixProvenance::MaterializedReference` so downstream code can
  distinguish materialized reference operands from inline literals
  or computed results.

**Error modes.**

- Multi-sheet ranges
  (`rAbsRange.aStart.Tab() != rAbsRange.aEnd.Tab()`) →
  `std::nullopt`. Callers must decline to legacy.
- Inverted / empty ranges → `std::nullopt`.
- Individual cell errors are embedded in the operand; they do not
  fail the whole materialization. Planner semantics decide whether
  to propagate (MDETERM propagates the first encountered cell error
  as the determinant result).

**Caller contract.**

- Caller owns stack-type pre-validation (`svSingleRef` /
  `svDoubleRef`; the current compat admissions only admit these two) and
  absolute-range conversion via `ScSingleRefData::toAbs` /
  `ScComplexRefData::toAbs`.
- Caller owns any scope fencing (determinant squareness, TRANSPOSE
  shape checks) after the operand is built.
- The helper does not detect self-reference; the matrix-frame path
  short-circuits that case before dispatch reaches the engine
  admission.

**Performance.** O(rows × columns) reads; no bulk fast path. Each
cell walks `readMaterializedHostCellValue`, which can invoke a
bounded referenced-formula materialization on dirty cells. Avoid
re-calling on overlapping ranges in the same dispatch pass.

**Current production consumers:** `tryPlanEngineTranspose`,
`tryPlanEngineMatrixDeterminant`, plus the compat matrix-reference
materialization bridge in
[interpr4.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr4.cxx).

**Current pairing:** lower-seam matrix admissions now pair this local-range
helper with `fetchExternalSingleRef` / `fetchExternalDoubleRef` when the
source token is external, so one MatrixOperand bridge spans both local and
external reference-backed inputs.

**Potential future consumers:** `tryPlanEngineSumProduct` family; INDEX
matrix-return form.

#### `readMaterializedHostCellValue` — canonical single-cell read

Declared in the same file.

```cpp
api::CellValue readMaterializedHostCellValue(
    const ScDocument& rDoc,
    ScInterpreterContext& rContext,
    const ScAddress& rAddress);
```

**Semantics.** Single entry point every engine admission uses when
it needs the "visible" value of a host cell. Ordered fallback:

1. Imported cached formula (hybrid formula / hybrid string / result
   string) → return cached value.
2. Formula cell with `NeedsInterpret()` → attempt
   `tryMaterializeBoundedReferencedFormulaCellValue` (depth-bounded,
   cycle-guarded via `ReferencedFormulaMaterializationGuard`).
3. Otherwise → `rDoc.GetRefCellValue` directly.
4. Empty result with a display-string representation → return the
   display-string value.
5. Error result whose formula has no code error → fall through to
   the display-string path.
6. Last resort on still-empty / error → retry the bounded
   referenced-formula materialization.

**Error modes / thread safety.** Never throws; never returns
`std::nullopt`. On any unresolvable path returns the empty /
error-valued `api::CellValue` the host surfaced. Single-threaded
through the interpreter context; a thread-local
`ReferencedFormulaMaterializationGuard::stack` detects cycles across
recursive calls.

**Callers.** Nearly every scalar / matrix materialization path in
the compat layer — `LookupAttempt` helpers,
`CriteriaAggregateMaterializer::materialize`, the matrix-frame
read, `materializeHostRangeToMatrixOperand`, and more.

Two specialized variants exist:

- `readTextParsingHostCellValue` — prefers a cell's display-string
  representation (hybrid-string result, stored result string)
  before falling through to `readMaterializedHostCellValue`. Used
  by the text-parsing family (VALUE, DATEVALUE, etc.).
- `readHostDocumentCellValue(..., [HostCellStringKind])` — the
  lowest-level host read; does NOT consult the bounded
  referenced-formula path. Other helpers layer on top of it.

All three share the "never throws, returns the empty / error
`CellValue` on unresolvable paths" contract.

### Spill allocation

#### `SpillRangeAllocator` (abstract host interface)

Declared in
[`api/Host.hxx`](../../inc/spreadsheetengine/api/Host.hxx).

```cpp
enum class SpillAllocationError : std::uint8_t {
    Collision, OutOfBounds, InvalidShape, InvalidRequest
};

class SpillRangeAllocator {
public:
    virtual ~SpillRangeAllocator() = default;

    virtual std::variant<CellRange, SpillAllocationError>
        allocateSpillRange(const CellAddress& rAnchor,
                           const MatrixDimensions& rDimensions) = 0;

    virtual bool checkSpillCollision(const CellRange& rRange) const = 0;

    virtual CellAddress getCurrentFormulaPosition() const = 0;

    virtual void markArrayFormulaBounds(const CellRange& rRange) = 0;
};
```

| Method | Purpose | Success | Failure |
|---|---|---|---|
| `allocateSpillRange` | Reserve a rectangle for a dynamic-array result anchored at `rAnchor`. | `CellRange`. | `SpillAllocationError` — Collision / OutOfBounds / InvalidShape / InvalidRequest. |
| `checkSpillCollision` | Non-destructive probe: is any non-anchor cell inside `rRange` non-empty? | `bool` — true if a collision would occur. | Never fails; single-cell ranges always return false. |
| `getCurrentFormulaPosition` | Report the sheet position of the formula currently being evaluated (matches `ScInterpreter::aPos`). | `CellAddress`. | Never fails; standalone hosts return their driver's anchor. |
| `markArrayFormulaBounds` | Record bounds after a successful allocation so downstream machinery (dependency tracking, draw-layer overlay) stays in sync. | `void`. | Never fails. |

**Error-to-Calc mapping.**

- `Collision` → `#SPILL!`. **This is new behavior.** Legacy Calc
  never emits `#SPILL!` from the dynamic-array family
  (`ScFilter`, `ScSort`, etc.). The allocator surfaces it only when
  an admitted opcode reaches it; Phase 5A admissions do not reach
  it yet.
- `OutOfBounds` / `InvalidShape` / `InvalidRequest` → bridge raises
  `api::Error::IllegalArgument`.

**Thread safety.** All four methods are invoked single-threaded
inside `ScInterpreter::Interpret` under the host document lock.
Batch group-interpretation does not currently reach the allocator.

#### `spillallocation` adapter (libreoffice)

Declared in
[`SpillAllocation.hxx`](../../inc/spreadsheetengine/compat/libreoffice/SpillAllocation.hxx).
Inline helpers in `spreadsheetengine::compat::libreoffice::spillallocation`:

- `getCurrentFormulaPosition(const ScAddress&)` — trivial
  triple-cast; the call site reads `ScInterpreter::aPos` and hands
  it in.
- `checkSpillCollision(const ScDocument&, const CellRange&)` —
  walks the closed rectangle and returns true on the first
  non-anchor `HasData` hit. Single-cell or un-normalized ranges
  return false.
- `allocateSpillRange(const ScDocument&, const CellAddress& anchor,
  const MatrixDimensions&)` — validates shape and sheet existence
  against `GetSheetLimits()`, computes the end cell, runs a
  collision probe, and returns either the committed `CellRange` or
  a `SpillAllocationError`.
- `markArrayFormulaBounds(ScDocument&, const CellRange&)` — stub.
  Current spill-shaped admissions still inherit the existing
  array-formula bounds handling through `PushMatrix`; no
  `EvaluationHost` implementation currently subclasses
  `SpillRangeAllocator`.

**Current state.** The allocator contract is real, but it is still
not bound as a `SpillRangeAllocator` subclass on any
`EvaluationHost` implementation. Existing spill-shaped planners
continue to emit matrix operands through the
`convertMatrixOperandToMatrixRef -> PushMatrix` bridge and inherit
legacy bounds handling.

### Cell read

The engine does not currently expose a first-class cell-**write**
primitive through the Host facade. Formula-cell result writes remain
a host responsibility reached through the dispatch bridge's
`PushX` / pop-and-finalize cycle.

The read-side contract is `readMaterializedHostCellValue` (see
Matrix materialization above), which is the canonical entry point
for every engine admission that needs a cell's visible value.

---

## Transition Notes

### Range iteration primitive (now landed)

**Needed for:**

- COUNTBLANK widening beyond single-matrix operands.
- SUMIF / COUNTIF / AVERAGEIF with large range criteria where the
  materialize-whole-range approach is wasteful.
- DB variance full-range accumulation for a statistically stable
  Welford path without first materializing the whole DB.
- Future streaming aggregates that cannot afford
  materialize-then-fold.

**Current state.** `RangeIterator` is now formalized on
`EvaluationHost`, and production query/countblank admissions now
route their reference scans through
`CriteriaAggregateMaterializer::iterate` (see
[`InterpretTailEngineEvaluator.hxx`](../../inc/spreadsheetengine/compat/libreoffice/InterpretTailEngineEvaluator.hxx)
and
[`QueryRuntime.hxx`](../../inc/spreadsheetengine/runtime/QueryRuntime.hxx)),
which delegates `ResolvedReference` inputs through the host walker
and falls back to row-major scalar/vector iteration for
materialized inputs.

**Contract.**

```cpp
class RangeIterator {
public:
    virtual ~RangeIterator() = default;

    // Visit each cell in `rRange` in row-major order. Returning
    // `false` from the callback aborts the walk early. The host
    // surfaces invalid ranges through the ValueResult error.
    virtual ValueResult<bool> iterateRangeCells(
        const CellRange& rRange,
        const std::function<bool(const CellAddress&,
                                 const CellValue&)>& rVisitor) const = 0;
};
```

Critical requirements remain the same: no whole-range
materialization up front; visits preserve `api::CellValue`
variants; empty cells are surfaced (not skipped) so COUNTBLANK can
count them. The LibreOffice `DocumentEvaluationHost` and the
standalone in-memory host both implement the contract, and the
query runtime now uses it in production instead of embedding its own
document walker.

### Regex / wildcard mode (now landed)

**Current state.** `RuntimeEnvironment` now exposes
`getSearchType()`, and both the LibreOffice
`DocumentEvaluationHost` adapter and the standalone in-memory host
implement it. The compat helper in
[`InterpretTailEngineEvaluator.hxx`](../../inc/spreadsheetengine/compat/libreoffice/InterpretTailEngineEvaluator.hxx)
delegates through that host method so lookup / query admissions no
longer depend on an ad hoc `ScDocOptions` read at the callsite.

**Contract.**

```cpp
class RuntimeEnvironment {
public:
    virtual api::query::SearchType getSearchType() const = 0;
    // ... existing members.
};
```

This keeps regex / wildcard / normal matching policy in the same
host-owned environment surface as locale tag and null-date policy.

### Evaluation-time `RangeResolver` follow-ups

Phase 4 landed the evaluation-time `RangeResolver` contract described
above and wired it into the production LibreOffice compat adapter.
That closes the "missing contract" blocker from the Phase 2 inventory.

The remaining follow-up is narrow: if a future non-LibreOffice host
needs to distinguish structured-table references from external names
more explicitly than `ResolvedRangeBindingKind::TokenBackedSymbol`,
the contract may grow a finer-grained discriminator. That is an
evolution concern, not an open migration blocker.

### Spill allocation host implementation (still unbound on current tree)

The abstract contract (`SpillRangeAllocator`) and the LibreOffice
inline helpers are in place, but the adapter is still not bound as a
`SpillRangeAllocator` subclass registered on an `EvaluationHost`
implementation. `markArrayFormulaBounds` therefore remains a no-op
stub on the current tree.

---

## Principles

### Add new primitive vs. extend existing

- **Prefer extending.** If an opcode family needs a slightly
  different read, first check whether
  `readMaterializedHostCellValue` can be parameterized (e.g. the
  `HostCellStringKind` enum already differentiates normal vs.
  display-string reads). Add a new primitive only when the new
  behavior meaningfully changes the shape of the return value or
  the fallback order.
- **Favor narrow, task-specific primitives** over general-purpose
  "give me anything about this cell" interfaces. The engine is
  easier to test when each host-facing helper has a single
  documented contract.
- **Push policy to the engine; keep data mechanics on the host.**
  The host is responsible for how to read a cell (imported-cache
  fallback, bounded referenced-formula materialization, display
  strings). The engine decides what to do with the result (coerce
  to double, reject text, propagate errors).
- **New primitives must land with contract documentation in this
  file in the same commit.** The acceptance audit treats any
  undocumented Host call as a regression candidate.

### Versioning policy

- The `api/Host.hxx` abstract interface is **append-only**. Adding
  a new virtual method or a new `SpillAllocationError` variant is
  an additive change that existing hosts (libreoffice compat,
  standalone test host) must implement in the same commit.
- Removing or renaming a virtual method requires a deprecation
  cycle: land the replacement alongside the old method, migrate
  callers one batch at a time, then remove the old entry.
- Behavior changes to an existing primitive (e.g. "empty cells now
  coerce to zero") require an update to the contract section here
  and a dedicated regression test.

### Testing expectations

Every admitted primitive needs:

1. **Standalone engine unit tests** in `spreadsheet_engine/qa/`
   (`query_tests`, `matrix_tests`, `spill_tests`, etc.) exercising
   the engine's view of the contract without pulling in
   `ScDocument`.
2. **Libreoffice compat tests** via `sc/qa/unit/` — at minimum one
   `ucalc_*` parity test per admission, matching the opcode's
   legacy behavior against the engine-first path.
3. **Corpus-replay validation** for any primitive that changes
   evaluator behavior: the known-regressions baseline in
   [`CALC_TEST_KNOWN_REGRESSIONS.md`](CALC_TEST_KNOWN_REGRESSIONS.md)
   must not grow, and the live authoritative-match rate reported
   by the InterpretTail migration harness must not regress.

A primitive is not "stable" (and therefore does not belong in
**Current contracts** above) until all three layers of tests are
green on the admission commit.

---

## Change log

- 2026-04 — Phase 5 inspection/metadata closeout. Retired the
  remaining Calc-owned `ExecuteTypeTerminal`,
  `ExecuteCellTerminal`, `ExecuteCellExternalTerminal`,
  `ExecuteCurrentTerminal`, `ExecuteStyleTerminal`,
  `ExecuteInfoTerminal`, and `ExecuteNTerminal` wrappers so
  inspection and metadata reads now route only through explicit
  inspection/runtime adapters or intentional host-owned side effects.
- 2026-04 — Phase 4 query/criteria/transform closeout. Retired the
  remaining Calc-owned `ExecuteSubTotalTerminal`,
  `ExecuteDBAreaTerminal`, `ExecuteSortByTerminal`, and
  `ExecuteColRowNameAutoTerminal` wrappers so query iteration, named
  DB-area resolution, and spill-shaped transforms now route only
  through the explicit host/runtime contracts already documented
  above.
- 2026-04 — Phase 4 range-resolution closeout. Landed
  `api/RangeResolver.hxx` plus the LibreOffice
  `DocumentRangeResolver` adapter, documented the unified
  evaluation-time range contract, and updated the inventory rows so
  reference / lookup / addressing work is no longer blocked on a
  missing resolver contract.
- 2026-04 — Phase I closeout sweep. Extended the Phase D stub with
  explicit contract sections for address resolution
  (`resolveIndirectReference`), named / external / DB range
  resolution (`DocumentCompileHost`), spill allocation
  (`SpillRangeAllocator` + libreoffice adapter), and canonical
  cell read (`readMaterializedHostCellValue`). Documented pending
  surfaces for range iteration, regex mode, and the
  evaluation-time range resolver.
- earlier — Phase D. Initial stub covering only
  `materializeHostRangeToMatrixOperand`.

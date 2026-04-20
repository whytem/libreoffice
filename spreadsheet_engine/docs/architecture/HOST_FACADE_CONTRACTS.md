# Host Facade Contracts

Phase I deliverable for the
[RPN Evaluator Close-Out Plan](CLOSE_OUT_PLAN.md). This document
inventories the stable, explicit contracts the engine RPN evaluator
depends on when it asks the host for document-side information it
cannot compute in isolation: address resolution, named / external /
database range resolution, range iteration, regex / wildcard mode,
spill allocation, and matrix materialization.

## How to read this doc

- **Current contracts** — primitives already in production on the
  engine side and the `libreoffice` compat adapter. Each entry lists
  signature, semantics, preconditions, error modes, thread safety,
  and representative callers.
- **Pending contracts** — surfaces still implicit (scattered across
  ad-hoc call sites, or missing entirely), with the shape we expect
  when they are formalized.
- **Principles** — policy for when to add a new primitive vs.
  extend one, how contracts are versioned, and what level of tests
  an admission needs before it is marked stable.

The primary source of truth is always the code. If a contract below
disagrees with the header, the header wins and this document must be
updated in the same commit.

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

Range resolution is currently split between **compile-time** binding
(via `DocumentCompileHost`) and **evaluation-time** dereferencing
(still inside the legacy dispatch tail). The engine sees opaque
indices at RPN-generation time and asks the host to dereference them
at evaluation time.

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

### Matrix materialization

#### `materializeHostRangeToMatrixOperand` (Phase D)

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
  `svDoubleRef`; Phase D admissions only admit these two) and
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

**Consumers (Phase D landing):** `tryPlanEngineTranspose`,
`tryPlanEngineMatrixDeterminant`.

**Future consumers:** `tryPlanEngineMatrixMultiply`,
`tryPlanEngineMatrixInverse` (Phase C widening);
`tryPlanEngineSumProduct` family (Phase E); INDEX matrix-return
form (Phase D follow-up).

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
  Phase 5A admissions inherit the existing array-formula bounds
  handling through `PushMatrix`; Phase 5B will fill this in.

**Phase status.** Phase 5A (FILTER / SORT / SORTBY / UNIQUE / TAKE /
DROP) does not yet reach the allocator — those planners emit matrix
operands directly through the existing
`convertMatrixOperandToMatrixRef → PushMatrix` bridge and inherit
legacy bounds handling. Phase 5B (HSTACK / VSTACK / CHOOSECOLS /
CHOOSEROWS / EXPAND / TOCOL / TOROW / WRAPCOLS / WRAPROWS /
TEXTSPLIT) is expected to bind the adapter as a
`SpillRangeAllocator` subclass on an `EvaluationHost`.

### Cell read

The engine does not currently expose a first-class cell-**write**
primitive through the Host facade. Formula-cell result writes remain
a host responsibility reached through the dispatch bridge's
`PushX` / pop-and-finalize cycle.

The read-side contract is `readMaterializedHostCellValue` (see
Matrix materialization above), which is the canonical entry point
for every engine admission that needs a cell's visible value.

---

## Contracts still pending

### Range iteration primitive (not yet formalized)

**Needed for:**

- COUNTBLANK widening beyond single-matrix operands.
- SUMIF / COUNTIF / AVERAGEIF with large range criteria where the
  materialize-whole-range approach is wasteful.
- DB variance full-range accumulation for a statistically stable
  Welford path without first materializing the whole DB.
- Future streaming aggregates that cannot afford
  materialize-then-fold.

**Current stopgap.** Every range iteration today goes through
`CriteriaAggregateMaterializer::materialize` (see
[`InterpretTailEngineEvaluator.hxx`](../../inc/spreadsheetengine/compat/libreoffice/InterpretTailEngineEvaluator.hxx)
and
[`QueryRuntime.hxx`](../../inc/spreadsheetengine/runtime/QueryRuntime.hxx)),
which takes a `CriteriaAggregateInput` carrying either a scalar, a
pre-materialized `std::vector<CellValue>`, or a `ResolvedReference`
with dimensions, and walks it cell-by-cell through
`readMaterializedHostCellValue`. That pattern works today but is
not a standalone primitive — iteration is fused into the query
evaluator.

**Desired shape (NOT YET LANDED):**

```cpp
class RangeIterator {
public:
    virtual ~RangeIterator() = default;

    // Visit each cell in `rRange` in row-major order. Returning
    // `false` from the callback aborts the walk early.
    virtual void iterateRangeCells(
        const CellRange& rRange,
        const std::function<bool(const CellAddress&,
                                 const CellValue&)>& rVisitor) const = 0;
};
```

Equivalently, an `ScCellIterator`-compatible forward walker that the
compat layer adapts from `dociter.hxx`. Critical requirements: no
whole-range materialization up front; visits preserve
`api::CellValue` variants; empty cells are surfaced (not skipped) so
COUNTBLANK can count them.

**Blocker.** The engine query evaluator currently prefers the
materialize-whole-range model because it composes cleanly with the
matrix-arithmetic substrate. A streaming primitive only pays off
when the aggregate does not need a second pass; the first target is
COUNTBLANK against whole-column references.

### Regex / wildcard mode (partial — currently read from `ScDocOptions` directly)

**Current state.** A single compat-layer helper,
`searchTypeFromDocument(const ScDocument&)` in
[`InterpretTailEngineEvaluator.hxx`](../../inc/spreadsheetengine/compat/libreoffice/InterpretTailEngineEvaluator.hxx),
reads `rDoc.GetDocOptions()` and maps `IsFormulaRegexEnabled()` /
`IsFormulaWildcardsEnabled()` to `api::query::SearchType` (one of
`Normal`, `Wildcard`, `Regex`). Every lookup / query admission calls
this helper and threads the result into the pure engine comparator.

**Desired formalization.** Promote the helper to a first-class
`RuntimeEnvironment` member:

```cpp
class RuntimeEnvironment {
public:
    virtual api::query::SearchType getSearchType() const = 0;
    // ... existing members.
};
```

Low-risk rename once the standalone test host grows the same
surface. Removes one `ScDocument` dependency from the engine-first
planners.

### Consolidated evaluation-time range resolver

**Current state.** Three different surfaces resolve ranges today:

1. **Compile-time** — `DocumentCompileHost` (above) covers the
   compile path through the `NameResolver` /
   `DatabaseRangeResolver` / `TableRefResolver` /
   `ColRowNameResolver` / `ExternalNameResolver` interfaces.
2. **Indirect-time** — `resolveIndirectReference` (above) runs an
   ad-hoc ordered search that partly duplicates the compile-host
   logic.
3. **Evaluation-time** — the legacy `ScInterpreter` body still
   bridges external names / DB references through its own
   `ScInterpreter::*` entries.

**Desired formalization.** A single evaluation-time resolver
paralleling the compile-host interfaces but returning materialized
`ResolvedReference` / `CellRange` shapes instead of opaque indices:

```cpp
class RangeResolver {
public:
    virtual ValueResult<ResolvedReference> resolveNamedRange(
        StringView rName, std::optional<SheetId> onSheet) const = 0;
    virtual ValueResult<ResolvedReference> resolveDatabaseRange(
        StringView rName) const = 0;
    virtual ValueResult<ResolvedReference> resolveExternalReference(
        sal_uInt16 nFileId, StringView rTabName,
        const SingleRefData&) const = 0;
    // double-ref variant; structured-table variant; ...
};
```

**Blocker.** The three existing paths have subtly different
behavior (header / totals stripping on DB ranges via indirect but
not via compile-time; structured-table evaluation is deferred to the
runtime token resolver, etc.). Unifying them into a single
evaluation-time surface is a substrate project, not a refactor.

### Spill allocation host implementation (Phase 5B)

The abstract contract (`SpillRangeAllocator`) and the libreoffice
inline helpers are in place, but the adapter has not yet been bound
as a `SpillRangeAllocator` subclass registered on an
`EvaluationHost` implementation. The binding lands with Phase 5B
when the first shape-reshaping spill admission (HSTACK / VSTACK /
…) reaches the allocator. `markArrayFormulaBounds` is a no-op stub
until then.

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

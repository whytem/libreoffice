# Host Facade Contracts

Stub tracking the stable explicit contracts that the engine RPN
evaluator relies on the Host facade to provide. Each contract listed
here has been admitted to production use via a specific opcode family;
new admissions that need primitives not listed here must extend this
document in the same commit.

The full catalog (address resolution, named / external / DB range
resolution, range iteration, regex mode, spill allocation, matrix
materialization) is the deliverable for Phase I of the
[Close-Out Plan](CLOSE_OUT_PLAN.md). This file starts with the Phase D
primitive and grows one section per phase until the Phase I sweep
backfills the earlier host contracts.

## `materializeHostRangeToMatrixOperand` (Phase D)

Declared in
[`spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/InterpretTailEngineEvaluator.hxx`](../../inc/spreadsheetengine/compat/libreoffice/InterpretTailEngineEvaluator.hxx)
under `spreadsheetengine::compat::libreoffice::interprettaileval::detail`:

```cpp
[[nodiscard]] inline std::optional<spreadsheetengine::core::rpn::MatrixOperand>
materializeHostRangeToMatrixOperand(
    const ScRange& rAbsRange,
    const ScDocument& rDoc,
    ScInterpreterContext& rContext);
```

Semantics:

- Reads the half-open-closed cell block `[rAbsRange.aStart,
  rAbsRange.aEnd]` (inclusive on both ends) from the host document in
  row-major order and returns a densely-populated
  `serpn::MatrixOperand`.
- Each cell is read via the canonical
  `readMaterializedHostCellValue(rDoc, rContext, aAddress)` pipeline,
  which handles imported-cache fallback, bounded referenced-formula
  materialization, display-string recovery, and error propagation in
  the same order the rest of the compat layer uses. This keeps one
  canonical cell-read path rather than duplicating iteration logic.
- Empty cells are preserved as `CellValue::empty()` rather than being
  coerced to zero-doubles, matching the legacy matrix-frame read.
- Error / text / boolean / number cells are preserved as their
  `api::CellValue` variants; downstream planners (determinant,
  transpose, etc.) are responsible for coercing or rejecting them.
- The returned `MatrixOperand` carries
  `MatrixProvenance::MaterializedReference` so downstream code can
  distinguish materialized reference operands from inline literals or
  computed results.

Error modes:

- Multi-sheet ranges (`rAbsRange.aStart.Tab() != rAbsRange.aEnd.Tab()`)
  return `std::nullopt`. Callers must decline to legacy.
- Inverted / empty ranges return `std::nullopt`.
- Individual cell errors are embedded in the operand; they do not fail
  the whole materialization. Planner semantics decide whether to
  propagate them (e.g., MDETERM propagates the first encountered cell
  error as the determinant result).

Caller contract:

- The caller owns stack-type pre-validation (must be `svSingleRef`,
  `svDoubleRef`, or a `svRefList` collapsed to a single range; the
  Phase D admissions only admit the first two) and absolute-range
  conversion (via `ScSingleRefData::toAbs` / `ScComplexRefData::toAbs`
  against the active formula position).
- The caller owns any additional scope fencing (e.g. determinant
  squareness, TRANSPOSE shape checks) after the operand is built.
- The caller must NOT pass an `rAbsRange` that contains the formula's
  own position without expecting the usual self-reference semantics;
  the helper does not detect self-reference because the matrix-frame
  path already short-circuits that case before dispatch reaches the
  engine admission.

Consumers (as of Phase D):

- `tryPlanEngineTranspose` (MDETERM widened in the same commit).
- `tryPlanEngineMatrixDeterminant`.

Future consumers (will follow when their phase lands):

- `tryPlanEngineMatrixMultiply` (Phase C — may pick up this bridge
  once both operands can be matrix-shaped via range materialization).
- `tryPlanEngineMatrixInverse` (Phase C).
- `tryPlanEngineSumProduct` family (Phase E).
- INDEX matrix-return form (Phase D follow-up).

## Pending contracts (stubs)

The following contracts are in-use but not yet documented here.
Phase I will pick them up:

- Address resolution (`ScSingleRefData::toAbs`,
  `ScComplexRefData::toAbs`, external-ref resolution).
- Named range resolution.
- Database range resolution.
- Streaming range iteration (already used by Batch 3; see
  `CriteriaAggregateMaterializer`).
- Regex / wildcard mode negotiation.
- Spill allocation (not yet implemented; see Phase F).

# Host-Boundary Audit For Full RPN Evaluation

Status: initial audit, active working document

## Purpose

This document begins the host-boundary audit for a full engine-side RPN
evaluator.

The goal is to define the minimal host contract required by the remaining
Calc evaluator subsystem so that `spreadsheet_engine/` can own the execution
model without re-absorbing broad Calc runtime state.

Current honest baseline:

- `legacy_interpreter_subroutine_count=100`
- `interp4_dispatch_legacy_lambda_count=62`
- `interp4_dispatch_legacy_dispatch_target_count=62`
- `interp4_dispatch_legacy_call_count=80`

## Audit Scope

The remaining work is not a flat list of `ScXxx()` wrappers.

It includes:

- surviving `Sc*` methods in [interpre.hxx](/home/ubuntu/repos/libreoffice/sc/source/core/inc/interpre.hxx)
- surviving `pushLegacy*` lambdas in [interpr4.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr4.cxx)
- the supporting stack and token-iteration machinery in
  [interpr4.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr4.cxx)

This audit categorizes the host services those surfaces still depend on.

## Category 1: Reference Resolution

Required host services:

- resolve single-cell, range, named, and 3D references
- resolve external references
- preserve reference-shaped operands as references, not only as scalars
- materialize references to matrices when a consumer requires matrix form

Representative surviving Calc surfaces:

- `ScIndirect`
- `ScOffset`
- `ScIndex`
- `ScLookup`
- `ScXLookup`
- `ScAddressFunc`
- `ScMultiArea`
- `ScAreas`
- `ScExternal`
- `ScMissing`
- `PopSingleRef`
- `PopDoubleRef`
- `PopDoubleRefPushMatrix`
- `PopExternalSingleRef`
- `PopExternalDoubleRef`

Immediate implication:

- the engine needs a first-class reference operand model, not just scalar and
  matrix materialization helpers

## Category 2: Cell Type And Format Inspection

Required host services:

- inspect cell type and stored result kind
- inspect or propagate number format / expression format
- preserve formatting decisions that affect function semantics

Representative surviving Calc surfaces:

- `ScType`
- `ScCell`
- `ScCellExternal`
- `ScCurrent`
- `ScStyle`
- `pushLegacyColor`
- `pushLegacyText`
- `pushLegacyCurrency`
- `pushLegacyFixed`

Immediate implication:

- the host contract needs an explicit cell-inspection service, separate from
  plain value reads

## Category 3: Document Iteration And Criteria

Required host services:

- iterate ranges with row/column/document semantics intact
- expose row hidden / filtered status
- evaluate criteria and database query entries
- preserve database-range and subtotal semantics

Representative surviving Calc surfaces:

- `ScCountIf`
- `ScSubTotal`
- `ScDBSum`
- `ScDBCount`
- `ScDBCount2`
- `ScDBAverage`
- `ScDBGet`
- `ScDBMax`
- `ScDBMin`
- `ScDBProduct`
- `ScDBStdDev`
- `ScDBStdDevP`
- `ScDBVar`
- `ScDBVarP`
- `ScDBArea`
- `ScFilter`
- `ScSort`
- `ScSortBy`
- `ScUnique`
- `ScExpand`
- `ScTextSplit`

Immediate implication:

- the engine needs a dedicated range-walk / criteria service rather than
  one-off ad hoc host calls per function

## Category 4: Context State And Control Flow

Required host services:

- current evaluation position
- token iterator / jump target state
- lazy branch evaluation
- `LET` binding context
- array-aware jump behavior

Representative surviving Calc surfaces:

- `ScIfJump`
- `ScIfJumpNotMatrix`
- `ScChooseJump`
- `ScLet`
- `ScSyntheticBinaryOp`
- `MatrixJumpConditionToMatrix`
- `ConvertMatrixJumpConditionToMatrix`

Immediate implication:

- control-flow opcodes must move as part of an engine RPN loop, not as
  isolated leaf functions

## Category 5: Operator Semantics

Required host services:

- polymorphic stack value coercion
- error convergence rules
- matrix broadcast rules
- reference/scalar coercion
- format propagation through operator results

Representative surviving Calc surfaces:

- `ScAmpersand`
- `ScMul`
- `ScDiv`
- `ScPow`
- `ScCompareOp`
- `ScLogicalFoldOp`
- `ScUnaryMatrixOrScalarOp`
- the remaining unseen `operator:+` tail on the corpus

Immediate implication:

- the next meaningful engine step is a typed operator subsystem, not another
  sequence of leaf-function migrations
- the initial contract now exists as:
  - `RpnValueKind::{Empty, Number, Boolean, String, Error, Reference, Matrix}`
  - `RpnCoercionReadiness::{Ready, NeedsReferenceResolution, NeedsMatrixMaterialization}`
  - unary/binary scalar operator readiness classifiers that can refuse
    reference and matrix operands explicitly
- the first scalar operator layer now exists in `RpnOperators.hxx`, which
  mirrors Calc's current scalar arithmetic / concat / comparison semantics
  without routing live opcodes through the engine yet

## Category 6: Matrix Frame And Spill State

Required host services:

- matrix allocation
- array broadcast state
- matrix-reference propagation
- spill-shaped result semantics

Representative surviving Calc surfaces:

- `ScMatValue`
- `ScMatInv`
- `ScMatMult`
- `ScMatSequence`
- `ScMatTrans`
- `ScEMat`
- `ScMatRef`
- `ScFrequency`
- `ScLinest`
- `ScLogest`
- `ScTrend`
- `ScForecast_Ets`
- `ScChooseColsOrRows`
- `ScToColOrRow`
- `ScWrapColsOrRows`
- `ScTakeOrDrop`
- `ScHorizontalOrVerticalStack`

Immediate implication:

- the engine needs explicit matrix-frame state rather than relying on
  `ScInterpreter` instance state

## Category 7: System State

Required host services:

- locale and collation behavior
- calendar/date-system behavior
- deterministic random source policy
- regex/text-search runtime behavior

Representative surviving Calc surfaces:

- `ScRandom`
- `ScRandbetween`
- `ScRandArray`
- `ScRandomImpl`
- `pushLegacySearch`
- `pushLegacyRegex`
- `pushLegacyTextBeforeAfter`
- `pushLegacyDateOrTimeValue`

Immediate implication:

- the engine contract should separate deterministic system services from
  document services so the standalone package does not silently re-grow broad
  host state

## Category 8: External Computation

Required host services:

- macro execution
- DDE / external links
- web / XML fetch behavior
- pivot-table lookups
- hyperlink-specific behavior

Representative surviving Calc surfaces:

- `ScMacro`
- `ScDde`
- `ScWebservice`
- `ScFilterXML`
- `ScGetPivotData`
- `ScHyperLink`

Immediate implication:

- these should be treated as explicit host terminals unless we intentionally
  decide they belong inside the engine contract

## Category 9: Stack Runtime Compatibility

Required host services:

- typed push/pop operations
- global error state
- number-format side channel
- token stack reversal and temporary token handling

Representative surviving Calc surfaces:

- `PushDouble`
- `PushString`
- `PushMatrix`
- `PushError`
- `Pop`
- `PopError`
- `ReverseStack`
- temporary-token push helpers in `interpr4.cxx`

Immediate implication:

- the engine needs its own stack-value model and evaluator state object before
  the remaining operator and control-flow migration can be honest
- the first piece of that stack model is now landed in `RpnValue.hxx`, but the
  evaluator state object itself is still outstanding

## Remaining `pushLegacy*` Surface By Cluster

The surviving `pushLegacy*` lambdas are already clustered by subsystem rather
than by isolated function:

- text / formatting / search:
  `pushLegacyText`, `pushLegacyFind`, `pushLegacySearch`,
  `pushLegacyRegex`, `pushLegacyTextBeforeAfter`, `pushLegacyTextJoinMs`,
  `pushLegacyBahtText`, `pushLegacyCurrency`, `pushLegacyFixed`,
  `pushLegacyReplace`, `pushLegacySubstitute`, `pushLegacyRept`,
  `pushLegacyConcat`, `pushLegacyConcatMs`, `pushLegacyExact`,
  `pushLegacyEncodeUrl`, `pushLegacyValue`, `pushLegacyNumberValue`,
  and the `*B` byte-text variants
- logical / conditional / predicate:
  `pushLegacyIfJump`, `pushLegacyIfError`, `pushLegacyIfs`,
  `pushLegacySwitch`, `pushLegacyLogicalFold`, `pushLegacyNot`,
  `pushLegacyIsEmpty`, `pushLegacyIsString`, `pushLegacyIsLogical`,
  `pushLegacyIsRef`, `pushLegacyIsValue`, `pushLegacyIsFormula`,
  `pushLegacyIsNA`, `pushLegacyIsErrLike`
- scalar math / finance / helper:
  `pushLegacyGcdOrLcm`, `pushLegacyCombin`, `pushLegacyBitwise`,
  `pushLegacyNpv`, `pushLegacyIrr`, `pushLegacyMirr`,
  `pushLegacyDateOrTimeValue`, `pushLegacyRawSubtract`, `pushLegacyColor`
- formula-source helper:
  `pushLegacyFormulaText`

That clustering reinforces the subsystem framing: the next honest drops in the
lambda metric should come from shared engine primitives, not one-off lambda
removals.

## Initial Conclusions

The audit already points to three immediate decisions:

1. the next engine milestone should be stack value model plus operator
   dispatch, because that removes a true subsystem blocker rather than one
   more leaf wrapper
2. no new `pushLegacy*` lambdas should be added for families the engine does
   not already own at the root
3. external computation surfaces (`Macro`, `DDE`, `Webservice`, `FilterXML`,
   `GetPivotData`) should be treated as explicit host-boundary questions, not
   silently absorbed into the engine

## Next Audit Steps

The next pass on this document should:

1. map every remaining `Sc*` method and surviving `pushLegacy*` lambda to one
   or more host-service categories
2. define the minimal engine-facing interface for each category
3. mark which categories are in-scope for `RpnEvaluator` and which stay
   host-owned
4. align `Host.hxx` and future compat helpers against that fixed contract

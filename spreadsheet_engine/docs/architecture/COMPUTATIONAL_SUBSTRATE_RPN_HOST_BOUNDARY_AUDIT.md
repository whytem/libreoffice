# Host-Boundary Audit For Full RPN Evaluation

Status: phase-2 audit complete, active reference document

## Purpose

This document began the host-boundary audit for a full engine-side RPN
evaluator.

The goal is to define the minimal host contract required by the remaining
Calc evaluator subsystem so that `spreadsheet_engine/` can own the execution
model without re-absorbing broad Calc runtime state.

Canonical current metrics now live in
[../PROJECT_STATUS.md](../PROJECT_STATUS.md). The explicit contract inventory
produced by this audit now lives in
[HOST_FACADE_CONTRACTS.md](HOST_FACADE_CONTRACTS.md). This file remains the
classification and boundary-analysis memo behind that inventory, not the moving
dashboard snapshot.

## Phase 2 Result

Phase 2 of
[COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_PIVOT_PLAN.md](COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_PIVOT_PLAN.md)
is complete on the current tree:

- every remaining legacy cluster is now mapped to one or more host-service
  categories
- [HOST_FACADE_CONTRACTS.md](HOST_FACADE_CONTRACTS.md) now records the
  contract-status matrix for those categories using the pivot plan's four
  labels:
  - `already exposed`
  - `exposed but too broad`
  - `missing`
  - `intentionally unsupported`
- the next subsystem phases can now point at named missing contracts
  (`RangeIterator`, evaluation-time `RangeResolver`, explicit search-policy
  API, formula-inspection API) instead of reopening the audit from scratch

## Audit Scope

The remaining work is not a flat list of `ScXxx()` wrappers.

It includes:

- surviving `Sc*` methods in [interpre.hxx](/home/ubuntu/repos/libreoffice/sc/source/core/inc/interpre.hxx)
- surviving `pushLegacy*` lambdas in [interpr4.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr4.cxx)
- the supporting stack and token-iteration machinery in
  [interpr4.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr4.cxx)

This audit categorizes the host services those surfaces still depend on.

## Phase 2 Service Ledger

The service IDs below are the stable labels used by the remaining-surface
inventory later in this document.

- `HS1` visible scalar cell read / text parse / value formatting
  Current contracts: `CellReader`, `TextCoercion`, `ValueFormatting`,
  `readMaterializedHostCellValue`, `readTextParsingHostCellValue`.
  Status: `already exposed`.
  Ownership: host performs document reads and formatting; engine owns coercion
  and function semantics.
- `HS2` compile-time named / DB / table / external resolution
  Current contracts: `DocumentCompileHost` resolver interfaces in
  `CompileHost.hxx`.
  Status: `already exposed`.
  Ownership: host owns document catalogs and compiler context; engine consumes
  opaque compile-time identities.
- `HS3` evaluation-time concrete reference normalization
  Current contracts: `api::ReferenceResolver`, `ResolvedReference`,
  external-ref fetch helpers.
  Status: `exposed but too broad`.
  Ownership: host normalizes concrete cell/range coordinates; engine should not
  see `ScAddress` / `ScRange` internals.
  Gap: this does not yet unify named / DB / table / external symbolic
  resolution at evaluation time.
- `HS4` evaluation-time symbolic range resolution
  Current shape: `resolveIndirectReference` plus scattered legacy interpreter
  bodies.
  Status: `missing`.
  Ownership: host must resolve names / DB ranges / table refs / external refs
  into `ResolvedReference`-shaped results; engine owns downstream execution.
- `HS5` matrix materialization and matrix-reference projection
  Current contracts: `materializeHostRangeToMatrixOperand`,
  `projectExternalDoubleRefMatrix`, matrix-reference aware `CellValueView`.
  Status: `already exposed`.
  Ownership: host reads cells and cache arrays; engine owns matrix planners and
  coercion.
- `HS6` range iteration / criteria walk
  Current shape: `CriteriaAggregateMaterializer` stopgap embedded in compat
  code.
  Status: `missing`.
  Ownership: host should provide ordered range walking with visibility /
  emptiness semantics; engine owns aggregate/query policy.
- `HS7` cell type / format inspection
  Current shape: direct `ScDocument` / `ScRefCellValue` inspection and a
  handful of compat helpers.
  Status: `exposed but too broad`.
  Ownership: host should answer narrow questions about cell kind / format /
  inspection data; engine should stop poking raw document internals.
- `HS8` runtime search / regex / wildcard policy
  Current shape: `searchTypeFromDocument(const ScDocument&)`.
  Status: `missing`.
  Ownership: host supplies search policy (`Normal` / `Wildcard` / `Regex`);
  engine owns matching semantics.
- `HS9` spill allocation lifecycle
  Current contracts: `SpillRangeAllocator` and
  `compat::libreoffice::spillallocation::*`.
  Status: `already exposed`.
  Ownership: host owns collision checks and spill reservation; engine owns
  result-shape planning.
  Gap: the abstract allocator is defined, but `EvaluationHost` binding and
  downstream bounds lifecycle are still pending.
- `HS10` runtime environment / workbook metadata
  Current contracts: `WorkbookInfo`, `RuntimeEnvironment`.
  Status: `already exposed`.
  Ownership: host owns sheet catalog, locale tag, null-date, and document
  environment.
- `HS11` evaluator state / control flow / typed stack
  Current shape: `RpnValue.hxx` plus legacy `ScInterpreter` state.
  Status: `missing`.
  Ownership: engine must own the evaluator state object, typed stack, operator
  dispatch, and control-flow loop; host should not re-grow stack-machine state.
- `HS12` external computation terminals
  Current shape: Calc-only interpreter terminals (`ScMacro`, `ScDde`,
  `ScWebservice`, `ScFilterXML`, `ScGetPivotData`, `ScHyperLink`).
  Status: `intentionally unsupported`.
  Ownership: remain host terminals unless the project explicitly decides to
  surface them through a dedicated engine contract.

## Phase 2 Remaining Legacy Surface Inventory

Every remaining `Sc*` method and `pushLegacy*` lambda is now mapped to one or
more service IDs from the ledger above.

### `Sc*` methods by primary host-service dependency

- `HS11` evaluator state / control flow / typed stack:
  `ScTableOp`, `ScLet`, `ScCompareOp`, `ScLogicalFoldOp`,
  `ScUnaryMatrixOrScalarOp`, `ScSyntheticBinaryOp`, `ScAmpersand`, `ScMul`,
  `ScDiv`, `ScPow`, `ScTTT`, `ScDebugVar`
- `HS4` symbolic range resolution with `HS3` / `HS5` follow-through:
  `ScIntersect`, `ScRangeFunc`, `ScUnionFunc`, `ScLookup`, `ScXLookup`,
  `ScMatchOp`, `ScIndirect`, `ScAddressFunc`, `ScIndex`, `ScMultiArea`,
  `ScExternal`, `ScMissing`, `ScColRowNameAuto`
- `HS7` cell type / format inspection:
  `ScType`, `ScCell`, `ScCellExternal`, `ScCurrent`, `ScStyle`, `ScInfo`
- `HS6` range iteration / criteria walk:
  `ScSubTotal`, `ScDBArea`
- `HS5` matrix materialization / matrix frame, often with `HS6`:
  `ScSortBy`, `ExecuteMatValueTerminal`, `ExecuteMatRefTerminal`,
  `ExecuteSumXMY2Terminal`, `ExecuteFourierTerminal`,
  `ExecuteFrequencyTerminal`, `ExecuteForecastEtsTerminal`
- `HS10` runtime environment / workbook metadata:
  `ExecuteRandomTerminal`, `ExecuteRandbetweenTerminal`,
  `ExecuteRandArrayTerminal`
- `HS1` visible scalar read / text parse / formatting plus `HS11` coercion:
  `ScN`
- `HS12` intentionally unsupported host terminals:
  `ScMacro`, `ScDde`, `ScGetPivotData`, `ScHyperLink`, `ScFilterXML`,
  `ScWebservice`

### `pushLegacy*` lambdas by primary host-service dependency

- `HS11` evaluator state / operator substrate:
  `pushLegacyGcdOrLcm`, `pushLegacyCombin`, `pushLegacyBitwise`,
  `pushLegacyTextJoinMs`, `pushLegacyConcatMs`
- `HS1` visible scalar read / text parse / formatting:
  `pushLegacyReplace`, `pushLegacySubstitute`, `pushLegacyLeftRight`,
  `pushLegacyEncodeUrl`, `pushLegacyRightB`, `pushLegacyLeftB`,
  `pushLegacyMidB`, `pushLegacyReplaceB`
- `HS8` runtime search / regex / wildcard policy, with `HS1` string material:
  `pushLegacyRegex`, `pushLegacyFindB`, `pushLegacySearchB`
- `HS7` cell type / format inspection:
  `pushLegacyCurrency`, `pushLegacyText`
- `HS10` runtime environment / locale-sensitive text services:
  `pushLegacyUnaryTextTransform`, `pushLegacyTextBeforeAfter`,
  `pushLegacyBahtText`

These inventories are intentionally exhaustive for the current tree. When the
remaining legacy surface changes, this document must be updated in the same
commit.

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

- `ExecuteMatValueTerminal`
- `ScMatInv`
- `ScMatMult`
- `ScMatSequence`
- `ScMatTrans`
- `ScEMat`
- `ExecuteMatRefTerminal`
- `ExecuteFrequencyTerminal`
- `ScLinest`
- `ScLogest`
- `ScTrend`
- `ExecuteForecastEtsTerminal`
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
- deterministic random source policy via `RuntimeEnvironment::sampleUniformReal()`
- regex/text-search runtime behavior

Representative surviving Calc surfaces:

- `ExecuteRandomTerminal`
- `ExecuteRandbetweenTerminal`
- `ExecuteRandArrayTerminal`
- `pushLegacySearch`
- `pushLegacyRegex`
- `pushLegacyTextBeforeAfter`
- `pushLegacyDateOrTimeValue`

Immediate implication:

- the engine contract now separates deterministic system services from
  document services explicitly: random draws come from
  `RuntimeEnvironment::sampleUniformReal()`, while Calc keeps only the
  terminal-level matrix/stack-shape bridge

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

## Phase 2 Conclusions

Phase 2 closes the audit from "next steps" into a usable contract baseline:

1. every remaining legacy execution surface is now mapped to an explicit host
   service ledger entry
2. the missing contracts are narrowed to a small set:
   `HS4`, `HS6`, `HS8`, and `HS11`
3. the intentionally host-terminal set is explicit:
   `HS12`
4. the next subsystem phases can now point at concrete missing contracts
   instead of inventing new Host surfaces ad hoc

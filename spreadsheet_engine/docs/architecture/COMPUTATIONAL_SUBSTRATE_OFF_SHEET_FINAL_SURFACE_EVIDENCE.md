# Computational Substrate Off-Sheet Final Surface Evidence

Status: completed evidence for the bounded off-sheet final-surface pass

## Admitted Result

This pass admitted the following new bounded off-sheet family:

- exact same-workbook one-consumer-sheet off-sheet named-range-combined
  `GlobalSingleAreaSingleConsumerSheet` `MemberExit` for
  `SetScalarValue`, `SetFormula`, and `ClearCell`

## Host-Shape Resolution

The pass also resolved the remaining direct merge-shaped off-sheet questions
with explicit live Calc proof:

- the bounded direct off-sheet replacement attempt is not a distinct live
  `ReplacementMerge`; live Calc classifies it as `Regroup`
- the bounded direct off-sheet gap-closing insertion does produce merged
  after-topology, but live Calc leaves the mutation-family classification
  at `None`

That means the replacement attempt is already covered by the admitted
direct off-sheet `Regroup` lane, while the gap-closing insertion remains a
retained off-sheet defer until the host exposes a stable family contract.

## Standalone Proof

Engine-only exactness proof is covered in:

- [../../tests/unit/workbook_facade_tests.cxx](../../tests/unit/workbook_facade_tests.cxx)
- [../../tests/unit/computational_substrate_tests.cxx](../../tests/unit/computational_substrate_tests.cxx)

Those buckets now demonstrate:

- bounded off-sheet named-range-combined `MemberExit` classification
- exact authority planning for bounded off-sheet named-range-combined
  `MemberExit`
- exact lifecycle planning for bounded off-sheet named-range-combined
  `MemberExit`
- synthetic direct off-sheet merge-shaped exactness remains representable in
  engine-only modeling, even where live Calc does not expose the same
  family contract

## Live Calc Proof

Live Calc proof is covered in:

- [../../../sc/qa/unit/ucalc_workbook_facade.cxx](../../../sc/qa/unit/ucalc_workbook_facade.cxx)
- [../../../sc/qa/unit/ucalc_dependency_shadow.cxx](../../../sc/qa/unit/ucalc_dependency_shadow.cxx)

Admitted live buckets now include:

- authority off-sheet named-range-combined `MemberExit` `SetScalarValue`
- authority off-sheet named-range-combined `MemberExit` `SetFormula`
- authority off-sheet named-range-combined `MemberExit` `ClearCell`
- lifecycle off-sheet named-range-combined `MemberExit` `SetFormula`
- lifecycle off-sheet named-range-combined `MemberExit` `ClearCell`
- mutation-entry off-sheet named-range-combined `MemberExit`
  `SetScalarValue`
- mutation-entry off-sheet named-range-combined `MemberExit`
  `SetFormula`
- mutation-entry off-sheet named-range-combined `MemberExit` `ClearCell`

Host-shape proof now also freezes:

- direct off-sheet replacement attempt actual host shape is `Regroup`
- direct off-sheet gap-closing insertion actual host shape is
  mutation-family `None` with merged after-topology

## Validation

Validated in this pass:

- `spreadsheetengine_workbook_facade_tests`
- `spreadsheetengine_computational_substrate_tests`
- `CppunitTest_sc_ucalc_workbook_facade`
- `CppunitTest_sc_ucalc_dependency_shadow`
- `CppunitTest_sc_ucalc_compile_diff`
- `spreadsheetengine_computational_graph_tests`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

Replay remained exact:

- `workbooks=500`
- `formula_cells=50661`
- `parsed_formulas=50652`
- `cached_fallback_cells=0`
- `cached_fallback_rate=0`

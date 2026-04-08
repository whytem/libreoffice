# Computational Substrate Shared-Group Named-Range Member-Exit Implementation

Status: completed implementation note for bounded named-range-combined scalar member-exit admission

## What Changed

The runtime change is intentionally narrow.

In
[AuthorityPilotBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/AuthorityPilotBuilder.hxx),
the named-range boundary gate now admits `MemberExit` only for
`SetScalarValue`, and the authority graph-after projection now reuses the
canonical dependency-graph shadow builder instead of maintaining a
second broadcaster-count reconstruction path.

In
[ComputationalSubstrateAuthority.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateAuthority.hxx)
and
[ComputationalSubstrateMutationEntry.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateMutationEntry.hxx),
live IR verification now compares Calc-hosted IR built from the predicted
computational after-shadow against Calc-hosted IR from the live after-state.

## Proof Updates

The bounded scalar member-exit admit proof now lives in:

- [computational_substrate_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/computational_substrate_tests.cxx)
- [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)

The retained non-scalar reject proof also lives in
[ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx).

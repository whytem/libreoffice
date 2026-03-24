# Shared Cases

This directory holds parity datasets that should be runnable in both:

- the standalone package build rooted in `spread_engine_extract/`
- the LibreOffice-integrated `spreadsheetengine` path

Current usage:

- standalone coverage runs these files from the `spread_engine_extract/tests/standalone/`
  test binaries
- Calc-side coverage runs the spreadsheet-facing rows from
  `CppunitTest_sc_ucalc_shared_cases`

Some rows may temporarily remain standalone-only when they exercise
helper-layer contracts that do not yet map 1:1 onto spreadsheet-facing Calc
formulas. Those should either be rewritten as spreadsheet-facing parity cases
or covered by a dedicated Calc-side helper test as extraction continues.

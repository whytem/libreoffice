# Shared Cases

This directory holds parity datasets that should be runnable in both:

- the standalone package build rooted in `spreadsheet_engine/`
- the LibreOffice-integrated `spreadsheetengine` path

Current usage:

- standalone coverage runs these files from the `spreadsheet_engine/tests/unit/`
  test binaries
- Calc-side coverage runs the spreadsheet-facing rows from
  `CppunitTest_sc_ucalc_shared_cases`
- the routine maintenance gate is
  `./spreadsheet_engine/tools/run_maintenance_validation.sh`, which runs the
  standalone suite and the Calc shared-cases lane together
- locale-aware spreadsheet parsing parity currently lives in
  `locale_parsing_cases.tsv` and runs in both lanes

Some rows may temporarily remain standalone-only when they exercise
helper-layer contracts that do not yet map 1:1 onto spreadsheet-facing Calc
formulas. Those should either be rewritten as spreadsheet-facing parity cases
or covered by a dedicated Calc-side helper test as extraction continues.

When adding a new shared-case file, keep both sides in sync:

- add or extend the standalone runner coverage
- add or extend `CppunitTest_sc_ucalc_shared_cases` when the rows are
  spreadsheet-facing
- make sure the routine maintenance gate still exercises the new parity path

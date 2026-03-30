# Standalone Parity Test Checklist

Goal: keep the standalone `spreadsheet_engine/` test surface close enough to the
LibreOffice Calc engine tests that drift is caught early when the standalone
project evolves independently.

Priority backlog:

- [x] Add shared spreadsheet-facing logic cases that run in both standalone and
  Calc.
- [x] Add shared lookup/reference parity cases for `MATCH`, `XMATCH`, `LOOKUP`,
  `VLOOKUP`, `HLOOKUP`, `XLOOKUP`, `INDEX`, and `OFFSET`.
- [x] Add shared dynamic-array parity cases for `TAKE`, `DROP`, `TOCOL`,
  `TOROW`, `WRAPCOLS`, `WRAPROWS`, `HSTACK`, `VSTACK`, `CHOOSECOLS`,
  `CHOOSEROWS`, and `EXPAND`.
- [x] Add shared scalar math parity cases for the high-traffic mathematical
  functions already exposed in the standalone API.
- [x] Add shared financial parity cases for the core financial functions
  already exposed in the standalone API.
- [x] Add standalone semantic cases for query, filter, sort, and
  lookup-cache behavior beyond pure planner checks.
- [x] Add standalone scenario-table tests for shared-formula and formulacell
  lifecycle transitions.
- [x] Reassess the larger Calc-only workbook families
  (`statistical`, `database`, `information`, `array`) and promote the portions
  that are now represented in the standalone API into shared parity datasets.

Notes:

- Prefer shared TSV datasets under `tests/parity/` whenever the behavior is
  spreadsheet-facing and can be expressed in both standalone and Calc.
- Prefer standalone-only unit tests under `tests/unit/` for planner/state/helper
  logic that does not yet map 1:1 to spreadsheet formulas.
- Expand `sc/qa/unit/ucalc_shared_cases.cxx` together with the standalone TSV
  consumers so the routine maintenance lane keeps exercising both sides.
- The larger Calc workbook families were reviewed again after adding the new
  lookup/reference, array, math, and financial parity datasets. No additional
  spreadsheet-facing batches are currently actionable without exposing more of
  those families through the standalone public API.

# Standalone Layout Refactor Checklist

Goal: make `spreadsheet_engine/` read like a maintained standalone project
instead of an extraction staging area.

Completed items:

- move extraction-history documents under `docs/extraction-history/`
- add this architecture/checklist note under `docs/archive/`
- move LibreOffice validation tooling under `integration/libreoffice/`
- move the combined maintenance runner under `tools/`
- keep thin root gbuild wrappers so LibreOffice module discovery still works
- rename standalone unit tests from `tests/standalone/` to `tests/unit/`
- rename shared parity datasets from `tests/shared_cases/` to `tests/parity/`
- rename shim headers from `standalone/include/` to `shims/include/`
- split installed support headers into `runtime/`
- move non-installed planner/runtime helpers into `detail/`
- rename the extraction-era `Phase0` probe to `LibraryProbe`
- split the large CMake source manifest into smaller fragments under
  `cmake/sources/`
- replace the old deferred-source bucket with
  `SPREADSHEETENGINE_LIBREOFFICE_ONLY_SOURCES`
- keep the fast smoke lane, stable milestone lane, and broader engine lane as
  part of routine maintenance

Resulting shape:

- public API: `inc/spreadsheetengine/api/`
- public runtime support: `inc/spreadsheetengine/runtime/`
- internal-only implementation helpers: `inc/spreadsheetengine/detail/`
- LibreOffice adapters: `inc/spreadsheetengine/compat/libreoffice/`
- formula compatibility layer: `inc/spreadsheetengine/compat/formula/`
- standalone shim headers: `shims/include/`
- standalone unit tests: `tests/unit/`
- shared parity datasets: `tests/parity/`
- external-consumer smoke lane: `tests/consumer/`
- LibreOffice integration files: `integration/libreoffice/`
- maintenance helpers: `tools/`

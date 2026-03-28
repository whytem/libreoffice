# Standalone Spreadsheet Engine

`spreadsheet_engine/` now supports a real standalone CMake package flow in
addition to the LibreOffice-integrated build.

## Project Docs

- architecture and post-extraction layout notes live under `docs/architecture/`
- extraction-history planning and audit notes live under
  `docs/extraction-history/`

## Routine Validation

```bash
./spreadsheet_engine/tools/run_maintenance_validation.sh
```

That combined maintenance gate now runs:

- standalone configure/build/test, including the installed-package consumer
  smoke path
- Calc spreadsheet-engine validation through
  [run_spreadsheet_unit_tests.sh](/home/ubuntu/repos/libreoffice/spreadsheet_engine/integration/libreoffice/run_spreadsheet_unit_tests.sh)

Use `--engine` when you want the broader Calc engine profile instead of the
default smoke lane:

```bash
./spreadsheet_engine/tools/run_maintenance_validation.sh --engine
```

Use `--milestone` for the broader stable maintenance subset that sits between
the fast smoke gate and the full engine profile:

```bash
./spreadsheet_engine/tools/run_maintenance_validation.sh --milestone
```

Use `--compiler-diff` when you want the standalone maintenance lane to also run
the compiled-vs-legacy replay diff smoke. By default that targets the logical
FODS family for fast iteration:

```bash
./spreadsheet_engine/tools/run_maintenance_validation.sh --compiler-diff
```

You can point that diff smoke at a different replay family or workbook path
with `--compiler-diff-target`:

```bash
./spreadsheet_engine/tools/run_maintenance_validation.sh \
  --compiler-diff \
  --compiler-diff-target sc/qa/unit/data/functions/text/fods
```

## Standalone Build

```bash
cmake -S spreadsheet_engine -B /tmp/spreadsheetengine-build
cmake --build /tmp/spreadsheetengine-build
ctest --test-dir /tmp/spreadsheetengine-build --output-on-failure
```

## Install The Package

```bash
cmake --install /tmp/spreadsheetengine-build --prefix /tmp/spreadsheetengine-install
```

This installs the `spreadsheetengine::core` package target plus the current
public header surface, including the lightweight `InMemoryEvaluationHost`
runtime helper for standalone scenarios.

## Consumer Smoke Build

```bash
cmake -S spreadsheet_engine/tests/consumer -B /tmp/spreadsheetengine-consumer \
  -DCMAKE_PREFIX_PATH=/tmp/spreadsheetengine-install
cmake --build /tmp/spreadsheetengine-consumer
/tmp/spreadsheetengine-consumer/spreadsheetengine_consumer_smoke
```

The installed consumer smoke now exercises a small spreadsheet-style flow:
host-backed parsing, reference planning, cached lookup routing, and formatting
through `InMemoryEvaluationHost`.

## LibreOffice Validation

For extraction slices that do not touch Calc code, the standalone build and
consumer smoke flow are the main gate. When a slice changes shared sources or
host adapters, also run the relevant LibreOffice build or unit-test targets.
The routine way to do that is now the combined maintenance script above, which
keeps the standalone suite and the Calc shared-cases lane in the regular loop
together.

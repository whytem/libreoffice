# Standalone Spreadsheet Engine

`spread_engine_extract/` now supports a real standalone CMake package flow in
addition to the LibreOffice-integrated build.

## Routine Validation

```bash
./spread_engine_extract/run_maintenance_validation.sh
```

That combined maintenance gate now runs:

- standalone configure/build/test, including the installed-package consumer
  smoke path
- Calc spreadsheet-engine validation through
  [run_spreadsheet_unit_tests.sh](/home/ubuntu/repos/libreoffice/spread_engine_extract/run_spreadsheet_unit_tests.sh)

Use `--engine` when you want the broader Calc engine profile instead of the
default smoke lane:

```bash
./spread_engine_extract/run_maintenance_validation.sh --engine
```

## Standalone Build

```bash
cmake -S spread_engine_extract -B /tmp/spreadsheetengine-build
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
cmake -S spread_engine_extract/tests/consumer -B /tmp/spreadsheetengine-consumer \
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

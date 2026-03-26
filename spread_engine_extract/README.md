# Standalone Spreadsheet Engine

`spread_engine_extract/` now supports a real standalone CMake package flow in
addition to the LibreOffice-integrated build.

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
public header surface.

## Consumer Smoke Build

```bash
cmake -S spread_engine_extract/tests/consumer -B /tmp/spreadsheetengine-consumer \
  -DCMAKE_PREFIX_PATH=/tmp/spreadsheetengine-install
cmake --build /tmp/spreadsheetengine-consumer
/tmp/spreadsheetengine-consumer/spreadsheetengine_consumer_smoke
```

## LibreOffice Validation

For extraction slices that do not touch Calc code, the standalone build and
consumer smoke flow are the main gate. When a slice changes shared sources or
host adapters, also run the relevant LibreOffice build or unit-test targets.

# sal_* Type Dependency Audit (2026-03-31)

## Overview

The standalone `spreadsheet_engine` currently depends on LibreOffice's `sal/types.h` for
fixed-width integer type aliases and on `rtl/math.hxx` for numeric utility functions. In
standalone builds, these are provided by thin shim headers under `shims/include/` that map
directly to standard C++ types (`<cstdint>`). When built inside LibreOffice, the real
`sal/types.h` and `rtl/math.hxx` from the LO SDK are used instead.

This audit catalogues every point of coupling to understand the scope of work required to
eliminate it entirely, making the engine's public API expressible in pure ISO C++.

---

## 1. sal_* Type Mapping

The shim at `shims/include/sal/types.h` defines these aliases:

| sal type | Standard C++ equivalent |
|----------|------------------------|
| `sal_Bool` | `bool` |
| `sal_Unicode` | `char16_t` |
| `sal_Int8` | `std::int8_t` |
| `sal_uInt8` | `std::uint8_t` |
| `sal_Int16` | `std::int16_t` |
| `sal_uInt16` | `std::uint16_t` |
| `sal_Int32` | `std::int32_t` |
| `sal_uInt32` | `std::uint32_t` |
| `sal_Int64` | `std::int64_t` |
| `sal_uInt64` | `std::uint64_t` |

All mappings are direct `using` aliases with no semantic differences.

---

## 2. Aggregate Metrics

| Category | Occurrences | Files |
|----------|-------------|-------|
| `sal_Int32` | ~300 | 48 |
| `sal_uInt8` (enum bases) | ~60 | 30+ |
| `sal_Int16` | ~130 | 20 |
| `sal_uInt16` | ~110 | 15 |
| `sal_Int64` | ~30 | 6 |
| `sal_uInt32` | ~15 | 8 |
| `sal_Unicode` | ~25 | 7 |
| `sal_Int8` | ~2 | 2 |
| `rtl::math::` calls | ~205 | 23 |
| `rtl_math_RoundingMode` | ~10 | 5 |

---

## 3. Public API Surface (api/ headers)

These define the engine's external contract. sal_* types here propagate to every consumer.

### 3a. Core type aliases (Host.hxx)

```
using SheetId     = sal_Int32;    // → std::int32_t
using ColumnIndex = sal_Int32;    // → std::int32_t
using RowIndex    = sal_Int32;    // → std::int32_t
using FormatIndex = sal_uInt32;   // → std::uint32_t
```

These four aliases are the single most impactful use: they are referenced by virtually
every header and source file in the engine.

### 3b. Matrix dimension type (Matrix.hxx)

```
using MatrixSize = sal_Int32;     // → std::int32_t
```

### 3c. Enum base types

30+ enums across api/ use `: sal_uInt8` as their underlying type. Examples:

- `api::CellValueKind : sal_uInt8`
- `api::query::SearchType : sal_uInt8`
- `api::lookup::Operation : sal_uInt8`
- `api::FormulaResult::ValueType : sal_uInt8`

### 3d. Function signatures

sal_Int32, sal_Int16, sal_uInt32 appear as parameters/returns in:

| Header | Types Used | Usage |
|--------|-----------|-------|
| Calendar.hxx | sal_Int16 | Year, month, day parameters |
| Text.hxx | sal_Int32, sal_uInt32 | Code point counts, Unicode values |
| Logic.hxx | sal_Int16 | Weekday, week number parameters |
| Array.hxx | sal_Int32 | Row/column dimensions |
| Query.hxx | sal_Int32 | Aggregate function codes |
| SharedFormula.hxx | sal_Int32, sal_uInt8 | Row spans, token comparison |
| Lookup.hxx | sal_Int8, sal_uInt8, sal_Int16 | Comparison modes, match types |
| LookupCache.hxx | sal_uInt8, sal_Int32 | Cache query types |
| Reference.hxx | sal_uInt8, sal_Int32, sal_Int64 | Reference flags, linear index |

---

## 4. Runtime Module Headers (runtime/)

| Header | Types Used | Role |
|--------|-----------|------|
| MathStatistical.hxx | (none) | Clean -- uses only `double` and `bool` |
| MathFinancial.hxx | (none) | Clean -- uses only `double` and `bool` |
| MathTranscendental.hxx | (none) | Clean |
| MathBitwise.hxx | (none) | Clean |
| MathScalar.hxx | (none) | Clean |
| MathRounding.hxx | sal_Int16 | `roundToDecimals` nDecimals parameter |
| MathAggregate.hxx | sal_Int32 | Function codes, non-empty counts |
| MathFunctionRuntime.hxx | sal_Int32 | Decimal count, digit count parameters |
| DateTimeParts.hxx | sal_Int16 | Year, month, day parameters |
| DateTimeWeek.hxx | sal_Int16 | Weekday flag |
| DateTimeWorkday.hxx | sal_Int32 | Workday count |
| DateTimeParse.hxx | sal_Int32, sal_Int16 | Parse result fields |
| NumeralConversion.hxx | sal_Int32 | Converted values |
| TextScalar.hxx | sal_Int32, sal_uInt32 | Code point counts, Unicode values |
| TextCase.hxx | (none) | Clean |
| TextWidth.hxx | (none) | Clean |
| TextServices.hxx | sal_Int32 | Character position |
| TextFunctionRuntime.hxx | sal_Int32 | String positions, lengths |
| TextRuntimeSupport.hxx | sal_Int32 | Pattern match positions |
| QueryRuntime.hxx | sal_uInt8, sal_Int32 | Enum bases, text comparison result |
| FinancialRuntime.hxx | sal_Int32 | Basis, frequency parameters |
| LookupRuntime.hxx | (none) | Clean |
| ConversionRuntime.hxx | (none) | Clean |
| InMemoryHost.hxx | sal_Int32 | Casts in inline methods |

---

## 5. Detail / Internal Headers (detail/)

| Header | sal_* types | Role |
|--------|------------|------|
| TokenModel.hxx | sal_uInt16, sal_uInt8, sal_Int16, sal_Int32, sal_Unicode | Opcode values, param classes, token fields |
| WorkbookCompilerLowering.hxx | sal_Unicode, sal_Int64, sal_uInt16, sal_uInt32, sal_uInt8 | Formula parsing/compilation |
| FormulaCellState.hxx | sal_uInt8, sal_Int32 | Cell state tracking (36 occurrences) |
| WorkbookFacadeTypes.hxx | sal_Int32, sal_Int64, sal_uInt8 | Facade data types |
| DependencyTypes.hxx | sal_Int32, sal_uInt8 | Dependency graph types |
| OdfFormulaParser.hxx | sal_uInt8, sal_Int32 | Parser node types |
| BuiltinExternalNames.hxx | sal_uInt16, sal_Unicode | External name catalog |

---

## 6. Source Files (source/core/)

### 6a. Highest usage

| File | Occurrences | Primary sal_* types |
|------|-------------|---------------------|
| FormulaEvaluator.cxx | ~114 | sal_Int32, sal_Int16, sal_Int64 |
| FinancialRuntime.cxx | ~93 | sal_Int32, sal_uInt16, sal_Int16 |
| DateAlgorithms.hxx | ~72 | sal_Int16, sal_uInt16, sal_Int32 |
| DateTimeParts.cxx | ~59 | sal_uInt16, sal_Int16, sal_Int64 |
| DateTimeParse.cxx | ~46 | sal_Int16, sal_Int32 |
| NumeralConversion.cxx | ~36 | sal_uInt16, sal_Unicode, sal_Int32 |
| MathStatistical.cxx | ~35 | sal_Int32 |
| TextFunctionRuntime.cxx | ~24 | sal_Int32 |

### 6b. rtl::math:: dependency

16 source files include `<rtl/math.hxx>` and use these functions:

| Function | Occurrences | Purpose |
|----------|-------------|---------|
| `rtl::math::approxFloor` | ~40 | Floor with epsilon tolerance |
| `rtl::math::approxCeil` | ~5 | Ceiling with epsilon tolerance |
| `rtl::math::approxAdd` | ~20 | Kahan-style addition |
| `rtl::math::approxEqual` | ~15 | Floating-point near-equality |
| `rtl::math::round` | ~25 | Decimal-aware rounding |
| `rtl::math::isRepresentableInteger` | ~5 | Integer range check |
| `rtl::math::stringToDouble` | ~3 | Locale-aware string parse |
| `rtl_math_RoundingMode` enum | ~10 | Rounding mode selection |

### 6c. Other LO-specific dependencies

| Header | Files | Purpose |
|--------|-------|---------|
| `<o3tl/untaint.hxx>` | 1 (MathFinancial.cxx) | `o3tl::div_allow_zero` |
| `<sal/log.hxx>` | 1 (ForceCalculation.cxx) | `SAL_WARN` macro |
| `<kahan.hxx>` | 3 | Kahan summation class |
| `<basegfx/numeric/ftools.hxx>` | 0 (available but unused in core) | Degree/radian conversion |

---

## 7. Calc Integration Constraint

LibreOffice Calc (sc/) includes engine headers directly in 25+ source files and
3 public sc/inc/ headers. It uses engine types at the API boundary:

| Integration Pattern | Calc Files | Engine Types Consumed |
|--------------------|------------|---------------------|
| Math/statistics runtime calls | interpr1-8.cxx | `semath::AggregateScan` (has `sal_Int32` member), function params |
| Lookup/query API | lookupcache.cxx, queryevaluator.cxx | `lookupcache::CacheEntry`, `query::SearchType` |
| Shared formula API | sharedformula.cxx, token.cxx | `sharedformula::TokenCompareState`, `GroupRunAction` |
| Formula cell lifecycle | formulacell.cxx | `FormulaCellState`, `NotifyKind`, update plan types |
| Reference data | refdata.hxx (sc/inc/) | `refdata::SingleRefData`, `ComplexRefData` |
| Dependency tracking | document.cxx | `ScopedInvalidationShadow`, mutation translation |

**Any change to engine header types must compile unchanged against Calc code.**

### Type identity hazard: `sal_Int32` vs `std::int32_t`

In the real LibreOffice `sal/types.h`:
- On **64-bit Linux** (`sizeof(long)==8`): `sal_Int32` = `signed int`
- On **32-bit Linux** (`sizeof(long)==4`): `sal_Int32` = `signed long`

Meanwhile `std::int32_t` is typically `int` on both platforms (glibc/musl).

On 64-bit Linux (the primary platform), `sal_Int32` and `std::int32_t` are both
`int` -- the **same type**. On 32-bit Linux, they could differ (`long` vs `int`),
which would break template deduction and overload resolution.

**Consequence:** Naively replacing `sal_Int32` with `std::int32_t` in engine
headers would break Calc compilation on 32-bit Linux and any platform where
`sizeof(long)==4`.

---

## 8. Proposed Elimination Plan

The plan must satisfy two constraints simultaneously:
1. Engine headers express a pure-C++ API with no `<sal/types.h>` include
2. Calc code that passes `sal_Int32` values to engine functions continues to compile

### Strategy: Conditional type resolution

The engine defines its own integer aliases that resolve to `sal_*` types when
built inside LibreOffice, and to `std::int*_t` when built standalone.

### Phase 0: Create `api/Types.hxx` with build-aware aliases

```cpp
// spreadsheet_engine/inc/spreadsheetengine/api/Types.hxx
#pragma once

#ifdef SPREADSHEETENGINE_WITHIN_LIBREOFFICE
  // Inside LO: use sal_* so Calc code sees identical types
  #include <sal/types.h>
  namespace spreadsheetengine::api {
      using Int8    = sal_Int8;
      using UInt8   = sal_uInt8;
      using Int16   = sal_Int16;
      using UInt16  = sal_uInt16;
      using Int32   = sal_Int32;
      using UInt32  = sal_uInt32;
      using Int64   = sal_Int64;
      using UInt64  = sal_uInt64;
  }
#else
  // Standalone: pure C++, no LO dependency
  #include <cstdint>
  namespace spreadsheetengine::api {
      using Int8    = std::int8_t;
      using UInt8   = std::uint8_t;
      using Int16   = std::int16_t;
      using UInt16  = std::uint16_t;
      using Int32   = std::int32_t;
      using UInt32  = std::uint32_t;
      using Int64   = std::int64_t;
      using UInt64  = std::uint64_t;
  }
#endif
```

The LO `.mk` build would add `-DSPREADSHEETENGINE_WITHIN_LIBREOFFICE` alongside
the existing `-DSPREADSHEETENGINE_DLLIMPLEMENTATION`. The standalone CMake build
does not define it.

This guarantees **type identity** with Calc's `sal_Int32` in the LO build while
giving the standalone build pure C++ types.

### Phase 1: Migrate api/ and runtime/ headers (~35 headers, ~400 tokens)

Replace every `sal_*` token with the corresponding `api::` alias:

| From | To |
|------|----|
| `sal_Int32` | `api::Int32` |
| `sal_uInt8` | `api::UInt8` |
| `sal_Int16` | `api::Int16` |
| `sal_uInt16` | `api::UInt16` |
| `sal_uInt32` | `api::UInt32` |
| `sal_Int64` | `api::Int64` |
| `sal_Unicode` | `char16_t` (always identical) |
| `sal_Bool` | `bool` (always identical) |

**Priority order:**
1. `api/Host.hxx` -- change 4 core type aliases (`SheetId`, `ColumnIndex`,
   `RowIndex`, `FormatIndex`) to use `api::Int32` / `api::UInt32`
2. `api/Matrix.hxx` -- change `MatrixSize`
3. 30+ enum bases `: sal_uInt8` → `: api::UInt8`
4. `runtime/*.hxx` function signatures (~15 headers)
5. `api/*.hxx` function signatures (~10 headers)

Remove `#include <sal/types.h>` from each migrated header, replacing it with
`#include <spreadsheetengine/api/Types.hxx>`.

**Calc impact: None.** In the LO build, `api::Int32` == `sal_Int32` == `signed int`,
so every Calc call site that passes or receives a `sal_Int32` continues to compile
with exact type identity. No casts, no template deduction failures.

### Phase 2: Migrate detail/ headers (~15 files, ~100 tokens)

Same mechanical replacement for internal headers:

- `TokenModel.hxx` (OpCodeValue, ParamClassValue, ErrorCode aliases)
- `FormulaCellState.hxx` (36 occurrences)
- `WorkbookCompilerLowering.hxx` (23 occurrences)
- `WorkbookFacadeTypes.hxx` (15 occurrences)

`sal_Unicode` → `char16_t` is safe everywhere (the LO `sal/types.h` also defines
`sal_Unicode` as `char16_t` on all modern platforms).

### Phase 3: Migrate source files (~24 files, ~180 tokens)

Mechanical follow-on from header changes. Source files use the types from headers,
so once headers are migrated the sources just need matching replacements for any
remaining local uses (loop counters, local variables, casts).

### Phase 4: Replace rtl::math:: with engine-owned equivalents (~205 call sites)

**4a. Internalize the shim implementations:**
The `shims/include/rtl/math.hxx` already provides standalone implementations of
all used functions. Move these into a new engine-owned module:

```
spreadsheet_engine/inc/spreadsheetengine/runtime/FloatingPoint.hxx
spreadsheet_engine/source/core/FloatingPoint.cxx
```

Namespace: `spreadsheetengine::core::fp`

| rtl::math function | Engine replacement |
|--------------------|-------------------|
| `approxFloor(x)` | `fp::approxFloor(x)` |
| `approxCeil(x)` | `fp::approxCeil(x)` |
| `approxAdd(a, b)` | `fp::approxAdd(a, b)` |
| `approxEqual(a, b)` | `fp::approxEqual(a, b)` |
| `round(x, dec, mode)` | `fp::round(x, dec, mode)` |
| `isRepresentableInteger(x)` | `fp::isRepresentableInteger(x)` |
| `stringToDouble(...)` | `fp::stringToDouble(...)` |

**4b. Replace `rtl_math_RoundingMode` enum** with an engine-owned enum.
The `compat/libreoffice/Rounding.hxx` maps between the two.

**4c. Internalize `kahan.hxx`** -- move from shims into engine proper.

**4d. Internalize `o3tl/untaint.hxx`** -- 1 use in MathFinancial.cxx.

**Calc impact:** The compat layer provides wrappers that accept the engine's
fp:: types and internally call the engine functions. Calc code that currently
calls `rtl::math::approxFloor` directly does not go through the engine for those
calls -- only engine-internal code uses `fp::`.

### Phase 5: Remove shim dependency

Once phases 1-4 are complete:
1. Remove `#include <sal/types.h>` from all non-compat engine headers and sources
2. Remove `#include <rtl/math.hxx>` from all engine sources
3. The standalone build no longer needs `shims/include` in its include path
4. The shims directory is retained only for the standalone build's test harness
   (or removed entirely if tests also migrate to engine-owned types)

### Phase 6: Verify both build configurations

1. **Standalone (CMake):** Build and run all 26 test suites. The engine uses
   `std::int32_t` etc. throughout. No sal dependency.
2. **LibreOffice (make):** Full `make check` on sc module. The engine uses
   `sal_Int32` etc. via the conditional `api/Types.hxx`. Calc code is unchanged.

---

## 9. Risk Assessment

| Risk | Severity | Mitigation |
|------|----------|------------|
| Type mismatch on 32-bit platforms | **Eliminated** | Conditional typedef resolves to `sal_Int32` inside LO |
| Calc compilation breakage | **Eliminated** | `api::Int32` == `sal_Int32` in LO build; exact type identity |
| Behavioral change in fp:: replacements | Medium | Copy shim implementations verbatim; existing test coverage |
| Large mechanical diff obscures real changes | Low | One commit per phase; scripted replacement |
| Missing `SPREADSHEETENGINE_WITHIN_LIBREOFFICE` define | Low | Build system already uses it; verify in Library_*.mk |

## 10. Recommended Execution Order

Phases 0-3 (type aliases) are purely mechanical and can be done in a single pass
with scripted replacement + build verification. They carry near-zero risk because
the conditional typedef guarantees type identity in the LO build.

Phase 4 (rtl::math) requires more care because the floating-point utilities have
subtle epsilon-dependent behavior. The shim implementations are already tested and
can be adopted directly.

**Total estimated scope:**
- ~680 sal_* token replacements across ~50 files
- ~205 rtl::math call site migrations across ~23 files
- 0 changes required in sc/ (Calc) code

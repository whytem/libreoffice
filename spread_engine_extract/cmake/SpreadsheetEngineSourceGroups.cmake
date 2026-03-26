# Source-group manifests for the standalone build track.
#
# The goal is to make it explicit which extracted sources are already
# standalone-buildable and which are still tied to LibreOffice or Calc
# vocabulary. As extraction progresses, sources should move from the deferred
# list into the standalone-ready core list.

set(SPREADSHEETENGINE_STANDALONE_PUBLIC_API_HEADERS
    "${SPREADSHEETENGINE_ROOT}/inc/spreadsheetengine/api/Array.hxx"
    "${SPREADSHEETENGINE_ROOT}/inc/spreadsheetengine/api/Calendar.hxx"
    "${SPREADSHEETENGINE_ROOT}/inc/spreadsheetengine/api/Compiler.hxx"
    "${SPREADSHEETENGINE_ROOT}/inc/spreadsheetengine/api/Config.hxx"
    "${SPREADSHEETENGINE_ROOT}/inc/spreadsheetengine/api/Date.hxx"
    "${SPREADSHEETENGINE_ROOT}/inc/spreadsheetengine/api/Error.hxx"
    "${SPREADSHEETENGINE_ROOT}/inc/spreadsheetengine/api/FormulaResult.hxx"
    "${SPREADSHEETENGINE_ROOT}/inc/spreadsheetengine/api/Grammar.hxx"
    "${SPREADSHEETENGINE_ROOT}/inc/spreadsheetengine/api/Host.hxx"
    "${SPREADSHEETENGINE_ROOT}/inc/spreadsheetengine/api/Logic.hxx"
    "${SPREADSHEETENGINE_ROOT}/inc/spreadsheetengine/api/Lookup.hxx"
    "${SPREADSHEETENGINE_ROOT}/inc/spreadsheetengine/api/LookupCache.hxx"
    "${SPREADSHEETENGINE_ROOT}/inc/spreadsheetengine/api/Math.hxx"
    "${SPREADSHEETENGINE_ROOT}/inc/spreadsheetengine/api/Matrix.hxx"
    "${SPREADSHEETENGINE_ROOT}/inc/spreadsheetengine/api/Numeral.hxx"
    "${SPREADSHEETENGINE_ROOT}/inc/spreadsheetengine/api/Parsing.hxx"
    "${SPREADSHEETENGINE_ROOT}/inc/spreadsheetengine/api/Query.hxx"
    "${SPREADSHEETENGINE_ROOT}/inc/spreadsheetengine/api/Reference.hxx"
    "${SPREADSHEETENGINE_ROOT}/inc/spreadsheetengine/api/ReferenceData.hxx"
    "${SPREADSHEETENGINE_ROOT}/inc/spreadsheetengine/api/ReferenceUpdate.hxx"
    "${SPREADSHEETENGINE_ROOT}/inc/spreadsheetengine/api/Rounding.hxx"
    "${SPREADSHEETENGINE_ROOT}/inc/spreadsheetengine/api/SharedFormula.hxx"
    "${SPREADSHEETENGINE_ROOT}/inc/spreadsheetengine/api/String.hxx"
    "${SPREADSHEETENGINE_ROOT}/inc/spreadsheetengine/api/StringReference.hxx"
    "${SPREADSHEETENGINE_ROOT}/inc/spreadsheetengine/api/Text.hxx"
    "${SPREADSHEETENGINE_ROOT}/inc/spreadsheetengine/api/Workday.hxx"
)

set(SPREADSHEETENGINE_STANDALONE_PUBLIC_SUPPORT_HEADERS
    "${SPREADSHEETENGINE_ROOT}/inc/spreadsheetengine/spreadsheetenginedllapi.h"
    "${SPREADSHEETENGINE_ROOT}/inc/spreadsheetengine/core/DateTimeParts.hxx"
    "${SPREADSHEETENGINE_ROOT}/inc/spreadsheetengine/core/DateTimeWeek.hxx"
    "${SPREADSHEETENGINE_ROOT}/inc/spreadsheetengine/core/DateTimeWorkday.hxx"
    "${SPREADSHEETENGINE_ROOT}/inc/spreadsheetengine/core/MathBitwise.hxx"
    "${SPREADSHEETENGINE_ROOT}/inc/spreadsheetengine/core/MathFinancial.hxx"
    "${SPREADSHEETENGINE_ROOT}/inc/spreadsheetengine/core/MathRounding.hxx"
    "${SPREADSHEETENGINE_ROOT}/inc/spreadsheetengine/core/MathScalar.hxx"
    "${SPREADSHEETENGINE_ROOT}/inc/spreadsheetengine/core/MathTranscendental.hxx"
    "${SPREADSHEETENGINE_ROOT}/inc/spreadsheetengine/core/NumeralConversion.hxx"
    "${SPREADSHEETENGINE_ROOT}/inc/spreadsheetengine/core/TextCase.hxx"
    "${SPREADSHEETENGINE_ROOT}/inc/spreadsheetengine/core/TextScalar.hxx"
    "${SPREADSHEETENGINE_ROOT}/inc/spreadsheetengine/core/TextServices.hxx"
    "${SPREADSHEETENGINE_ROOT}/inc/spreadsheetengine/core/TextWidth.hxx"
)

set(SPREADSHEETENGINE_STANDALONE_PUBLIC_SHIM_HEADERS
    "${SPREADSHEETENGINE_ROOT}/standalone/include/rtl/math.hxx"
    "${SPREADSHEETENGINE_ROOT}/standalone/include/sal/types.h"
)

set(SPREADSHEETENGINE_STANDALONE_PUBLIC_HEADERS
    ${SPREADSHEETENGINE_STANDALONE_PUBLIC_API_HEADERS}
    ${SPREADSHEETENGINE_STANDALONE_PUBLIC_SUPPORT_HEADERS}
    ${SPREADSHEETENGINE_STANDALONE_PUBLIC_SHIM_HEADERS}
)

set(SPREADSHEETENGINE_STANDALONE_CORE_SOURCES
    "${SPREADSHEETENGINE_ROOT}/source/compat/formula/FormulaGrammar.cxx"
    "${SPREADSHEETENGINE_ROOT}/source/core/CalcConfig.cxx"
    "${SPREADSHEETENGINE_ROOT}/source/core/CompilerSupport.cxx"
    "${SPREADSHEETENGINE_ROOT}/source/core/Phase0.cxx"
    "${SPREADSHEETENGINE_ROOT}/source/core/DateTimeParts.cxx"
    "${SPREADSHEETENGINE_ROOT}/source/core/DateTimeWeek.cxx"
    "${SPREADSHEETENGINE_ROOT}/source/core/DateTimeWorkday.cxx"
    "${SPREADSHEETENGINE_ROOT}/source/core/ForceCalculation.cxx"
    "${SPREADSHEETENGINE_ROOT}/source/core/MathBitwise.cxx"
    "${SPREADSHEETENGINE_ROOT}/source/core/MathFinancial.cxx"
    "${SPREADSHEETENGINE_ROOT}/source/core/MathRounding.cxx"
    "${SPREADSHEETENGINE_ROOT}/source/core/MathScalar.cxx"
    "${SPREADSHEETENGINE_ROOT}/source/core/MathTranscendental.cxx"
    "${SPREADSHEETENGINE_ROOT}/source/core/NumeralConversion.cxx"
    "${SPREADSHEETENGINE_ROOT}/source/core/TextCase.cxx"
    "${SPREADSHEETENGINE_ROOT}/source/core/TextScalar.cxx"
    "${SPREADSHEETENGINE_ROOT}/source/core/TextWidth.cxx"
)

set(SPREADSHEETENGINE_STANDALONE_SMOKE_SOURCES
    "${SPREADSHEETENGINE_ROOT}/tests/standalone/smoke_main.cxx"
)

set(SPREADSHEETENGINE_STANDALONE_COMPILER_TEST_SOURCES
    "${SPREADSHEETENGINE_ROOT}/tests/standalone/compiler_api_tests.cxx"
)

set(SPREADSHEETENGINE_STANDALONE_MATRIX_TEST_SOURCES
    "${SPREADSHEETENGINE_ROOT}/tests/standalone/matrix_api_tests.cxx"
)

set(SPREADSHEETENGINE_STANDALONE_MATH_TEST_SOURCES
    "${SPREADSHEETENGINE_ROOT}/tests/standalone/math_api_tests.cxx"
)

set(SPREADSHEETENGINE_STANDALONE_CALENDAR_TEST_SOURCES
    "${SPREADSHEETENGINE_ROOT}/tests/standalone/calendar_api_tests.cxx"
)

set(SPREADSHEETENGINE_STANDALONE_TEXT_TEST_SOURCES
    "${SPREADSHEETENGINE_ROOT}/tests/standalone/text_api_tests.cxx"
)

set(SPREADSHEETENGINE_STANDALONE_CONFIG_TEST_SOURCES
    "${SPREADSHEETENGINE_ROOT}/tests/standalone/config_api_tests.cxx"
)

set(SPREADSHEETENGINE_STANDALONE_EXECUTION_TEST_SOURCES
    "${SPREADSHEETENGINE_ROOT}/tests/standalone/execution_api_tests.cxx"
)

set(SPREADSHEETENGINE_STANDALONE_HOST_TEST_SOURCES
    "${SPREADSHEETENGINE_ROOT}/tests/standalone/host_api_tests.cxx"
)

set(SPREADSHEETENGINE_STANDALONE_LOGIC_TEST_SOURCES
    "${SPREADSHEETENGINE_ROOT}/tests/standalone/logic_api_tests.cxx"
)

set(SPREADSHEETENGINE_STANDALONE_PARSING_TEST_SOURCES
    "${SPREADSHEETENGINE_ROOT}/tests/standalone/parsing_api_tests.cxx"
)

set(SPREADSHEETENGINE_STANDALONE_LOOKUP_TEST_SOURCES
    "${SPREADSHEETENGINE_ROOT}/tests/standalone/lookup_api_tests.cxx"
)

set(SPREADSHEETENGINE_STANDALONE_ARRAY_TEST_SOURCES
    "${SPREADSHEETENGINE_ROOT}/tests/standalone/array_api_tests.cxx"
)

set(SPREADSHEETENGINE_STANDALONE_REFERENCE_TEST_SOURCES
    "${SPREADSHEETENGINE_ROOT}/tests/standalone/reference_api_tests.cxx"
)

set(SPREADSHEETENGINE_STANDALONE_CARRIER_TEST_SOURCES
    "${SPREADSHEETENGINE_ROOT}/tests/standalone/carrier_api_tests.cxx"
)

set(SPREADSHEETENGINE_STANDALONE_QUERY_TEST_SOURCES
    "${SPREADSHEETENGINE_ROOT}/tests/standalone/query_api_tests.cxx"
)

set(SPREADSHEETENGINE_STANDALONE_SHAREDFORMULA_TEST_SOURCES
    "${SPREADSHEETENGINE_ROOT}/tests/standalone/sharedformula_api_tests.cxx"
)

set(SPREADSHEETENGINE_STANDALONE_FORMULACELL_TEST_SOURCES
    "${SPREADSHEETENGINE_ROOT}/tests/standalone/formulacell_api_tests.cxx"
)

set(SPREADSHEETENGINE_STANDALONE_DEFERRED_SOURCES
    "${SPREADSHEETENGINE_ROOT}/source/core/MatrixOperators.cxx"
)

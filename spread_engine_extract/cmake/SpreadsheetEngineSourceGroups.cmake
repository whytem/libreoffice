# Source-group manifests for the standalone build track.
#
# The goal is to make it explicit which extracted sources are already
# standalone-buildable and which are still tied to LibreOffice or Calc
# vocabulary. As extraction progresses, sources should move from the deferred
# list into the standalone-ready core list.

set(SPREADSHEETENGINE_STANDALONE_CORE_SOURCES
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

set(SPREADSHEETENGINE_STANDALONE_DEFERRED_SOURCES
    "${SPREADSHEETENGINE_ROOT}/source/compat/formula/FormulaGrammar.cxx"
    "${SPREADSHEETENGINE_ROOT}/source/core/CalcConfig.cxx"
    "${SPREADSHEETENGINE_ROOT}/source/core/MatrixOperators.cxx"
)

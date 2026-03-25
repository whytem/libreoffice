cmake_minimum_required(VERSION 3.20)

project(spreadsheetengine_standalone LANGUAGES CXX)

include(CTest)
find_package(ICU REQUIRED COMPONENTS uc)

set(SPREADSHEETENGINE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/SpreadsheetEngineSourceGroups.cmake")

add_library(spreadsheetengine_core STATIC
    ${SPREADSHEETENGINE_STANDALONE_CORE_SOURCES}
)

add_library(spreadsheetengine::core ALIAS spreadsheetengine_core)

target_compile_features(spreadsheetengine_core PUBLIC cxx_std_20)
target_compile_definitions(spreadsheetengine_core PRIVATE SPREADSHEETENGINE_DLLIMPLEMENTATION)
target_include_directories(spreadsheetengine_core
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/inc"
        "${CMAKE_CURRENT_SOURCE_DIR}/standalone/include"
)
target_link_libraries(spreadsheetengine_core PUBLIC ICU::uc)

add_executable(spreadsheetengine_smoke
    ${SPREADSHEETENGINE_STANDALONE_SMOKE_SOURCES}
)

target_compile_features(spreadsheetengine_smoke PRIVATE cxx_std_20)
target_compile_definitions(spreadsheetengine_smoke
    PRIVATE SPREADSHEETENGINE_TEST_ROOT="${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(spreadsheetengine_smoke PRIVATE spreadsheetengine::core)

function(add_spreadsheetengine_test target_name)
    add_executable(${target_name} ${ARGN})
    target_compile_features(${target_name} PRIVATE cxx_std_20)
    target_compile_definitions(${target_name}
        PRIVATE SPREADSHEETENGINE_TEST_ROOT="${CMAKE_CURRENT_SOURCE_DIR}"
    )
    target_link_libraries(${target_name} PRIVATE spreadsheetengine::core)
    if(BUILD_TESTING)
        add_test(NAME ${target_name} COMMAND ${target_name})
    endif()
endfunction()

add_spreadsheetengine_test(spreadsheetengine_compiler_tests
    ${SPREADSHEETENGINE_STANDALONE_COMPILER_TEST_SOURCES}
)

add_spreadsheetengine_test(spreadsheetengine_matrix_tests
    ${SPREADSHEETENGINE_STANDALONE_MATRIX_TEST_SOURCES}
)

add_spreadsheetengine_test(spreadsheetengine_math_tests
    ${SPREADSHEETENGINE_STANDALONE_MATH_TEST_SOURCES}
)

add_spreadsheetengine_test(spreadsheetengine_calendar_tests
    ${SPREADSHEETENGINE_STANDALONE_CALENDAR_TEST_SOURCES}
)

add_spreadsheetengine_test(spreadsheetengine_text_tests
    ${SPREADSHEETENGINE_STANDALONE_TEXT_TEST_SOURCES}
)

add_spreadsheetengine_test(spreadsheetengine_config_tests
    ${SPREADSHEETENGINE_STANDALONE_CONFIG_TEST_SOURCES}
)

add_spreadsheetengine_test(spreadsheetengine_execution_tests
    ${SPREADSHEETENGINE_STANDALONE_EXECUTION_TEST_SOURCES}
)

add_spreadsheetengine_test(spreadsheetengine_host_tests
    ${SPREADSHEETENGINE_STANDALONE_HOST_TEST_SOURCES}
)

add_spreadsheetengine_test(spreadsheetengine_logic_tests
    ${SPREADSHEETENGINE_STANDALONE_LOGIC_TEST_SOURCES}
)

add_spreadsheetengine_test(spreadsheetengine_parsing_tests
    ${SPREADSHEETENGINE_STANDALONE_PARSING_TEST_SOURCES}
)

add_spreadsheetengine_test(spreadsheetengine_lookup_tests
    ${SPREADSHEETENGINE_STANDALONE_LOOKUP_TEST_SOURCES}
)

add_spreadsheetengine_test(spreadsheetengine_array_tests
    ${SPREADSHEETENGINE_STANDALONE_ARRAY_TEST_SOURCES}
)

add_spreadsheetengine_test(spreadsheetengine_reference_tests
    ${SPREADSHEETENGINE_STANDALONE_REFERENCE_TEST_SOURCES}
)

add_spreadsheetengine_test(spreadsheetengine_carrier_tests
    ${SPREADSHEETENGINE_STANDALONE_CARRIER_TEST_SOURCES}
)

if(BUILD_TESTING)
    add_test(NAME spreadsheetengine_smoke COMMAND spreadsheetengine_smoke)
endif()

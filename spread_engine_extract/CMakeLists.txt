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
target_link_libraries(spreadsheetengine_smoke PRIVATE spreadsheetengine::core)

add_executable(spreadsheetengine_tests
    ${SPREADSHEETENGINE_STANDALONE_TEST_SOURCES}
)

target_compile_features(spreadsheetengine_tests PRIVATE cxx_std_20)
target_link_libraries(spreadsheetengine_tests PRIVATE spreadsheetengine::core)

if(BUILD_TESTING)
    add_test(NAME spreadsheetengine_smoke COMMAND spreadsheetengine_smoke)
    add_test(NAME spreadsheetengine_tests COMMAND spreadsheetengine_tests)
endif()

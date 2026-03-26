cmake_minimum_required(VERSION 3.20)

project(spreadsheetengine_standalone VERSION 0.1.0 LANGUAGES CXX)

include(CTest)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
find_package(ICU REQUIRED COMPONENTS uc)

set(SPREADSHEETENGINE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(SPREADSHEETENGINE_PACKAGE_NAME "spreadsheetengine")
set(SPREADSHEETENGINE_INSTALL_CMAKEDIR
    "${CMAKE_INSTALL_LIBDIR}/cmake/${SPREADSHEETENGINE_PACKAGE_NAME}"
)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/SpreadsheetEngineSourceGroups.cmake")

add_library(spreadsheetengine_core STATIC
    ${SPREADSHEETENGINE_STANDALONE_CORE_SOURCES}
)

add_library(spreadsheetengine::core ALIAS spreadsheetengine_core)

target_compile_features(spreadsheetengine_core PUBLIC cxx_std_20)
target_compile_definitions(spreadsheetengine_core PRIVATE SPREADSHEETENGINE_DLLIMPLEMENTATION)
set_target_properties(spreadsheetengine_core PROPERTIES EXPORT_NAME core)
target_include_directories(spreadsheetengine_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/shims/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
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

function(install_spreadsheetengine_headers base_dir)
    foreach(header IN LISTS ARGN)
        file(RELATIVE_PATH header_rel "${base_dir}" "${header}")
        get_filename_component(header_dest "${header_rel}" DIRECTORY)
        install(FILES "${header}" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${header_dest}")
    endforeach()
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

add_spreadsheetengine_test(spreadsheetengine_query_tests
    ${SPREADSHEETENGINE_STANDALONE_QUERY_TEST_SOURCES}
)

add_spreadsheetengine_test(spreadsheetengine_sharedformula_tests
    ${SPREADSHEETENGINE_STANDALONE_SHAREDFORMULA_TEST_SOURCES}
)

add_spreadsheetengine_test(spreadsheetengine_formulacell_tests
    ${SPREADSHEETENGINE_STANDALONE_FORMULACELL_TEST_SOURCES}
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/spreadsheetengine-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${SPREADSHEETENGINE_PACKAGE_NAME}-config.cmake"
    INSTALL_DESTINATION "${SPREADSHEETENGINE_INSTALL_CMAKEDIR}"
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${SPREADSHEETENGINE_PACKAGE_NAME}-config-version.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMajorVersion
)

install(TARGETS spreadsheetengine_core
    EXPORT spreadsheetengineTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

export(
    EXPORT spreadsheetengineTargets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/${SPREADSHEETENGINE_PACKAGE_NAME}Targets.cmake"
    NAMESPACE spreadsheetengine::
)

install_spreadsheetengine_headers(
    "${CMAKE_CURRENT_SOURCE_DIR}/inc"
    ${SPREADSHEETENGINE_STANDALONE_PUBLIC_API_HEADERS}
    ${SPREADSHEETENGINE_STANDALONE_PUBLIC_RUNTIME_HEADERS}
)

install_spreadsheetengine_headers(
    "${CMAKE_CURRENT_SOURCE_DIR}/shims/include"
    ${SPREADSHEETENGINE_STANDALONE_PUBLIC_SHIM_HEADERS}
)

install(
    EXPORT spreadsheetengineTargets
    FILE "${SPREADSHEETENGINE_PACKAGE_NAME}Targets.cmake"
    NAMESPACE spreadsheetengine::
    DESTINATION "${SPREADSHEETENGINE_INSTALL_CMAKEDIR}"
)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/${SPREADSHEETENGINE_PACKAGE_NAME}-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/${SPREADSHEETENGINE_PACKAGE_NAME}-config-version.cmake"
    DESTINATION "${SPREADSHEETENGINE_INSTALL_CMAKEDIR}"
)

if(BUILD_TESTING)
    add_test(NAME spreadsheetengine_smoke COMMAND spreadsheetengine_smoke)

    set(SPREADSHEETENGINE_PACKAGE_TEST_PREFIX
        "${CMAKE_CURRENT_BINARY_DIR}/package-install"
    )
    set(SPREADSHEETENGINE_PACKAGE_TEST_BUILD_DIR
        "${CMAKE_CURRENT_BINARY_DIR}/package-consumer-build"
    )
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RunInstalledConsumerSmoke.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/RunInstalledConsumerSmoke.cmake"
        @ONLY
    )
    add_test(
        NAME spreadsheetengine_installed_package_smoke
        COMMAND "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/RunInstalledConsumerSmoke.cmake"
    )
endif()

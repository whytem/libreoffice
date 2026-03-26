#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
REPO_ROOT=$(cd -- "${SCRIPT_DIR}/.." && pwd)
STANDALONE_SOURCE_DIR="${REPO_ROOT}/spread_engine_extract"

RUN_STANDALONE=1
RUN_CALC=1
CONFIGURE_STANDALONE=1
CALC_PROFILE=smoke
STANDALONE_BUILD_DIR=${STANDALONE_BUILD_DIR:-/tmp/spreadsheetengine-standalone-build}

declare -a CALC_ARGS=()

usage() {
    cat <<'EOF'
Usage: spread_engine_extract/run_maintenance_validation.sh [options] [-- calc-make-target ...]

Runs the routine spreadsheet-engine maintenance gate:
1. standalone configure/build/test
2. Calc spreadsheet-engine validation

Examples:
  spread_engine_extract/run_maintenance_validation.sh
  spread_engine_extract/run_maintenance_validation.sh --engine
  spread_engine_extract/run_maintenance_validation.sh --calc-only --engine
  spread_engine_extract/run_maintenance_validation.sh --build-dir /tmp/se-build
  spread_engine_extract/run_maintenance_validation.sh -- -- CppunitTest_sc_ucalc

Options:
  --standalone-only   Run only the standalone configure/build/test lane
  --calc-only         Run only the Calc validation lane
  --no-configure      Skip the standalone cmake configure step
  --build-dir DIR     Override the standalone build directory
  --smoke             Run the Calc smoke profile (default)
  --engine            Run the broader Calc engine profile
  --profile NAME      Forward a named Calc profile to run_spreadsheet_unit_tests.sh
  --help, -h          Show this help text

Environment:
  STANDALONE_BUILD_DIR  Default standalone build directory
  CMAKE_BIN             Override the cmake binary (default: cmake)
  CTEST_BIN             Override the ctest binary (default: ctest)
  MAKE_BIN              Forwarded to Calc validation
  JOBS                  Used for both cmake --build and Calc make -j
EOF
}

while (($# > 0)); do
    case "$1" in
        --help|-h)
            usage
            exit 0
            ;;
        --standalone-only)
            RUN_STANDALONE=1
            RUN_CALC=0
            ;;
        --calc-only)
            RUN_STANDALONE=0
            RUN_CALC=1
            ;;
        --no-configure)
            CONFIGURE_STANDALONE=0
            ;;
        --build-dir)
            shift
            if (($# == 0)); then
                printf '%s\n' 'Missing directory after --build-dir.' >&2
                exit 2
            fi
            STANDALONE_BUILD_DIR=$1
            ;;
        --smoke)
            CALC_PROFILE=smoke
            ;;
        --engine|--full-engine)
            CALC_PROFILE=engine
            ;;
        --profile)
            shift
            if (($# == 0)); then
                printf '%s\n' 'Missing profile name after --profile.' >&2
                exit 2
            fi
            CALC_PROFILE=$1
            ;;
        --)
            shift
            if (($# > 0)); then
                CALC_ARGS=("$@")
            fi
            break
            ;;
        -*)
            printf 'Unknown option: %s\n' "$1" >&2
            exit 2
            ;;
        *)
            CALC_ARGS+=("$1")
            ;;
    esac
    shift
done

CMAKE_BIN=${CMAKE_BIN:-cmake}
CTEST_BIN=${CTEST_BIN:-ctest}

if ((RUN_STANDALONE == 1)); then
    printf '\n==> Standalone configure/build/test\n'
    printf 'Build dir: %s\n' "${STANDALONE_BUILD_DIR}"

    if ((CONFIGURE_STANDALONE == 1)); then
        "${CMAKE_BIN}" -S "${STANDALONE_SOURCE_DIR}" -B "${STANDALONE_BUILD_DIR}"
    fi

    BUILD_CMD=("${CMAKE_BIN}" --build "${STANDALONE_BUILD_DIR}")
    if [[ -n ${JOBS:-} ]]; then
        BUILD_CMD+=(--parallel "${JOBS}")
    fi
    "${BUILD_CMD[@]}"

    "${CTEST_BIN}" --test-dir "${STANDALONE_BUILD_DIR}" --output-on-failure
fi

if ((RUN_CALC == 1)); then
    printf '\n==> Calc validation\n'
    if ((${#CALC_ARGS[@]} > 0)); then
        "${SCRIPT_DIR}/run_spreadsheet_unit_tests.sh" -- "${CALC_ARGS[@]}"
    else
        "${SCRIPT_DIR}/run_spreadsheet_unit_tests.sh" --profile "${CALC_PROFILE}"
    fi
fi

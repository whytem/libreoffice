#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
PACKAGE_ROOT=$(cd -- "${SCRIPT_DIR}/.." && pwd)
REPO_ROOT=$(cd -- "${PACKAGE_ROOT}/.." && pwd)
STANDALONE_SOURCE_DIR="${PACKAGE_ROOT}"

RUN_STANDALONE=1
RUN_CALC=1
CONFIGURE_STANDALONE=1
CALC_PROFILE=smoke
STANDALONE_BUILD_DIR=${STANDALONE_BUILD_DIR:-/tmp/spreadsheet_engine-standalone-build}
RUN_COMPILER_DIFF=0
COMPILER_DIFF_TARGET=${COMPILER_DIFF_TARGET:-sc/qa/unit/data/functions/logical/fods}

declare -a CALC_ARGS=()

usage() {
    cat <<'EOF'
Usage: spreadsheet_engine/tools/run_maintenance_validation.sh [options] [-- calc-make-target ...]

Runs the routine spreadsheet-engine maintenance gate:
1. standalone configure/build/test
2. Calc spreadsheet-engine validation

Examples:
  spreadsheet_engine/tools/run_maintenance_validation.sh
  spreadsheet_engine/tools/run_maintenance_validation.sh --milestone
  spreadsheet_engine/tools/run_maintenance_validation.sh --engine
  spreadsheet_engine/tools/run_maintenance_validation.sh --calc-only --engine
  spreadsheet_engine/tools/run_maintenance_validation.sh --compiler-diff
  spreadsheet_engine/tools/run_maintenance_validation.sh --compiler-diff-target sc/qa/unit/data/functions/text/fods
  spreadsheet_engine/tools/run_maintenance_validation.sh --build-dir /tmp/se-build
  spreadsheet_engine/tools/run_maintenance_validation.sh -- -- CppunitTest_sc_ucalc

Options:
  --standalone-only   Run only the standalone configure/build/test lane
  --calc-only         Run only the Calc validation lane
  --no-configure      Skip the standalone cmake configure step
  --build-dir DIR     Override the standalone build directory
  --compiler-diff     Run the standalone compiled-vs-legacy replay diff smoke
  --compiler-diff-target PATH
                     Override the replay path for --compiler-diff
  --smoke             Run the Calc smoke profile (default)
  --milestone         Run the broader stable Calc milestone profile
  --engine            Run the broader Calc engine profile
  --profile NAME      Forward a named Calc profile to run_spreadsheet_unit_tests.sh
  --help, -h          Show this help text

Environment:
  STANDALONE_BUILD_DIR  Default standalone build directory
  CMAKE_BIN             Override the cmake binary (default: cmake)
  CTEST_BIN             Override the ctest binary (default: ctest)
  COMPILER_DIFF_TARGET  Default replay target for --compiler-diff
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
        --compiler-diff)
            RUN_COMPILER_DIFF=1
            ;;
        --compiler-diff-target)
            shift
            if (($# == 0)); then
                printf '%s\n' 'Missing path after --compiler-diff-target.' >&2
                exit 2
            fi
            COMPILER_DIFF_TARGET=$1
            ;;
        --smoke)
            CALC_PROFILE=smoke
            ;;
        --milestone)
            CALC_PROFILE=milestone
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

resolve_diff_target() {
    if [[ ${COMPILER_DIFF_TARGET} = /* ]]; then
        printf '%s\n' "${COMPILER_DIFF_TARGET}"
    else
        printf '%s/%s\n' "${REPO_ROOT}" "${COMPILER_DIFF_TARGET}"
    fi
}

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

    if ((RUN_COMPILER_DIFF == 1)); then
        printf '\n==> Standalone compiled replay diff\n'
        local_target=$(resolve_diff_target)
        printf 'Replay target: %s\n' "${local_target}"
        "${STANDALONE_BUILD_DIR}/spreadsheetengine_fods_replay_tests" --compiled-diff "${local_target}"
    fi
fi

if ((RUN_CALC == 1)); then
    printf '\n==> Calc validation\n'
    if ((${#CALC_ARGS[@]} > 0)); then
        "${PACKAGE_ROOT}/integration/libreoffice/run_spreadsheet_unit_tests.sh" -- "${CALC_ARGS[@]}"
    else
        "${PACKAGE_ROOT}/integration/libreoffice/run_spreadsheet_unit_tests.sh" --profile "${CALC_PROFILE}"
    fi
fi

#!/usr/bin/env bash

set -uo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
PACKAGE_ROOT=$(cd -- "${SCRIPT_DIR}/../.." && pwd)
REPO_ROOT=$(cd -- "${PACKAGE_ROOT}/.." && pwd)

DEFAULT_TARGETS=(
    CppunitTest_sc_ucalc
    CppunitTest_sc_ucalc_token_bridge
    CppunitTest_sc_ucalc_compile_host
    CppunitTest_sc_ucalc_shadow_compiler
    CppunitTest_sc_ucalc_compile_diff
    CppunitTest_sc_ucalc_dependency_shadow
    CppunitTest_sc_ucalc_formula2
    CppunitTest_sc_ucalc_shared_cases
    CppunitTest_sc_ucalc_sharedformula
    CppunitTest_sc_spreadsheet_functions_test
)

MILESTONE_TARGETS=(
    CppunitTest_sc_ucalc
    CppunitTest_sc_ucalc_token_bridge
    CppunitTest_sc_ucalc_compile_host
    CppunitTest_sc_ucalc_shadow_compiler
    CppunitTest_sc_ucalc_compile_diff
    CppunitTest_sc_ucalc_dependency_shadow
    CppunitTest_sc_ucalc_formula2
    CppunitTest_sc_ucalc_shared_cases
    CppunitTest_sc_ucalc_sharedformula
    CppunitTest_sc_ucalc_sort
    CppunitTest_sc_cache_test
    CppunitTest_sc_datetime_functions_test
    CppunitTest_sc_text_functions_test
    CppunitTest_sc_spreadsheet_functions_test
)

ENGINE_TARGETS=(
    CppunitTest_sc_ucalc
    CppunitTest_sc_ucalc_token_bridge
    CppunitTest_sc_ucalc_compile_host
    CppunitTest_sc_ucalc_shadow_compiler
    CppunitTest_sc_ucalc_compile_diff
    CppunitTest_sc_ucalc_dependency_shadow
    CppunitTest_sc_ucalc_nanpayload
    CppunitTest_sc_ucalc_condformat
    CppunitTest_sc_ucalc_copypaste
    CppunitTest_sc_ucalc_datatransformation
    CppunitTest_sc_ucalc_document_themes
    CppunitTest_sc_ucalc_formula
    CppunitTest_sc_ucalc_formula2
    CppunitTest_sc_ucalc_parallelism
    CppunitTest_sc_ucalc_pivottable
    CppunitTest_sc_ucalc_rangelst
    CppunitTest_sc_ucalc_range
    CppunitTest_sc_ucalc_shared_cases
    CppunitTest_sc_ucalc_sharedformula
    CppunitTest_sc_ucalc_sparkline
    CppunitTest_sc_ucalc_solver
    CppunitTest_sc_ucalc_sort
    CppunitTest_sc_mark_test
    CppunitTest_sc_core
    CppunitTest_sc_cache_test
    CppunitTest_sc_parallelism
    CppunitTest_sc_array_functions_test
    CppunitTest_sc_information_functions_test
    CppunitTest_sc_logical_functions_test
    CppunitTest_sc_mathematical_functions_test
    CppunitTest_sc_spreadsheet_functions_test
    CppunitTest_sc_text_functions_test
)

usage() {
    cat <<'EOF'
Usage: spreadsheet_engine/integration/libreoffice/run_spreadsheet_unit_tests.sh [options] [make-target ...]

Runs a Calc spreadsheet-engine test profile when no explicit make targets are
provided. Pass explicit make targets to override the profile list.

Examples:
  spreadsheet_engine/integration/libreoffice/run_spreadsheet_unit_tests.sh
  spreadsheet_engine/integration/libreoffice/run_spreadsheet_unit_tests.sh --milestone
  spreadsheet_engine/integration/libreoffice/run_spreadsheet_unit_tests.sh --engine
  spreadsheet_engine/integration/libreoffice/run_spreadsheet_unit_tests.sh CppunitTest_sc_ucalc

Options:
  --smoke         Run the default fast validation subset
  --milestone     Run the broader stable milestone subset
  --engine        Run a broader non-rendering Calc engine suite
  --profile NAME  Select 'smoke', 'milestone', or 'engine'
  --help, -h      Show this help text

Environment:
  MAKE_BIN   Override the make binary (default: make)
  JOBS       If set, passes -j<JOBS> to make
EOF
}

PROFILE=smoke
declare -a EXPLICIT_TARGETS=()

while (($# > 0)); do
    case "$1" in
        --help|-h)
            usage
            exit 0
            ;;
        --smoke)
            PROFILE=smoke
            ;;
        --milestone)
            PROFILE=milestone
            ;;
        --engine|--full-engine)
            PROFILE=engine
            ;;
        --profile)
            shift
            if (($# == 0)); then
                printf '%s\n' 'Missing profile name after --profile.' >&2
                exit 2
            fi
            PROFILE=$1
            ;;
        --)
            shift
            if (($# > 0)); then
                EXPLICIT_TARGETS+=("$@")
            fi
            break
            ;;
        -*)
            printf 'Unknown option: %s\n' "$1" >&2
            exit 2
            ;;
        *)
            EXPLICIT_TARGETS+=("$1")
            ;;
    esac
    shift
done

if ((${#EXPLICIT_TARGETS[@]} > 0)); then
    TARGETS=("${EXPLICIT_TARGETS[@]}")
else
        case "${PROFILE}" in
        smoke)
            TARGETS=("${DEFAULT_TARGETS[@]}")
            ;;
        milestone)
            TARGETS=("${MILESTONE_TARGETS[@]}")
            ;;
        engine)
            TARGETS=("${ENGINE_TARGETS[@]}")
            ;;
        *)
            printf 'Unknown profile: %s\n' "${PROFILE}" >&2
            exit 2
            ;;
    esac
fi

MAKE_BIN=${MAKE_BIN:-make}
MAKE_CMD=("${MAKE_BIN}")
if [[ -n ${JOBS:-} ]]; then
    MAKE_CMD+=("-j${JOBS}")
fi

cd "${REPO_ROOT}"

if ((${#EXPLICIT_TARGETS[@]} > 0)); then
    printf 'Using explicit target list.\n'
else
    printf 'Using profile: %s\n' "${PROFILE}"
fi

declare -a TARGET_RESULTS=()
declare -a TARGET_CASE_RESULTS=()
declare -a TARGET_LOGS=()

passed_targets=0
failed_targets=0
exact_case_summary=1
known_passed_cases=0
known_total_cases=0

run_case_count() {
    local log_file=$1
    grep -Ec '^\[_RUN_____\]' "${log_file}" || true
}

spreadsheet_case_counts() {
    local log_file=$1
    local total passed failed

    total=$(grep -Ec '^Tested load .*: (Pass|FAIL) ' "${log_file}" || true)
    passed=$(grep -Ec '^Tested load .*: Pass ' "${log_file}" || true)
    failed=$((total - passed))

    printf '%s %s %s\n' "${total}" "${passed}" "${failed}"
}

printf 'Running %d target(s) from %s\n' "${#TARGETS[@]}" "${REPO_ROOT}"

for target in "${TARGETS[@]}"; do
    log_stem=${target#CppunitTest_}
    log_file="${REPO_ROOT}/workdir/CppunitTest/${log_stem}.test.log"

    printf '\n==> %s\n' "${target}"

    start_time=$(date +%s)
    "${MAKE_CMD[@]}" "${target}"
    target_rc=$?
    end_time=$(date +%s)
    elapsed=$((end_time - start_time))

    if ((target_rc == 0)); then
        status="PASS"
        passed_targets=$((passed_targets + 1))
    else
        status="FAIL"
        failed_targets=$((failed_targets + 1))
    fi

    case_summary="case summary unavailable"
    if [[ -f "${log_file}" ]]; then
        if grep -Eq '^Tested load .*: (Pass|FAIL) ' "${log_file}"; then
            read -r total_cases passed_cases failed_cases < <(spreadsheet_case_counts "${log_file}")
            known_total_cases=$((known_total_cases + total_cases))
            known_passed_cases=$((known_passed_cases + passed_cases))
            case_summary="${passed_cases}/${total_cases} dataset checks passed"
            if ((failed_cases > 0)); then
                case_summary="${case_summary}, ${failed_cases} failed"
            fi
        else
            total_cases=$(run_case_count "${log_file}")
            if ((target_rc == 0)); then
                known_total_cases=$((known_total_cases + total_cases))
                known_passed_cases=$((known_passed_cases + total_cases))
                case_summary="${total_cases}/${total_cases} cppunit cases passed"
            else
                exact_case_summary=0
                case_summary="${total_cases} cppunit cases started before failure"
            fi
        fi
    else
        exact_case_summary=0
    fi

    TARGET_RESULTS+=("$(printf '%-46s %4s  %4ss' "${target}" "${status}" "${elapsed}")")
    TARGET_CASE_RESULTS+=("${case_summary}")
    TARGET_LOGS+=("${log_file}")
done

printf '\nSummary\n'
printf '%s\n' '-------'
for i in "${!TARGET_RESULTS[@]}"; do
    printf '%s  %s\n' "${TARGET_RESULTS[i]}" "${TARGET_CASE_RESULTS[i]}"
    printf '  log: %s\n' "${TARGET_LOGS[i]}"
done

total_targets=${#TARGETS[@]}
target_pass_rate=$(awk "BEGIN { printf \"%.1f\", (${passed_targets} / ${total_targets}) * 100 }")

printf '\nTarget pass rate: %d/%d (%s%%)\n' "${passed_targets}" "${total_targets}" "${target_pass_rate}"

if ((known_total_cases > 0)); then
    case_pass_rate=$(awk "BEGIN { printf \"%.1f\", (${known_passed_cases} / ${known_total_cases}) * 100 }")
    if ((exact_case_summary == 1)); then
        printf 'Case pass rate: %d/%d (%s%%)\n' "${known_passed_cases}" "${known_total_cases}" "${case_pass_rate}"
    else
        printf 'Known case pass rate: %d/%d (%s%%)\n' "${known_passed_cases}" "${known_total_cases}" "${case_pass_rate}"
        printf '%s\n' 'Case totals are partial because at least one failing cppunit target does not expose an exact per-case fail count in its log.'
    fi
fi

if ((failed_targets > 0)); then
    exit 1
fi

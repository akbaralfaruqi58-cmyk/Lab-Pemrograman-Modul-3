#!/usr/bin/env bash
# PinjamLab Module 3: complete verification for Labs 1–3.
# The script fails loudly if purity, C++ tests, golden outputs, Python tests,
# or cleanup/leak evidence checks do not pass.
set -euo pipefail

cd "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
mkdir -p bin

CXX="${CXX:-g++}"
PY="${PY:-python}"
command -v "$CXX" >/dev/null || { echo "FATAL: no C++ compiler ($CXX) on PATH."; exit 2; }
command -v "$PY"  >/dev/null || { echo "FATAL: no Python interpreter ($PY) on PATH."; exit 2; }

echo "================================================================================"
echo " [PinjamLab M3] $("$CXX" --version | head -1)"
echo " [PinjamLab M3] $("$PY" --version 2>&1)"
echo "================================================================================"

# ---- 1/6: dependency-rule purity check --------------------------------------
echo ""
echo ">>> [1/6] Dependency-rule purity check (C++ includes + Python imports)"
"$PY" scripts/check_dependencies.py

# ---- 2/6: build C++ suites ----------------------------------------------------
echo ""
echo ">>> [2/6] Building C++20 tests (-Wall -Wextra -Werror)"
for t in test_errors test_raii test_result test_app; do
    "$CXX" -std=c++20 -Wall -Wextra -Werror \
        -Icpp/include -Icpp/tests "cpp/tests/$t.cpp" -o "bin/$t"
done
"$CXX" -std=c++20 -Wall -Wextra -Werror -Icpp/include cpp/app/main.cpp -o bin/pinjamlab_cli
"$CXX" -std=c++20 -Wall -Wextra -Werror cpp/tests/leak_demo.cpp -o bin/leak_demo

# ---- 3/6: run C++ suites ------------------------------------------------------
echo ""
echo ">>> [3/6] Running C++20 test suites"
for t in test_errors test_raii test_result test_app; do
    "./bin/$t"
done

# ---- 4/6: cross-language golden files ---------------------------------------
echo ""
echo ">>> [4/6] Checking fixed and student-authored CSV outputs"
check_cli_output() {
    local csv="$1" expected="$2" stderr_file="$3" expected_lf="$4" status
    set +e
    ./bin/pinjamlab_cli "$csv" > /dev/null 2> "$stderr_file"
    status=$?
    set -e
    if [ "$status" -ne 1 ]; then
        echo "FATAL: expected CLI exit code 1 for $csv, got $status"
        return 1
    fi
    # Normalize CRLF from Windows checkouts before byte-for-byte comparison.
    sed 's/\r$//' "$expected" > "$expected_lf"
    sed 's/\r$//' "$stderr_file" > "${stderr_file}.lf"
    diff "${stderr_file}.lf" "$expected_lf"
    echo "  match: $csv -> $expected"
}
check_cli_output sample-data/bad-requests.csv sample-data/expected-output.txt \
    bin/cpp_stderr_fixed.txt bin/expected_fixed_lf.txt
check_cli_output sample-data/my-bad-requests.csv sample-data/my-expected-output.txt \
    bin/cpp_stderr_custom.txt bin/expected_custom_lf.txt

# ---- 5/6: Python suites -------------------------------------------------------
echo ""
echo ">>> [5/6] Running Python test suite"
PYTHONPATH=py "$PY" -m pytest py/tests -q

# ---- 6/6: leak reproduction and RAII fix -------------------------------------
echo ""
echo ">>> [6/6] Defect B: reproduce raw-pointer leak, then verify RAII cleanup"
asan_available=0
if "$CXX" -std=c++20 -Wall -Wextra -Werror -fsanitize=address \
      -fno-omit-frame-pointer -g cpp/tests/leak_demo.cpp -o bin/leak_demo_asan \
      >bin/asan_build.txt 2>&1; then
    set +e
    ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 ./bin/leak_demo_asan raw \
        >bin/asan_before.txt 2>&1
    raw_status=$?
    set -e
    if [ "$raw_status" -ne 0 ] && \
       grep -Eq 'ERROR: LeakSanitizer: detected memory leaks|Direct leak' bin/asan_before.txt; then
        if ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 ./bin/leak_demo_asan raii \
             >bin/asan_after.txt 2>&1; then
            if ! grep -Eq 'ERROR: LeakSanitizer: detected memory leaks|Direct leak' bin/asan_after.txt; then
                asan_available=1
                echo "  reproduced: ASan reports the raw-pointer leak (bin/asan_before.txt)"
                echo "  fixed: unique_ptr path exits cleanly (bin/asan_after.txt)"
            fi
        fi
    fi
fi

if [ "$asan_available" -ne 1 ]; then
    # Some MinGW/Windows GCC distributions do not include a working libasan.
    # The module README permits a destructor/live-object counter as fallback.
    set +e
    ./bin/leak_demo raw >bin/leak_before_fallback.txt 2>&1
    raw_status=$?
    ./bin/leak_demo raii >bin/leak_after_fallback.txt 2>&1
    raii_status=$?
    set -e
    if [ "$raw_status" -ne 1 ] || [ "$raii_status" -ne 0 ] || \
       ! grep -q 'live objects after exception: 1' bin/leak_before_fallback.txt || \
       ! grep -q 'live objects after exception: 0' bin/leak_after_fallback.txt; then
        echo "FATAL: leak fallback did not show raw=1 live object and RAII=0."
        cat bin/leak_before_fallback.txt bin/leak_after_fallback.txt
        exit 4
    fi
    echo "  ASan not available; live-object counter confirms raw=1, unique_ptr=0."
    echo "  evidence: bin/leak_before_fallback.txt and bin/leak_after_fallback.txt"
fi

echo ""
echo "================================================================================"
echo " [SUCCESS] Labs 1–3 checks passed: dependency rule, Result refactor, tests, golden output, and cleanup evidence."
echo "================================================================================"

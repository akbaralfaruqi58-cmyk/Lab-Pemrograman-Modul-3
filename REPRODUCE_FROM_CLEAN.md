# Lab 3.4 — Reproduce from a clean clone

The purpose is to prove that a classmate can reproduce the exact fixed-file and
student-authored-file diagnostics from the branch README alone.

## 1. Push the completed branch

From the project working copy:

```powershell
git status
git push -u origin m3-work
```

## 2. Clone a clean copy

Use the repository URL assigned to your group/repository:

```bash
git clone --branch m3-work <REPOSITORY_URL> Module3-clean
cd Module3-clean
```

Do not copy `bin/`, `.pytest_cache/`, or `__pycache__/` from the original working
folder. A clean clone must build and generate its own test artifacts.

## 3. Reproduce the verification

From Git Bash:

```bash
./run_tests.sh
```

The script checks the dependency rule, builds and runs the C++20 tests, checks
the C++ CLI output against both committed expected-output files, runs the Python
tests, and verifies the raw-pointer/RAII leak demonstration.

Acceptance criteria:

- `sample-data/bad-requests.csv` produces the exact lines in
  `sample-data/expected-output.txt` in the same order.
- `sample-data/my-bad-requests.csv` produces the exact lines in
  `sample-data/my-expected-output.txt` in the same order.
- Both catalogue runs return exit code `1` because those files intentionally
  contain rejected rows.
- `run_tests.sh` ends with `[SUCCESS]` and returns process status `0` because
  every assertion/check passed.

## 4. Record reproducibility

Record the clean-clone commit hash, compiler/Python/pytest versions, the final
`run_tests.sh` output, and any differences. A different message, line order, or
exit code is a reproducibility defect and must be fixed before the report claims
success.

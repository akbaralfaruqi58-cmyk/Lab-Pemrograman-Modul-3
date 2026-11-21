# Lab 3.1 — Prediction recorded before the Result refactor

**Decision:** `EquipmentUnavailable` should be an expected `Result<Loan>` failure at the `LoanService::borrow()` boundary, not an exception for the ordinary case where an item is already on loan.

## Reasoning

The caller of `borrow()` in the normal application path is `app/runner`. It processes a catalogue row and can immediately act when an item is not available: report the row as rejected, tell the user the equipment is currently loaned, or later offer a waiting-list workflow. An item being loaned is a normal operational state in PinjamLab; it does not mean the service has violated its contract or encountered an unforeseen failure.

A typed exception remains appropriate for conditions such as an unknown asset tag, an unknown member, a quota rule violation, or a reservation conflict: those indicate an invalid request or a domain rule that the current operation cannot fulfil, and the application may handle those distinctly.

## Prediction about call sites

Before implementation, I predict that **four production call expressions in each language** need adaptation:

1. The three seeding calls in `app/seed.hpp` / `app/seed.py` must explicitly unwrap the successful `Result` value.
2. The row-processing call in `app/runner.hpp` / `app/runner.py` must inspect `ok()` / `ok` and report the expected failure message instead of relying on an exception handler.

Test call sites that consume a `Loan` value will also need to unwrap the result, and the former `EquipmentUnavailable` exception test will become a `Result.failure` assertion. I predict the application row handler will become simpler for this expected outcome because the normal unavailable path becomes an explicit branch rather than exception control flow.

## Evidence to record after implementation

After the code has been changed and tested, first include new files in the diff view,
then run:

```powershell
git add -N .
git diff --stat
```

Paste the actual output into `LAB3_REPORT.md` and state whether the prediction about the four production call expressions was correct. The actual diff is intentionally not pre-filled here; it must come from the student's Git working tree after applying the refactor.

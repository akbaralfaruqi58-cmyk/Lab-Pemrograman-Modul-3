#ifndef PINJAMLAB_APP_RUNNER_HPP
#define PINJAMLAB_APP_RUNNER_HPP

// app layer: the top-level handler. This is the ONE place in the codebase
// allowed to catch `const LoanError&` and turn it into a message a human
// reads, and the ONE place that decides the process exit code. It contains
// no business rules of its own.

#include "pinjamlab/domain/loan_error.hpp"
#include "pinjamlab/io/csv_reader.hpp"
#include "pinjamlab/io/report_writer.hpp"
#include "pinjamlab/io/validator.hpp"
#include "pinjamlab/service/loan_service.hpp"

#include <string>
#include <vector>

namespace pinjamlab {

struct RunResult {
    std::vector<std::string> rejectedLines;  // exactly what expected-output.txt holds
    int total = 0;
    int created = 0;
    int rejected = 0;
    [[nodiscard]] int exitCode() const noexcept { return rejected == 0 ? 0 : 1; }
};

// Pure of console I/O on purpose: returns the lines instead of printing them,
// so both the CLI (main.cpp) and the tests (test_app.cpp) can drive the same
// code path -- one prints to stderr, the other diffs against a golden file.
[[nodiscard]] inline RunResult runCatalogue(LoanService& svc, const std::string& csvPath) {
    CsvReader reader;
    Validator validator;
    RunResult result;

    const auto rows = reader.read(csvPath);
    result.total = static_cast<int>(rows.size());

    for (const auto& row : rows) {
        const auto validated = validator.validate(row);
        if (!validated.ok()) {
            ++result.rejected;
            result.rejectedLines.push_back(
                ReportWriter::rejectedLine(row.lineNumber, validated.error()));
            continue;
        }
        const auto& req = validated.value();
        try {
            (void)svc.borrow(req.tag, req.memberId, req.days);
            ++result.created;
        } catch (const ReservationConflict& e) {  // the handler uses structured context
            ++result.rejected;
            const std::string message =
                "Reservation conflict for " + e.tag() + ": reserved by " +
                e.reservedBy() + ", requested by " + e.requestedBy();
            result.rejectedLines.push_back(
                ReportWriter::rejectedLine(row.lineNumber, message));
        } catch (const LoanError& e) {  // the remaining family we understand
            ++result.rejected;
            result.rejectedLines.push_back(
                ReportWriter::rejectedLine(row.lineNumber, e.what()));
        }
        // Anything that is NOT a LoanError is left to travel: we do not know
        // what it is, so we are not entitled to decide it is unimportant.
    }
    return result;
}

// App-level handler for fine assessment. It translates the typed domain
// failure into a human-readable message using the exception's structured data.
[[nodiscard]] inline std::string assessFineForDisplay(
    const LoanService& svc, const Loan& loan, int overdueDays,
    long long dailyRateCents, long long capCents) {
    try {
        const long long fine = svc.calculateFineCents(
            loan, overdueDays, dailyRateCents, capCents);
        return "Fine for " + loan.id() + ": " + std::to_string(fine) + " cents";
    } catch (const FineCapReached& e) {
        return "Fine cap reached for " + e.loanId() + ": calculated " +
               std::to_string(e.calculatedFineCents()) + " cents, cap " +
               std::to_string(e.capCents()) + " cents";
    }
}

}  // namespace pinjamlab

#endif  // PINJAMLAB_APP_RUNNER_HPP

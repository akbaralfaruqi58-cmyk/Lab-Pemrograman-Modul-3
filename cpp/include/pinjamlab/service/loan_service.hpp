#ifndef PINJAMLAB_SERVICE_LOAN_SERVICE_HPP
#define PINJAMLAB_SERVICE_LOAN_SERVICE_HPP

// Orchestration only: LoanService coordinates domain objects. Expected
// equipment unavailability is returned as Result<Loan>; invalid requests and
// other domain-rule failures remain typed exceptions. No CSV, console, or
// presentation formatting belongs in this service layer.

#include "pinjamlab/domain/equipment.hpp"
#include "pinjamlab/domain/loan.hpp"
#include "pinjamlab/domain/loan_error.hpp"
#include "pinjamlab/domain/member.hpp"
#include "pinjamlab/domain/result.hpp"
#include "pinjamlab/service/database.hpp"
#include "pinjamlab/service/txn_guard.hpp"

#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

namespace pinjamlab {

class LoanService {
public:
    static constexpr int kMaxLoanDays = 90;

    void addEquipment(Equipment eq) {
        catalog_.emplace(eq.tag(), std::move(eq));
    }
    void addMember(Member m) {
        roster_.emplace(m.id(), std::move(m));
    }

    [[nodiscard]] Equipment* findEquipment(const std::string& tag) {
        auto it = catalog_.find(tag);
        return it == catalog_.end() ? nullptr : &it->second;
    }
    [[nodiscard]] Member* findMember(const std::string& id) {
        auto it = roster_.find(id);
        return it == roster_.end() ? nullptr : &it->second;
    }

    // Reserve an available equipment item for a known member. A second
    // member cannot replace an existing reservation.
    void reserveEquipment(const std::string& tag, const std::string& memberId) {
        auto eqIt = catalog_.find(tag);
        if (eqIt == catalog_.end()) throw UnknownAssetTag(tag);

        auto memIt = roster_.find(memberId);
        if (memIt == roster_.end()) throw UnknownMember(memberId);

        eqIt->second.reserveFor(memberId);
    }

    // Fine is calculated in integer cents to avoid floating-point currency
    // errors. Negative inputs are programmer/caller contract violations;
    // exceeding the configured cap is a typed domain failure.
    [[nodiscard]] long long calculateFineCents(
        const Loan& loan, int overdueDays, long long dailyRateCents,
        long long capCents) const {
        if (overdueDays < 0 || dailyRateCents < 0 || capCents < 0) {
            throw std::invalid_argument("fine inputs must not be negative");
        }

        long long calculated = 0;
        if (overdueDays > 0 && dailyRateCents >
            std::numeric_limits<long long>::max() / overdueDays) {
            calculated = std::numeric_limits<long long>::max();
        } else {
            calculated = static_cast<long long>(overdueDays) * dailyRateCents;
        }

        if (calculated > capCents) {
            throw FineCapReached(loan.id(), calculated, capCents);
        }
        return calculated;
    }

    // The failure cascade, guard by guard. Every check below happens BEFORE
    // any mutation, so no rollback is ever needed for the current guard set
    // -- the TxnGuard exists for the mutation step itself, to keep the
    // pattern honest for the day a guard is added AFTER a mutation begins
    // (Module 5's database transactions are exactly that day).
    // Equipment already being on loan is an expected operational outcome:
    // callers can report it, retry later, or offer a waiting list. Other
    // contract/business violations remain typed exceptions.
    [[nodiscard]] Result<Loan> borrow(
        const std::string& tag, const std::string& memberId, int days) {
        auto eqIt = catalog_.find(tag);
        if (eqIt == catalog_.end()) throw UnknownAssetTag(tag);

        auto memIt = roster_.find(memberId);
        if (memIt == roster_.end()) throw UnknownMember(memberId);

        if (days > kMaxLoanDays) throw InvalidLoanDuration(days, kMaxLoanDays);

        Equipment& eq = eqIt->second;
        Member& mem = memIt->second;

        if (!eq.reservedBy().empty() && eq.reservedBy() != memberId) {
            throw ReservationConflict(tag, eq.reservedBy(), memberId);
        }
        if (!eq.isAvailable()) {
            return Result<Loan>::failure(EquipmentUnavailable(tag).what());
        }
        if (!mem.canBorrow()) throw QuotaExceeded(mem.activeLoans(), mem.maxLoans());

        TxnGuard txn(db_);
        mem.recordLoan();
        db_.stage("loan:" + memberId + ":" + tag);
        try {
            eq.markLoaned();
        } catch (...) {
            mem.undoRecordLoan();  // manual compensation: txn.commit() was
            throw;                 // never called, so the guard still rolls
        }                          // back the LEDGER on the way out.
        db_.stage("equipment:" + tag + ":loaned");
        txn.commit();

        return Result<Loan>::success(
            Loan("L-" + tag + "-" + memberId, tag, memberId, days));
    }

    // Compatibility adapter used only to compare the old throwing behaviour
    // with the new Result outcome in Lab 3.1 tests. Production callers use borrow().
    [[nodiscard]] Loan borrowThrowingForComparison(
        const std::string& tag, const std::string& memberId, int days) {
        auto outcome = borrow(tag, memberId, days);
        if (!outcome.ok()) {
            throw EquipmentUnavailable(tag);
        }
        return outcome.value();
    }

    void returnItem(const std::string& loanTag, const std::string& memberId) {
        auto eqIt = catalog_.find(loanTag);
        auto memIt = roster_.find(memberId);
        if (eqIt != catalog_.end()) eqIt->second.markReturned();
        if (memIt != roster_.end()) memIt->second.recordReturn();
    }

    [[nodiscard]] const Database& ledger() const noexcept { return db_; }

private:
    std::unordered_map<std::string, Equipment> catalog_;
    std::unordered_map<std::string, Member> roster_;
    Database db_;
};

}  // namespace pinjamlab

#endif  // PINJAMLAB_SERVICE_LOAN_SERVICE_HPP

#ifndef PINJAMLAB_IO_VALIDATOR_HPP
#define PINJAMLAB_IO_VALIDATOR_HPP

// io layer: converts a RawRow into a Result<LoanRequest>. Format failures
// (empty fields, unparseable or non-positive numbers) are rejected HERE,
// with plain string errors -- they never reach domain, and domain never
// sees a request that could not possibly be valid. This is the boundary
// Figure "The Failure Cascade" calls guard 2.

#include "pinjamlab/domain/result.hpp"
#include "pinjamlab/io/csv_reader.hpp"

#include <string>

namespace pinjamlab {

struct LoanRequest {
    std::string tag;
    std::string memberId;
    int days;
};

class Validator {
public:
    [[nodiscard]] Result<LoanRequest> validate(const RawRow& row) const {
        if (row.tag.empty()) return Result<LoanRequest>::failure("tag is empty");
        if (isWhitespaceOnly(row.tag)) {
            return Result<LoanRequest>::failure("tag must not be whitespace-only");
        }
        if (!hasCanonicalTag(row.tag)) {
            return Result<LoanRequest>::failure(
                "tag must use canonical format LP-NNN, got: " + row.tag);
        }

        if (row.memberId.empty()) {
            return Result<LoanRequest>::failure("memberId is empty");
        }
        if (isWhitespaceOnly(row.memberId)) {
            return Result<LoanRequest>::failure("memberId must not be whitespace-only");
        }
        if (!hasCanonicalMemberId(row.memberId)) {
            return Result<LoanRequest>::failure(
                "memberId must use canonical format M-NNN, got: " + row.memberId);
        }

        int days = 0;
        const ParseStatus status = parseInt(row.daysRaw, days);
        if (status == ParseStatus::NotNumber) {
            return Result<LoanRequest>::failure("days is not a number: " + row.daysRaw);
        }
        if (status == ParseStatus::OutOfRange) {
            return Result<LoanRequest>::failure(
                "days is outside supported integer range: " + row.daysRaw);
        }
        if (days <= 0) {
            return Result<LoanRequest>::failure(
                "days must be positive, got " + std::to_string(days));
        }
        return Result<LoanRequest>::success(LoanRequest{row.tag, row.memberId, days});
    }

private:
    enum class ParseStatus { Ok, NotNumber, OutOfRange };

    [[nodiscard]] static bool isWhitespaceOnly(const std::string& value) {
        for (const unsigned char ch : value) {
            if (ch != ' ' && ch != '\t' && ch != '\n' && ch != '\r' &&
                ch != '\f' && ch != '\v') return false;
        }
        return true;
    }

    [[nodiscard]] static bool hasCanonicalTag(const std::string& value) {
        if (value.size() != 6 || value.compare(0, 3, "LP-") != 0) return false;
        for (std::size_t i = 3; i < value.size(); ++i) {
            if (value[i] < '0' || value[i] > '9') return false;
        }
        return true;
    }

    [[nodiscard]] static bool hasCanonicalMemberId(const std::string& value) {
        if (value.size() != 5 || value.compare(0, 2, "M-") != 0) return false;
        for (std::size_t i = 2; i < value.size(); ++i) {
            if (value[i] < '0' || value[i] > '9') return false;
        }
        return true;
    }

    // Parse without overflow. C++ int is a 32-bit signed value on the target
    // toolchains used by this lab; the range check happens before multiplication.
    [[nodiscard]] static ParseStatus parseInt(const std::string& value, int& out) {
        if (value.empty()) return ParseStatus::NotNumber;
        std::size_t i = 0;
        bool negative = false;
        if (value[0] == '+' || value[0] == '-') {
            negative = value[0] == '-';
            i = 1;
        }
        if (i == value.size()) return ParseStatus::NotNumber;

        const long long limit = negative ? 2147483648LL : 2147483647LL;
        long long parsed = 0;
        for (; i < value.size(); ++i) {
            const char ch = value[i];
            if (ch < '0' || ch > '9') return ParseStatus::NotNumber;
            const int digit = ch - '0';
            if (parsed > (limit - digit) / 10) return ParseStatus::OutOfRange;
            parsed = parsed * 10 + digit;
        }

        const long long signedValue = negative ? -parsed : parsed;
        out = static_cast<int>(signedValue);
        return ParseStatus::Ok;
    }
};

}  // namespace pinjamlab

#endif  // PINJAMLAB_IO_VALIDATOR_HPP

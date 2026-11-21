// The acceptance criterion for Lab 2: running the fixed bad-requests.csv
// through the real io+service+domain pipeline must produce the ten messages
// in sample-data/expected-output.txt, in order, byte for byte, with a
// non-zero exit code. This is the golden-file test Lab 3.4 asks a partner to
// reproduce from a clean clone.

#include "mini_catch.hpp"
#include "pinjamlab/app/runner.hpp"
#include "pinjamlab/app/seed.hpp"

#include <fstream>
#include <cstdio>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <chrono>

using namespace pinjamlab;

static std::vector<std::string> readLines(const std::string& path) {
    std::ifstream in(path);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        lines.push_back(line);
    }
    return lines;
}

TEST_CASE("bad-requests.csv reproduces expected-output.txt byte for byte") {
    LoanService svc;
    seedPinjamLab(svc);
    const auto result = runCatalogue(svc, "sample-data/bad-requests.csv");

    const auto expected = readLines("sample-data/expected-output.txt");
    REQUIRE(result.rejectedLines.size() == expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        REQUIRE(result.rejectedLines[i] == expected[i]);
    }
    REQUIRE(result.total == 10);
    REQUIRE(result.created == 0);
    REQUIRE(result.rejected == 10);
    REQUIRE(result.exitCode() == 1);
}


TEST_CASE("my-bad-requests.csv reports all five custom failures") {
    LoanService svc;
    seedPinjamLab(svc);
    const auto result = runCatalogue(svc, "sample-data/my-bad-requests.csv");
    const auto expected = readLines("sample-data/my-expected-output.txt");
    REQUIRE(result.rejectedLines.size() == expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        REQUIRE(result.rejectedLines[i] == expected[i]);
    }
    REQUIRE(result.total == 5);
    REQUIRE(result.created == 0);
    REQUIRE(result.rejected == 5);
    REQUIRE(result.exitCode() == 1);
}

TEST_CASE("app catches ReservationConflict and reports its structured context") {
    const std::string path = "bin/reservation-conflict-test.csv";
    {
        std::ofstream out(path);
        out << "tag,member_id,days\nLP-901,M-101,7\n";
    }

    LoanService svc;
    svc.addEquipment(Equipment("LP-901", "Test equipment"));
    svc.addMember(Member("M-101", "Ayu Lestari", Role::Student));
    svc.addMember(Member("M-102", "Budi Santoso", Role::Student));
    svc.reserveEquipment("LP-901", "M-102");

    const auto result = runCatalogue(svc, path);
    std::remove(path.c_str());
    REQUIRE(result.rejected == 1);
    REQUIRE(result.exitCode() == 1);
    REQUIRE(result.rejectedLines.size() == 1);
    REQUIRE(result.rejectedLines[0] ==
            "row 1: Reservation conflict for LP-901: reserved by M-102, requested by M-101");
}

TEST_CASE("app FineCapReached handler uses structured values") {
    LoanService svc;
    const Loan loan("L-TEST", "LP-001", "M-101", 7);
    const std::string message = assessFineForDisplay(svc, loan, 5, 250, 1000);
    REQUIRE(message ==
            "Fine cap reached for L-TEST: calculated 1250 cents, cap 1000 cents");
}

TEST_CASE("unexpected input-file errors are not swallowed by app") {
    LoanService svc;

    // Use a fresh path under the system temporary directory. A fixed path
    // under bin/ can accidentally exist from a previous local run, making
    // the test pass through the reader instead of exercising the error path.
    const auto uniqueName =
        "pinjamlab-missing-input-" +
        std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()
        );
    const auto missingPath =
        std::filesystem::temp_directory_path() / uniqueName / "missing.csv";

    REQUIRE(!std::filesystem::exists(missingPath));
    REQUIRE_THROWS_AS(
        (void)runCatalogue(svc, missingPath.string()),
        std::runtime_error
    );
}

int main() { return test_runner::runAll(); }

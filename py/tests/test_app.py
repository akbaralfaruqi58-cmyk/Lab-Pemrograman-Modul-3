"""The acceptance criterion for Lab 2: running the fixed bad-requests.csv
through the real io+service+domain pipeline must produce the ten messages in
sample-data/expected-output.txt, in order, byte for byte, with a non-zero
exit code -- and it must be the SAME ten messages the C++ side produces.
"""
from pathlib import Path

import pytest

from pinjamlab.app.runner import assess_fine_for_display, run_catalogue
from pinjamlab.app.seed import seed_pinjamlab
from pinjamlab.domain.equipment import Equipment
from pinjamlab.domain.loan import Loan
from pinjamlab.domain.member import Member, Role
from pinjamlab.service.loan_service import LoanService

ROOT = Path(__file__).resolve().parents[2]
CSV = ROOT / "sample-data" / "bad-requests.csv"
EXPECTED = ROOT / "sample-data" / "expected-output.txt"


def test_bad_requests_csv_reproduces_expected_output_byte_for_byte():
    svc = LoanService()
    seed_pinjamlab(svc)
    result = run_catalogue(svc, str(CSV))

    expected_lines = EXPECTED.read_text(encoding="utf-8").splitlines()
    assert result.rejected_lines == expected_lines
    assert result.total == 10
    assert result.created == 0
    assert result.rejected == 10
    assert result.exit_code == 1


def test_my_bad_requests_csv_reports_all_five_custom_failures():
    svc = LoanService()
    seed_pinjamlab(svc)
    custom_csv = ROOT / "sample-data" / "my-bad-requests.csv"
    custom_expected = ROOT / "sample-data" / "my-expected-output.txt"

    result = run_catalogue(svc, str(custom_csv))
    expected_lines = custom_expected.read_text(encoding="utf-8").splitlines()

    assert result.rejected_lines == expected_lines
    assert result.total == 5
    assert result.created == 0
    assert result.rejected == 5
    assert result.exit_code == 1


def test_app_catches_reservation_conflict_and_uses_structured_context(tmp_path):
    csv_path = tmp_path / "reservation-conflict.csv"
    csv_path.write_text("tag,member_id,days\nLP-901,M-101,7\n", encoding="utf-8")

    svc = LoanService()
    svc.add_equipment(Equipment("LP-901", "Test equipment"))
    svc.add_member(Member("M-101", "Ayu Lestari", Role.STUDENT))
    svc.add_member(Member("M-102", "Budi Santoso", Role.STUDENT))
    svc.reserve_equipment("LP-901", "M-102")

    result = run_catalogue(svc, str(csv_path))
    assert result.rejected == 1
    assert result.exit_code == 1
    assert result.rejected_lines == [
        "row 1: Reservation conflict for LP-901: reserved by M-102, requested by M-101"
    ]


def test_app_fine_cap_handler_uses_structured_values():
    svc = LoanService()
    loan = Loan("L-TEST", "LP-001", "M-101", 7)
    message = assess_fine_for_display(svc, loan, 5, 250, 1000)
    assert message == (
        "Fine cap reached for L-TEST: calculated 1250 cents, cap 1000 cents"
    )


def test_unexpected_csv_open_error_is_not_swallowed(tmp_path):
    svc = LoanService()
    with pytest.raises(FileNotFoundError):
        run_catalogue(svc, str(tmp_path / "does-not-exist.csv"))

"""Result[T]: proves value on a failed Result raises, and that a Result-
based rule and a throwing equivalent agree on every input -- the
"from-scratch match" Lab 2.2 requires.
"""
import pytest

from pinjamlab.domain.result import Result
from pinjamlab.domain.loan_error import EquipmentUnavailable
from pinjamlab.app.seed import seed_pinjamlab
from pinjamlab.service.loan_service import LoanService
from pinjamlab.io.csv_reader import RawRow
from pinjamlab.io.validator import Validator


def test_success_carries_a_value_and_ok_is_true():
    r = Result.success(42)
    assert r.ok is True
    assert r.value == 42
    assert r.error == ""


def test_failure_carries_a_message_and_ok_is_false():
    r = Result.failure("days must be positive, got -3")
    assert r.ok is False
    assert r.error == "days must be positive, got -3"


def test_value_on_a_failed_result_raises_rather_than_returning_garbage():
    r = Result.failure("tag is empty")
    with pytest.raises(RuntimeError):
        _ = r.value


def parse_days_or_raise(raw: str) -> int:
    """A deliberately duplicated throwing version of Validator's own rule,
    so the two can be compared on the same inputs."""
    try:
        days = int(raw)
    except ValueError as exc:
        raise ValueError(f"days is not a number: {raw}") from exc
    if days < -2147483648 or days > 2147483647:
        raise ValueError(f"days is outside supported integer range: {raw}")
    if days <= 0:
        raise ValueError(f"days must be positive, got {days}")
    return days


def test_from_scratch_match_agree_on_well_formed_input():
    v = Validator()
    row = RawRow(1, "LP-001", "M-101", "7")
    r = v.validate(row)
    assert r.ok
    assert r.value.days == 7
    assert parse_days_or_raise("7") == 7



def test_equipment_unavailable_result_matches_prior_throwing_behavior():
    result_service = LoanService()
    seed_pinjamlab(result_service)
    outcome = result_service.borrow("LP-002", "M-101", 7)
    assert not outcome.ok
    assert outcome.error == "Equipment already on loan: LP-002"

    throwing_service = LoanService()
    seed_pinjamlab(throwing_service)
    with pytest.raises(EquipmentUnavailable) as exc:
        throwing_service.borrow_throwing_for_comparison("LP-002", "M-101", 7)
    assert str(exc.value) == outcome.error


def test_equipment_unavailable_result_preserves_successful_loan_values():
    result_service = LoanService()
    seed_pinjamlab(result_service)
    outcome = result_service.borrow("LP-001", "M-101", 7)
    assert outcome.ok

    throwing_service = LoanService()
    seed_pinjamlab(throwing_service)
    old_shape = throwing_service.borrow_throwing_for_comparison("LP-001", "M-101", 7)
    assert outcome.value == old_shape


@pytest.mark.parametrize("raw", ["abc", "-3", "0", "2147483648"])
def test_from_scratch_match_agree_on_every_malformed_input(raw):
    v = Validator()
    row = RawRow(1, "LP-001", "M-101", raw)
    r = v.validate(row)
    assert not r.ok  # Result: caller decides, cannot ignore .ok

    with pytest.raises(ValueError) as exc:
        parse_days_or_raise(raw)
    assert r.error == str(exc.value)

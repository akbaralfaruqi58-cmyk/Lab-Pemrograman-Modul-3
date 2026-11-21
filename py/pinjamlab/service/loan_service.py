"""Orchestration only: coordinate domain objects without CSV or console I/O.
Expected equipment unavailability is a Result failure; other domain failures
remain typed exceptions.
"""
from __future__ import annotations

from ..domain.equipment import Equipment
from ..domain.loan import Loan
from ..domain.loan_error import (
    EquipmentUnavailable,
    InvalidLoanDuration,
    FineCapReached,
    QuotaExceeded,
    ReservationConflict,
    UnknownAssetTag,
    UnknownMember,
)
from ..domain.member import Member
from ..domain.result import Result
from .database import Database
from .txn_guard import txn_guard

MAX_LOAN_DAYS = 90


class LoanService:
    def __init__(self) -> None:
        self._catalog: dict[str, Equipment] = {}
        self._roster: dict[str, Member] = {}
        self._db = Database()

    def add_equipment(self, equipment: Equipment) -> None:
        self._catalog[equipment.tag] = equipment

    def add_member(self, member: Member) -> None:
        self._roster[member.member_id] = member

    def find_equipment(self, tag: str) -> Equipment | None:
        return self._catalog.get(tag)

    def find_member(self, member_id: str) -> Member | None:
        return self._roster.get(member_id)

    @property
    def ledger(self) -> Database:
        return self._db

    def reserve_equipment(self, tag: str, member_id: str) -> None:
        equipment = self._catalog.get(tag)
        if equipment is None:
            raise UnknownAssetTag(tag)

        member = self._roster.get(member_id)
        if member is None:
            raise UnknownMember(member_id)

        equipment.reserve_for(member_id)

    def calculate_fine_cents(
        self, loan: Loan, overdue_days: int, daily_rate_cents: int, cap_cents: int
    ) -> int:
        if overdue_days < 0 or daily_rate_cents < 0 or cap_cents < 0:
            raise ValueError("fine inputs must not be negative")

        max_cents = 9223372036854775807
        if overdue_days > 0 and daily_rate_cents > max_cents // overdue_days:
            calculated = max_cents
        else:
            calculated = overdue_days * daily_rate_cents
        if calculated > cap_cents:
            raise FineCapReached(loan.loan_id, calculated, cap_cents)
        return calculated

    def borrow(self, tag: str, member_id: str, days: int) -> Result[Loan]:
        """Borrow equipment; already-loaned is an expected Result failure."""
        equipment = self._catalog.get(tag)
        if equipment is None:
            raise UnknownAssetTag(tag)

        member = self._roster.get(member_id)
        if member is None:
            raise UnknownMember(member_id)

        if days > MAX_LOAN_DAYS:
            raise InvalidLoanDuration(days, MAX_LOAN_DAYS)

        # Reservation ownership is checked before any mutation.
        if equipment.reserved_by is not None and equipment.reserved_by != member_id:
            raise ReservationConflict(tag, equipment.reserved_by, member_id)
        if not equipment.is_available():
            return Result.failure(str(EquipmentUnavailable(tag)))
        if not member.can_borrow():
            raise QuotaExceeded(member.active_loans, member.max_loans)

        with txn_guard(self._db) as db:
            member.record_loan()
            db.stage(f"loan:{member_id}:{tag}")
            try:
                equipment.mark_loaned()
            except Exception:
                member.undo_record_loan()
                raise
            db.stage(f"equipment:{tag}:loaned")

        return Result.success(Loan(f"L-{tag}-{member_id}", tag, member_id, days))

    def borrow_throwing_for_comparison(self, tag: str, member_id: str, days: int) -> Loan:
        """Test adapter preserving the pre-refactor throwing behaviour."""
        outcome = self.borrow(tag, member_id, days)
        if not outcome.ok:
            raise EquipmentUnavailable(tag)
        return outcome.value

    def return_item(self, tag: str, member_id: str) -> None:
        equipment = self._catalog.get(tag)
        member = self._roster.get(member_id)
        if equipment is not None:
            equipment.mark_returned()
        if member is not None:
            member.record_return()

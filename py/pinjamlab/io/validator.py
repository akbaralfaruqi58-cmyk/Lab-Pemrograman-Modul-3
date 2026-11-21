"""io layer: converts a RawRow into a Result[LoanRequest]. Format failures
are rejected HERE with plain string errors -- they never reach domain.
"""
from __future__ import annotations

from dataclasses import dataclass
import re

from ..domain.result import Result
from .csv_reader import RawRow


@dataclass(frozen=True, slots=True)
class LoanRequest:
    tag: str
    member_id: str
    days: int


class Validator:
    def validate(self, row: RawRow) -> Result[LoanRequest]:
        if row.tag == "":
            return Result.failure("tag is empty")
        if not row.tag.strip():
            return Result.failure("tag must not be whitespace-only")
        if re.fullmatch(r"LP-[0-9]{3}", row.tag) is None:
            return Result.failure(
                f"tag must use canonical format LP-NNN, got: {row.tag}"
            )

        if row.member_id == "":
            return Result.failure("memberId is empty")
        if not row.member_id.strip():
            return Result.failure("memberId must not be whitespace-only")
        if re.fullmatch(r"M-[0-9]{3}", row.member_id) is None:
            return Result.failure(
                f"memberId must use canonical format M-NNN, got: {row.member_id}"
            )

        if re.fullmatch(r"[+-]?[0-9]+", row.days_raw) is None:
            return Result.failure(f"days is not a number: {row.days_raw}")
        days = int(row.days_raw)

        if days < -2147483648 or days > 2147483647:
            return Result.failure(
                f"days is outside supported integer range: {row.days_raw}"
            )
        if days <= 0:
            return Result.failure(f"days must be positive, got {days}")

        return Result.success(LoanRequest(row.tag, row.member_id, days))

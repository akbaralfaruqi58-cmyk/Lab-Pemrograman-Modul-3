"""Pure domain module: Equipment entity, carried forward from Modules 1-2."""
from __future__ import annotations

from enum import Enum, auto

from .loan_error import EquipmentUnavailable, ReservationConflict


class EquipmentState(Enum):
    AVAILABLE = auto()
    LOANED = auto()


class Equipment:
    def __init__(self, tag: str, model: str) -> None:
        self.tag = tag
        self.model = model
        self._state = EquipmentState.AVAILABLE
        self._reserved_by: str | None = None

    @property
    def state(self) -> EquipmentState:
        return self._state

    def is_available(self) -> bool:
        return self._state == EquipmentState.AVAILABLE

    @property
    def reserved_by(self) -> str | None:
        return self._reserved_by

    def reserve_for(self, member_id: str) -> None:
        if self._reserved_by is not None and self._reserved_by != member_id:
            raise ReservationConflict(self.tag, self._reserved_by, member_id)
        if not self.is_available():
            raise EquipmentUnavailable(self.tag)
        self._reserved_by = member_id

    def mark_loaned(self) -> None:
        if not self.is_available():
            raise EquipmentUnavailable(self.tag)
        self._state = EquipmentState.LOANED
        self._reserved_by = None

    def mark_returned(self) -> None:
        self._state = EquipmentState.AVAILABLE

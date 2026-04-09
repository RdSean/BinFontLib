from __future__ import annotations

from datetime import datetime, timezone
from typing import Any

from sqlalchemy import Index, Integer, String, Text
from sqlalchemy.orm import Mapped, mapped_column
from sqlalchemy.types import TypeDecorator

from .db import Base


def _to_utc_iso(value: datetime) -> str:
    if value.tzinfo is None:
        value = value.replace(tzinfo=timezone.utc)
    value = value.astimezone(timezone.utc)
    return value.isoformat().replace("+00:00", "Z")


def _from_utc_iso(value: str) -> datetime:
    if value.endswith("Z"):
        value = value[:-1] + "+00:00"
    return datetime.fromisoformat(value).astimezone(timezone.utc)


class UTCDateTime(TypeDecorator[str]):
    impl = String(32)
    cache_ok = True

    def process_bind_param(self, value: Any, dialect: Any) -> str | None:
        if value is None:
            return None
        if isinstance(value, datetime):
            return _to_utc_iso(value)
        if isinstance(value, str):
            return _to_utc_iso(datetime.fromisoformat(value.replace("Z", "+00:00")))
        raise TypeError(f"Unsupported datetime value: {type(value)!r}")

    def process_result_value(self, value: Any, dialect: Any) -> datetime | None:
        if value is None:
            return None
        return _from_utc_iso(value)


class Message(Base):
    __tablename__ = "messages"
    __table_args__ = (Index("ix_messages_source_event_id", "source_event_id", unique=True),)

    id: Mapped[int] = mapped_column(Integer, primary_key=True, autoincrement=True)
    source_event_id: Mapped[str] = mapped_column(String(128), nullable=False)
    source_type: Mapped[str] = mapped_column(String(32), nullable=False)
    title: Mapped[str] = mapped_column(String(255), nullable=False)
    body: Mapped[str] = mapped_column(Text, nullable=False)
    sent_at: Mapped[datetime] = mapped_column(UTCDateTime(), nullable=False)
    raw_payload: Mapped[str | None] = mapped_column(Text, nullable=True)
    created_at: Mapped[datetime] = mapped_column(
        UTCDateTime(),
        nullable=False,
        default=lambda: datetime.now(timezone.utc),
    )

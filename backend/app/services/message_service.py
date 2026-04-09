from __future__ import annotations

from dataclasses import dataclass
from datetime import datetime, timezone

from sqlalchemy import select
from sqlalchemy.orm import Session

from app.models import Message


@dataclass(frozen=True)
class MessageInput:
    source_event_id: str
    source_type: str
    title: str
    body: str
    sent_at: datetime
    raw_payload: str | None = None


def save_message(session: Session, message_input: MessageInput) -> Message:
    existing = session.scalar(
        select(Message).where(Message.source_event_id == message_input.source_event_id)
    )
    if existing is not None:
        return existing

    message = Message(
        source_event_id=message_input.source_event_id,
        source_type=message_input.source_type,
        title=message_input.title,
        body=message_input.body,
        sent_at=message_input.sent_at,
        raw_payload=message_input.raw_payload,
    )
    session.add(message)
    session.flush()
    return message


def normalize_cursor_timestamp(value: datetime) -> datetime:
    if value.tzinfo is None:
        return value.replace(tzinfo=timezone.utc)
    return value.astimezone(timezone.utc)


def format_cursor_timestamp(value: datetime) -> str:
    return normalize_cursor_timestamp(value).isoformat().replace("+00:00", "Z")


def list_messages_after(session: Session, since_ts: datetime | None, limit: int) -> list[Message]:
    stmt = (
        select(Message)
        .order_by(Message.sent_at.asc(), Message.id.asc())
        .limit(limit)
    )
    if since_ts is not None:
        stmt = stmt.where(Message.sent_at > normalize_cursor_timestamp(since_ts))
    return list(session.scalars(stmt).all())

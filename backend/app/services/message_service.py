from __future__ import annotations

from dataclasses import dataclass
from datetime import datetime, timezone

from collections.abc import Iterable, Sequence

from sqlalchemy import select
from sqlalchemy.orm import Session

from app.models import Message
from app.schemas import DeviceMessageFilter


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
        is_read=False,
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


def list_messages_after(
    session: Session,
    since_ts: datetime | None = None,
    limit: int = 20,
    after_id: int | None = None,
) -> list[Message]:
    stmt = select(Message).order_by(Message.sent_at.asc(), Message.id.asc()).limit(limit)
    if after_id is not None:
        stmt = stmt.where(Message.id > after_id)
    elif since_ts is not None:
        stmt = stmt.where(Message.sent_at > normalize_cursor_timestamp(since_ts))
    return list(session.scalars(stmt).all())


def list_unread_messages(session: Session, limit: int) -> list[Message]:
    stmt = (
        select(Message)
        .where(Message.is_read.is_(False))
        .order_by(Message.sent_at.asc(), Message.id.asc())
        .limit(limit)
    )
    return list(session.scalars(stmt).all())


def has_unread_messages(session: Session) -> bool:
    stmt = select(Message.id).where(Message.is_read.is_(False)).limit(1)
    return session.scalar(stmt) is not None


def claim_unread_messages(session: Session, limit: int) -> tuple[list[Message], bool]:
    session.connection().exec_driver_sql("BEGIN IMMEDIATE")
    messages = list_unread_messages(session, limit)
    mark_messages_read(session, messages)
    has_more = has_unread_messages(session)
    return messages, has_more


def list_device_messages(
    session: Session,
    message_filter: DeviceMessageFilter | str,
    limit: int,
) -> list[Message]:
    if message_filter == DeviceMessageFilter.all or message_filter == DeviceMessageFilter.all.value:
        return list_all_messages(session, limit)
    return list_unread_messages(session, limit)


def list_all_messages(session: Session, limit: int) -> list[Message]:
    stmt = (
        select(Message)
        .order_by(Message.sent_at.desc(), Message.id.desc())
        .limit(limit)
    )
    return list(session.scalars(stmt).all())


def mark_messages_read(session: Session, messages: Sequence[Message] | Iterable[Message]) -> int:
    count = 0
    for message in messages:
        if not message.is_read:
            message.is_read = True
            count += 1
    if count:
        session.flush()
    return count

from __future__ import annotations

from collections.abc import Mapping
from datetime import datetime, timezone
import json
from typing import Any

from sqlalchemy.orm import Session

from app.models import Message
from .message_service import MessageInput, save_message

SOURCE_TYPE_DINGTALK = "dingtalk"


def _as_mapping(payload: Mapping[str, Any] | dict[str, Any]) -> Mapping[str, Any]:
    if not isinstance(payload, Mapping):
        raise TypeError(f"Expected mapping payload, got {type(payload)!r}")
    return payload


def _extract_source_event_id(payload: Mapping[str, Any]) -> str:
    for key in ("msgId", "messageId", "eventId", "id"):
        value = payload.get(key)
        if value:
            return str(value)
    raise ValueError("DingTalk payload is missing a source event id")


def _extract_title(payload: Mapping[str, Any]) -> str:
    for key in ("senderNick", "senderName", "conversationTitle", "title"):
        value = payload.get(key)
        if value:
            return str(value).strip()
    return "钉钉消息"


def _extract_body(payload: Mapping[str, Any]) -> str:
    text_payload = payload.get("text")
    if isinstance(text_payload, Mapping):
        content = text_payload.get("content")
        if content:
            return str(content).strip()

    content = payload.get("content")
    if content:
        return str(content).strip()

    msgtype = payload.get("msgtype")
    if msgtype and str(msgtype).lower() != "text":
        raise ValueError("Only text DingTalk messages are supported for now")
    raise ValueError("DingTalk payload is missing text content")


def _extract_sent_at(payload: Mapping[str, Any]) -> datetime:
    for key in ("createAt", "timestamp", "sentAt"):
        value = payload.get(key)
        if value is None:
            continue
        numeric = int(value)
        if numeric > 1_000_000_000_000:
            numeric //= 1000
        return datetime.fromtimestamp(numeric, tz=timezone.utc)

    headers = payload.get("headers")
    if isinstance(headers, Mapping):
        value = headers.get("time")
        if value is not None:
            numeric = int(value)
            if numeric > 1_000_000_000_000:
                numeric //= 1000
            return datetime.fromtimestamp(numeric, tz=timezone.utc)

    return datetime.now(timezone.utc)


def normalize_dingtalk_message(payload: Mapping[str, Any] | dict[str, Any]) -> MessageInput:
    data = _as_mapping(payload)
    raw_payload = json.dumps(data, ensure_ascii=False, separators=(",", ":"))
    return MessageInput(
        source_event_id=_extract_source_event_id(data),
        source_type=SOURCE_TYPE_DINGTALK,
        title=_extract_title(data),
        body=_extract_body(data),
        sent_at=_extract_sent_at(data),
        raw_payload=raw_payload,
    )


def save_normalized_dingtalk_message(session: Session, payload: Mapping[str, Any] | dict[str, Any]) -> Message:
    return save_message(session, normalize_dingtalk_message(payload))

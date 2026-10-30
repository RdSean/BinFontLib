from __future__ import annotations

from datetime import datetime
from enum import Enum

from pydantic import BaseModel, ConfigDict


class DeviceMessageFilter(str, Enum):
    unread = "unread"
    all = "all"


class MessageRead(BaseModel):
    model_config = ConfigDict(from_attributes=True)

    id: int
    source_event_id: str
    source_type: str
    title: str
    body: str
    sent_at: datetime
    raw_payload: str | None = None
    created_at: datetime


class DeviceMessagesResponse(BaseModel):
    device_id: str
    messages: list[MessageRead]
    has_more: bool
    server_time: datetime

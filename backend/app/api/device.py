from __future__ import annotations

from datetime import datetime, timezone

from fastapi import APIRouter, Depends, HTTPException, Query, Request, status

from app.config import get_settings
from app.db import get_db_session
from app.schemas import DeviceMessagesResponse, MessageRead
from app.services.message_service import format_cursor_timestamp, list_messages_after

router = APIRouter(prefix="/api/device", tags=["device"])


def require_device_token(request: Request) -> None:
    settings = get_settings()
    expected_token = settings.device_token
    if not expected_token:
        raise HTTPException(
            status_code=status.HTTP_503_SERVICE_UNAVAILABLE,
            detail="device token not configured",
        )

    provided_token = request.headers.get("X-Device-Token", "")
    if provided_token != expected_token:
        raise HTTPException(status_code=status.HTTP_401_UNAUTHORIZED, detail="invalid device token")


@router.get("/messages", response_model=DeviceMessagesResponse)
def get_messages(
    device_id: str = Query(min_length=1),
    since_ts: datetime | None = Query(default=None),
    limit: int = Query(default=20, ge=1, le=100),
    _: None = Depends(require_device_token),
):
    with get_db_session() as session:
        messages = list_messages_after(session, since_ts=since_ts, limit=limit + 1)

    has_more = len(messages) > limit
    messages = messages[:limit]
    next_cursor = format_cursor_timestamp(messages[-1].sent_at) if messages else (
        format_cursor_timestamp(since_ts) if since_ts is not None else ""
    )
    return DeviceMessagesResponse(
        device_id=device_id,
        messages=[MessageRead.model_validate(item) for item in messages],
        next_cursor=next_cursor,
        has_more=has_more,
        server_time=datetime.now(timezone.utc),
    )

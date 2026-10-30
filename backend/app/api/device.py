from __future__ import annotations

from datetime import datetime, timezone

from fastapi import APIRouter, Depends, HTTPException, Query, Request, status

from app.config import get_settings
from app.db import get_db_session
from app.schemas import DeviceMessageFilter, DeviceMessagesResponse, MessageRead
from app.services.message_service import (
    claim_unread_messages,
    list_device_messages,
)

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
    limit: int = Query(default=20, ge=1, le=100),
    message_filter: DeviceMessageFilter = Query(default=DeviceMessageFilter.unread, alias="filter"),
    debug: int = Query(default=0, ge=0, le=1),
    _: None = Depends(require_device_token),
):
    with get_db_session() as session:
        if debug == 0 and message_filter is DeviceMessageFilter.unread:
            messages, has_more = claim_unread_messages(session, limit=limit)
            session.commit()
        else:
            effective_filter = DeviceMessageFilter.all if debug == 1 else message_filter
            requested_messages = list_device_messages(
                session,
                message_filter=effective_filter,
                limit=limit + 1,
            )
            has_more = len(requested_messages) > limit
            messages = requested_messages[:limit]

    return DeviceMessagesResponse(
        device_id=device_id,
        messages=[MessageRead.model_validate(item) for item in messages],
        has_more=has_more,
        server_time=datetime.now(timezone.utc),
    )

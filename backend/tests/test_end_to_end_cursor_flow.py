from __future__ import annotations

from datetime import datetime, timezone

import httpx
import pytest
import pytest_asyncio
from httpx import ASGITransport

from app.db import get_db_session
from app.services.dingtalk_service import save_normalized_dingtalk_message


@pytest_asyncio.fixture()
async def client(monkeypatch, tmp_path):
    database_url = f"sqlite:///{tmp_path / 'cursor.db'}"
    monkeypatch.setenv("DATABASE_URL", database_url)
    monkeypatch.setenv("DEVICE_TOKEN", "cursor-token")
    monkeypatch.setenv("DINGTALK_CLIENT_ID", "test-client-id")
    monkeypatch.setenv("DINGTALK_CLIENT_SECRET", "test-client-secret")

    from app.main import app

    async with app.router.lifespan_context(app):
        async with httpx.AsyncClient(
            transport=ASGITransport(app=app),
            base_url="http://testserver",
        ) as test_client:
            yield test_client


@pytest.mark.asyncio
async def test_cursor_flow_advances_after_timestamp_cursor_reads(client):
    first_payload = {
        "msgId": "cursor-msg-001",
        "msgtype": "text",
        "senderNick": "班主任",
        "createAt": 1744100100000,
        "text": {"content": "今天记得带雨伞。"},
    }
    second_payload = {
        "msgId": "cursor-msg-002",
        "msgtype": "text",
        "senderNick": "家长群",
        "createAt": 1744100400000,
        "text": {"content": "晚饭后请读半小时。"},
    }

    with get_db_session() as session:
        first = save_normalized_dingtalk_message(session, first_payload)
        second = save_normalized_dingtalk_message(session, second_payload)
        session.commit()

    response = await client.get(
        "/api/device/messages",
        params={"device_id": "paper-s3", "since_ts": first.sent_at.isoformat().replace("+00:00", "Z"), "limit": 10},
        headers={"X-Device-Token": "cursor-token"},
    )

    assert response.status_code == 200
    payload = response.json()
    assert payload["device_id"] == "paper-s3"
    assert [item["id"] for item in payload["messages"]] == [second.id]
    assert payload["next_cursor"] == second.sent_at.isoformat().replace("+00:00", "Z")
    assert payload["has_more"] is False
    assert isinstance(payload["server_time"], str)

    empty_response = await client.get(
        "/api/device/messages",
        params={"device_id": "paper-s3", "since_ts": payload["next_cursor"], "limit": 10},
        headers={"X-Device-Token": "cursor-token"},
    )

    assert empty_response.status_code == 200
    empty_payload = empty_response.json()
    assert empty_payload["messages"] == []
    assert empty_payload["next_cursor"] == second.sent_at.isoformat().replace("+00:00", "Z")
    assert empty_payload["has_more"] is False

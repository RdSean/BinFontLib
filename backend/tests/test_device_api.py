from __future__ import annotations

from datetime import datetime, timezone

import httpx
import pytest
import pytest_asyncio
from httpx import ASGITransport

from app.db import get_db_session
from app.services.message_service import MessageInput, save_message


@pytest_asyncio.fixture()
async def client(monkeypatch, tmp_path):
    database_url = f"sqlite:///{tmp_path / 'device.db'}"
    monkeypatch.setenv("DATABASE_URL", database_url)
    monkeypatch.setenv("DEVICE_TOKEN", "secret-token")

    from app.main import app

    async with app.router.lifespan_context(app):
        async with httpx.AsyncClient(
            transport=ASGITransport(app=app),
            base_url="http://testserver",
        ) as test_client:
            yield test_client


@pytest.mark.asyncio
async def test_get_messages_requires_device_token(client):
    response = await client.get("/api/device/messages", params={"device_id": "paper-s3"})
    assert response.status_code == 401


@pytest.mark.asyncio
async def test_get_messages_returns_incremental_messages_by_timestamp_cursor(client):
    with get_db_session() as session:
        first = save_message(
            session,
            MessageInput(
                source_event_id="evt-001",
                source_type="dingtalk",
                title="第一条",
                body="A",
                sent_at=datetime(2026, 4, 8, 8, 0, tzinfo=timezone.utc),
            ),
        )
        second = save_message(
            session,
            MessageInput(
                source_event_id="evt-002",
                source_type="dingtalk",
                title="第二条",
                body="B",
                sent_at=datetime(2026, 4, 8, 8, 1, tzinfo=timezone.utc),
            ),
        )
        session.commit()

    response = await client.get(
        "/api/device/messages",
        params={"device_id": "paper-s3", "since_ts": first.sent_at.isoformat().replace("+00:00", "Z"), "limit": 10},
        headers={"X-Device-Token": "secret-token"},
    )

    assert response.status_code == 200
    payload = response.json()
    assert payload["device_id"] == "paper-s3"
    assert [item["id"] for item in payload["messages"]] == [second.id]
    assert payload["next_cursor"] == second.sent_at.isoformat().replace("+00:00", "Z")
    assert payload["has_more"] is False
    assert isinstance(payload["server_time"], str)


@pytest.mark.asyncio
async def test_get_messages_without_cursor_returns_initial_batch_and_iso_cursor(client):
    with get_db_session() as session:
        first = save_message(
            session,
            MessageInput(
                source_event_id="evt-010",
                source_type="dingtalk",
                title="第一条",
                body="A",
                sent_at=datetime(2026, 4, 8, 9, 0, tzinfo=timezone.utc),
            ),
        )
        second = save_message(
            session,
            MessageInput(
                source_event_id="evt-011",
                source_type="dingtalk",
                title="第二条",
                body="B",
                sent_at=datetime(2026, 4, 8, 9, 1, tzinfo=timezone.utc),
            ),
        )
        session.commit()

    response = await client.get(
        "/api/device/messages",
        params={"device_id": "paper-s3", "limit": 10},
        headers={"X-Device-Token": "secret-token"},
    )

    assert response.status_code == 200
    payload = response.json()
    assert [item["id"] for item in payload["messages"]] == [first.id, second.id]
    assert payload["next_cursor"] == second.sent_at.isoformat().replace("+00:00", "Z")
    assert payload["has_more"] is False
    assert isinstance(payload["server_time"], str)

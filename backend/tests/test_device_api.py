from __future__ import annotations

from datetime import datetime, timezone

import httpx
import pytest
import pytest_asyncio
from httpx import ASGITransport

from app.db import get_db_session
from app.services.message_service import (
    MessageInput,
    list_all_messages,
    list_unread_messages,
    mark_messages_read,
    save_message,
)


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
async def test_get_messages_defaults_to_unread_and_marks_messages_read(client):
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
        params={"device_id": "paper-s3", "limit": 10},
        headers={"X-Device-Token": "secret-token"},
    )

    assert response.status_code == 200
    payload = response.json()
    assert payload["device_id"] == "paper-s3"
    assert [item["id"] for item in payload["messages"]] == [first.id, second.id]
    assert payload["has_more"] is False
    assert isinstance(payload["server_time"], str)

    with get_db_session() as session:
        unread = list_unread_messages(session, limit=10)
        assert unread == []
        all_messages = list_all_messages(session, limit=10)
        assert [item.is_read for item in all_messages] == [True, True]


@pytest.mark.asyncio
async def test_get_messages_unread_limit_does_not_consume_extra_message(client):
    with get_db_session() as session:
        first = save_message(
            session,
            MessageInput(
                source_event_id="evt-003",
                source_type="dingtalk",
                title="第一条",
                body="A",
                sent_at=datetime(2026, 4, 8, 8, 10, tzinfo=timezone.utc),
            ),
        )
        second = save_message(
            session,
            MessageInput(
                source_event_id="evt-004",
                source_type="dingtalk",
                title="第二条",
                body="B",
                sent_at=datetime(2026, 4, 8, 8, 11, tzinfo=timezone.utc),
            ),
        )
        session.commit()

    first_response = await client.get(
        "/api/device/messages",
        params={"device_id": "paper-s3", "limit": 1},
        headers={"X-Device-Token": "secret-token"},
    )

    assert first_response.status_code == 200
    first_payload = first_response.json()
    assert [item["id"] for item in first_payload["messages"]] == [first.id]
    assert first_payload["has_more"] is True

    second_response = await client.get(
        "/api/device/messages",
        params={"device_id": "paper-s3", "limit": 1},
        headers={"X-Device-Token": "secret-token"},
    )

    assert second_response.status_code == 200
    second_payload = second_response.json()
    assert [item["id"] for item in second_payload["messages"]] == [second.id]
    assert second_payload["has_more"] is False


@pytest.mark.asyncio
async def test_get_messages_filter_all_returns_newest_without_mutating_read_state(client):
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
        third = save_message(
            session,
            MessageInput(
                source_event_id="evt-012",
                source_type="dingtalk",
                title="第三条",
                body="C",
                sent_at=datetime(2026, 4, 8, 9, 2, tzinfo=timezone.utc),
            ),
        )
        mark_messages_read(session, [first])
        session.commit()

    response = await client.get(
        "/api/device/messages",
        params={"device_id": "paper-s3", "limit": 2, "filter": "all"},
        headers={"X-Device-Token": "secret-token"},
    )

    assert response.status_code == 200
    payload = response.json()
    assert [item["id"] for item in payload["messages"]] == [third.id, second.id]
    assert payload["has_more"] is True

    with get_db_session() as session:
        all_messages = list_all_messages(session, limit=10)
        assert [item.is_read for item in all_messages] == [False, False, True]


@pytest.mark.asyncio
async def test_get_messages_debug_mode_does_not_mutate_read_state(client):
    with get_db_session() as session:
        first = save_message(
            session,
            MessageInput(
                source_event_id="evt-020",
                source_type="dingtalk",
                title="第一条",
                body="A",
                sent_at=datetime(2026, 4, 8, 10, 0, tzinfo=timezone.utc),
            ),
        )
        second = save_message(
            session,
            MessageInput(
                source_event_id="evt-021",
                source_type="dingtalk",
                title="第二条",
                body="B",
                sent_at=datetime(2026, 4, 8, 10, 1, tzinfo=timezone.utc),
            ),
        )
        mark_messages_read(session, [first])
        session.commit()

    response = await client.get(
        "/api/device/messages",
        params={"device_id": "paper-s3", "limit": 2, "debug": 1},
        headers={"X-Device-Token": "secret-token"},
    )

    assert response.status_code == 200
    payload = response.json()
    assert [item["id"] for item in payload["messages"]] == [second.id, first.id]

    with get_db_session() as session:
        all_messages = list_all_messages(session, limit=10)
        assert [item.is_read for item in all_messages] == [False, True]

from __future__ import annotations

import httpx
import pytest
import pytest_asyncio
from httpx import ASGITransport

from app.db import get_db_session
from app.services.dingtalk_service import save_normalized_dingtalk_message
from app.services.message_service import list_all_messages, list_unread_messages


@pytest_asyncio.fixture()
async def client(monkeypatch, tmp_path):
    database_url = f"sqlite:///{tmp_path / 'queue.db'}"
    monkeypatch.setenv("DATABASE_URL", database_url)
    monkeypatch.setenv("DEVICE_TOKEN", "queue-token")
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
async def test_device_default_filter_returns_oldest_unread_and_marks_them_read(client):
    first_payload = {
        "msgId": "queue-msg-001",
        "msgtype": "text",
        "senderNick": "班主任",
        "createAt": 1744100100000,
        "text": {"content": "今天记得带雨伞。"},
    }
    second_payload = {
        "msgId": "queue-msg-002",
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
        params={"device_id": "paper-s3", "limit": 10},
        headers={"X-Device-Token": "queue-token"},
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
async def test_device_filter_all_returns_newest_without_mutating_read_state(client):
    first_payload = {
        "msgId": "queue-msg-010",
        "msgtype": "text",
        "senderNick": "班主任",
        "createAt": 1744101000000,
        "text": {"content": "第一条。"},
    }
    second_payload = {
        "msgId": "queue-msg-011",
        "msgtype": "text",
        "senderNick": "家长群",
        "createAt": 1744101300000,
        "text": {"content": "第二条。"},
    }
    third_payload = {
        "msgId": "queue-msg-012",
        "msgtype": "text",
        "senderNick": "办公室",
        "createAt": 1744101600000,
        "text": {"content": "第三条。"},
    }

    with get_db_session() as session:
        first = save_normalized_dingtalk_message(session, first_payload)
        second = save_normalized_dingtalk_message(session, second_payload)
        third = save_normalized_dingtalk_message(session, third_payload)
        session.commit()

    response = await client.get(
        "/api/device/messages",
        params={"device_id": "paper-s3", "limit": 2, "filter": "all"},
        headers={"X-Device-Token": "queue-token"},
    )

    assert response.status_code == 200
    payload = response.json()
    assert [item["id"] for item in payload["messages"]] == [third.id, second.id]
    assert payload["has_more"] is True
    assert isinstance(payload["server_time"], str)

    with get_db_session() as session:
        all_messages = list_all_messages(session, limit=10)
        assert [item.is_read for item in all_messages] == [False, False, False]


@pytest.mark.asyncio
async def test_device_debug_mode_does_not_mutate_read_state(client):
    first_payload = {
        "msgId": "queue-msg-020",
        "msgtype": "text",
        "senderNick": "班主任",
        "createAt": 1744101900000,
        "text": {"content": "第一条。"},
    }
    second_payload = {
        "msgId": "queue-msg-021",
        "msgtype": "text",
        "senderNick": "家长群",
        "createAt": 1744102200000,
        "text": {"content": "第二条。"},
    }

    with get_db_session() as session:
        first = save_normalized_dingtalk_message(session, first_payload)
        second = save_normalized_dingtalk_message(session, second_payload)
        session.commit()

    response = await client.get(
        "/api/device/messages",
        params={"device_id": "paper-s3", "limit": 2, "debug": 1},
        headers={"X-Device-Token": "queue-token"},
    )

    assert response.status_code == 200
    payload = response.json()
    assert [item["id"] for item in payload["messages"]] == [second.id, first.id]

    with get_db_session() as session:
        all_messages = list_all_messages(session, limit=10)
        assert [item.is_read for item in all_messages] == [False, False]

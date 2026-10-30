from __future__ import annotations

from datetime import datetime, timezone

import pytest

from app.db import Base, get_engine, get_db_session
from app.services.message_service import (
    MessageInput,
    list_all_messages,
    list_messages_after,
    list_unread_messages,
    mark_messages_read,
    save_message,
)


@pytest.fixture()
def temp_db(monkeypatch, tmp_path):
    database_url = f"sqlite:///{tmp_path / 'app.db'}"
    monkeypatch.setenv("DATABASE_URL", database_url)
    engine = get_engine(database_url)
    Base.metadata.create_all(bind=engine)
    yield


def test_save_message_deduplicates_source_event_id(temp_db):
    message = MessageInput(
        source_event_id="evt-001",
        source_type="dingtalk",
        title="家长提醒",
        body="今天记得带雨伞。",
        sent_at=datetime(2026, 4, 8, 8, 30, tzinfo=timezone.utc),
        raw_payload='{"text":"今天记得带雨伞。"}',
    )

    first = None
    second = None
    with get_db_session() as session:
        first = save_message(session, message)
        second = save_message(session, message)
        session.commit()

    assert first.id == second.id
    assert first.source_event_id == "evt-001"


def test_list_messages_after_returns_newer_messages(temp_db):
    with get_db_session() as session:
        save_message(
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

    with get_db_session() as session:
        messages = list_messages_after(session, after_id=1, limit=10)

    assert [item.id for item in messages] == [second.id]


def test_message_defaults_to_unread(temp_db):
    with get_db_session() as session:
        message = save_message(
            session,
            MessageInput(
                source_event_id="evt-100",
                source_type="dingtalk",
                title="第一条",
                body="A",
                sent_at=datetime(2026, 4, 8, 10, 0, tzinfo=timezone.utc),
            ),
        )
        session.commit()

    assert message.is_read is False


def test_unread_query_returns_oldest_first_and_can_mark_batch_read(temp_db):
    with get_db_session() as session:
        first = save_message(
            session,
            MessageInput(
                source_event_id="evt-101",
                source_type="dingtalk",
                title="第一条",
                body="A",
                sent_at=datetime(2026, 4, 8, 10, 0, tzinfo=timezone.utc),
            ),
        )
        second = save_message(
            session,
            MessageInput(
                source_event_id="evt-102",
                source_type="dingtalk",
                title="第二条",
                body="B",
                sent_at=datetime(2026, 4, 8, 10, 1, tzinfo=timezone.utc),
            ),
        )
        session.commit()

    with get_db_session() as session:
        unread = list_unread_messages(session, limit=10)
        assert [item.id for item in unread] == [first.id, second.id]
        assert [item.is_read for item in unread] == [False, False]
        mark_messages_read(session, unread)
        session.commit()

    with get_db_session() as session:
        unread = list_unread_messages(session, limit=10)
        assert unread == []


def test_all_query_returns_newest_first_without_changing_read_state(temp_db):
    with get_db_session() as session:
        first = save_message(
            session,
            MessageInput(
                source_event_id="evt-201",
                source_type="dingtalk",
                title="第一条",
                body="A",
                sent_at=datetime(2026, 4, 8, 11, 0, tzinfo=timezone.utc),
            ),
        )
        second = save_message(
            session,
            MessageInput(
                source_event_id="evt-202",
                source_type="dingtalk",
                title="第二条",
                body="B",
                sent_at=datetime(2026, 4, 8, 11, 1, tzinfo=timezone.utc),
            ),
        )
        third = save_message(
            session,
            MessageInput(
                source_event_id="evt-203",
                source_type="dingtalk",
                title="第三条",
                body="C",
                sent_at=datetime(2026, 4, 8, 11, 2, tzinfo=timezone.utc),
            ),
        )
        mark_messages_read(session, [first])
        session.commit()

    with get_db_session() as session:
        all_messages = list_all_messages(session, limit=10)
        assert [item.id for item in all_messages] == [third.id, second.id, first.id]
        assert [item.is_read for item in all_messages] == [False, False, True]

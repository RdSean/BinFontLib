from __future__ import annotations

from datetime import datetime, timezone

import pytest

from app.db import Base, get_engine, get_db_session
from app.models import Message
from app.services.message_service import MessageInput, list_messages_after, save_message


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

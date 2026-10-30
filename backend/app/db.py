from __future__ import annotations

from contextlib import contextmanager
from functools import lru_cache
from pathlib import Path
from typing import Iterator

from sqlalchemy import create_engine, inspect, text
from sqlalchemy.orm import DeclarativeBase, Session, sessionmaker

from .config import get_settings


class Base(DeclarativeBase):
    pass


@lru_cache(maxsize=None)
def get_engine(database_url: str):
    if database_url.startswith("sqlite:///") and database_url != "sqlite:///:memory:":
        db_path = database_url.removeprefix("sqlite:///")
        Path(db_path).expanduser().resolve().parent.mkdir(parents=True, exist_ok=True)
    return create_engine(
        database_url,
        connect_args={"check_same_thread": False}
        if database_url.startswith("sqlite")
        else {},
        future=True,
    )


def get_engine_for_current_settings():
    settings = get_settings()
    return get_engine(settings.database_url)


@lru_cache(maxsize=None)
def get_session_factory(database_url: str):
    return sessionmaker(
        bind=get_engine(database_url),
        autoflush=False,
        autocommit=False,
        expire_on_commit=False,
        future=True,
    )


def get_session_factory_for_current_settings():
    settings = get_settings()
    return get_session_factory(settings.database_url)


@contextmanager
def get_db_session() -> Iterator[Session]:
    session_factory = get_session_factory_for_current_settings()
    session = session_factory()
    try:
        yield session
    except Exception:
        session.rollback()
        raise
    finally:
        session.close()


def create_tables() -> None:
    from . import models  # noqa: F401

    engine = get_engine_for_current_settings()
    Base.metadata.create_all(bind=engine)
    _ensure_sqlite_message_schema(engine)


def _ensure_sqlite_message_schema(engine) -> None:
    if engine.dialect.name != "sqlite":
        return

    inspector = inspect(engine)
    if "messages" not in inspector.get_table_names():
        return

    columns = {column["name"] for column in inspector.get_columns("messages")}
    with engine.begin() as conn:
        if "is_read" not in columns:
            conn.execute(text("ALTER TABLE messages ADD COLUMN is_read BOOLEAN NOT NULL DEFAULT 0"))
        conn.execute(
            text(
                "CREATE UNIQUE INDEX IF NOT EXISTS ix_messages_source_event_id "
                "ON messages (source_event_id)"
            )
        )
        conn.execute(
            text(
                "CREATE INDEX IF NOT EXISTS ix_messages_unread_sent_at_id "
                "ON messages (sent_at, id) WHERE is_read = 0"
            )
        )

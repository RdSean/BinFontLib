from __future__ import annotations

from contextlib import contextmanager
from functools import lru_cache
from pathlib import Path
from typing import Iterator

from sqlalchemy import create_engine
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

    Base.metadata.create_all(bind=get_engine_for_current_settings())

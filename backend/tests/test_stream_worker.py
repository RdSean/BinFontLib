from __future__ import annotations

import asyncio
from types import SimpleNamespace

import pytest

from run_stream_worker import build_handler


class FakeChatbotHandler:
    def __init__(self):
        pass


class FakeAckMessage:
    STATUS_OK = "OK"
    STATUS_BAD_REQUEST = "BAD_REQUEST"
    STATUS_SYSTEM_EXCEPTION = "SYSTEM_EXCEPTION"


class FakeDingTalkStream:
    ChatbotHandler = FakeChatbotHandler
    AckMessage = FakeAckMessage


def build_test_handler():
    return build_handler(__import__("logging").getLogger("test"), FakeDingTalkStream)


@pytest.mark.asyncio
async def test_process_returns_bad_request_for_invalid_json(caplog):
    handler = build_test_handler()

    result = await handler.process(SimpleNamespace(data="{not json"))

    assert result == (FakeAckMessage.STATUS_BAD_REQUEST, "invalid payload")
    assert "failed to parse DingTalk payload" in caplog.text


@pytest.mark.asyncio
async def test_process_returns_system_exception_when_persistence_fails(monkeypatch, caplog):
    handler = build_test_handler()

    def boom(*args, **kwargs):
        raise RuntimeError("boom")

    monkeypatch.setattr("run_stream_worker.save_normalized_dingtalk_message", boom)

    result = await handler.process(
        SimpleNamespace(
            data={
                "msgId": "evt-001",
                "msgtype": "text",
                "senderNick": "班主任",
                "createAt": 1744100100000,
                "text": {"content": "今天记得带雨伞。"},
            }
        )
    )

    assert result == (FakeAckMessage.STATUS_SYSTEM_EXCEPTION, "processing failed")
    assert "failed to process DingTalk payload" in caplog.text

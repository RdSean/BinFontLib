from __future__ import annotations

from datetime import datetime, timezone


def test_normalize_dingtalk_text_message():
    from app.services.dingtalk_service import normalize_dingtalk_message

    payload = {
        "msgId": "dingtalk-msg-001",
        "msgtype": "text",
        "senderNick": "班主任",
        "createAt": 1744100100000,
        "text": {
            "content": "今天记得带雨伞。  ",
        },
    }

    message = normalize_dingtalk_message(payload)

    assert message.source_event_id == "dingtalk-msg-001"
    assert message.source_type == "dingtalk"
    assert message.title == "班主任"
    assert message.body == "今天记得带雨伞。"
    assert message.sent_at == datetime(2025, 4, 8, 8, 15, tzinfo=timezone.utc)

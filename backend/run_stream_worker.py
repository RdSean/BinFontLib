from __future__ import annotations

import json
import logging

from app.db import create_tables, get_db_session
from app.integrations.dingtalk_stream_client import build_dingtalk_stream_client, load_dingtalk_stream_sdk
from app.services.dingtalk_service import save_normalized_dingtalk_message
from app.config import get_settings


def setup_logger() -> logging.Logger:
    logger = logging.getLogger("papers3-dingtalk-stream-worker")
    handler = logging.StreamHandler()
    handler.setFormatter(
        logging.Formatter("%(asctime)s %(name)s %(levelname)s %(message)s [%(filename)s:%(lineno)d]")
    )
    logger.handlers.clear()
    logger.addHandler(handler)
    logger.setLevel(logging.INFO)
    return logger


def build_handler(logger: logging.Logger, dingtalk_stream):
    class PaperS3DingTalkHandler(dingtalk_stream.ChatbotHandler):
        def __init__(self):
            super().__init__()
            self.logger = logger

        async def process(self, callback):  # pragma: no cover - exercised at runtime with real SDK
            payload = callback.data
            event_type = type(payload).__name__
            event_id = "unknown"
            try:
                if isinstance(payload, str):
                    payload = json.loads(payload)
                if isinstance(payload, dict):
                    event_id = str(payload.get("msgId") or payload.get("messageId") or payload.get("eventId") or "unknown")

                with get_db_session() as session:
                    message = save_normalized_dingtalk_message(session, payload)
                    session.commit()

                self.logger.info("saved message id=%s event=%s", message.id, message.source_event_id)
                return dingtalk_stream.AckMessage.STATUS_OK, "OK"
            except json.JSONDecodeError:
                self.logger.warning("failed to parse DingTalk payload type=%s event=%s", event_type, event_id)
                return dingtalk_stream.AckMessage.STATUS_BAD_REQUEST, "invalid payload"
            except Exception:
                self.logger.exception("failed to process DingTalk payload type=%s event=%s", event_type, event_id)
                return dingtalk_stream.AckMessage.STATUS_SYSTEM_EXCEPTION, "processing failed"

    return PaperS3DingTalkHandler()


def main() -> None:
    logger = setup_logger()
    settings = get_settings()
    logger.info("starting DingTalk stream worker for database=%s", settings.database_url)
    create_tables()

    dingtalk_stream = load_dingtalk_stream_sdk()
    client = build_dingtalk_stream_client()
    client.register_callback_handler(
        dingtalk_stream.chatbot.ChatbotMessage.TOPIC,
        build_handler(logger, dingtalk_stream),
    )
    client.start_forever()


if __name__ == "__main__":
    main()

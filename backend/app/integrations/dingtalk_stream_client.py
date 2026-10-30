from __future__ import annotations

from functools import lru_cache

from app.config import get_settings


@lru_cache(maxsize=1)
def load_dingtalk_stream_sdk():
    try:
        import dingtalk_stream
    except ImportError as exc:  # pragma: no cover - exercised in real worker runtime
        raise RuntimeError("dingtalk-stream is required to run the Stream worker") from exc
    return dingtalk_stream


def build_dingtalk_stream_client():
    settings = get_settings()
    if not settings.dingtalk_client_id or not settings.dingtalk_client_secret:
        raise RuntimeError("DingTalk client id/secret are not configured")

    dingtalk_stream = load_dingtalk_stream_sdk()
    credential = dingtalk_stream.Credential(
        settings.dingtalk_client_id,
        settings.dingtalk_client_secret,
    )
    return dingtalk_stream.DingTalkStreamClient(credential)

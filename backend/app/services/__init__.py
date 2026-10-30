"""Backend services for PaperS3 DingTalk relay."""

from .dingtalk_service import (
    SOURCE_TYPE_DINGTALK,
    normalize_dingtalk_message,
    save_normalized_dingtalk_message,
)
from .message_service import (
    MessageInput,
    list_all_messages,
    list_device_messages,
    list_messages_after,
    list_unread_messages,
    mark_messages_read,
    save_message,
)

__all__ = [
    "MessageInput",
    "list_all_messages",
    "list_device_messages",
    "SOURCE_TYPE_DINGTALK",
    "list_messages_after",
    "list_unread_messages",
    "mark_messages_read",
    "normalize_dingtalk_message",
    "save_message",
    "save_normalized_dingtalk_message",
]

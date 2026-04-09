from dataclasses import dataclass
import os
from functools import lru_cache
from pathlib import Path


_LOCAL_ENV_FILE = Path(__file__).resolve().parents[1] / ".env.dingtalk"


@dataclass(frozen=True)
class Settings:
    app_name: str = "PaperS3 DingTalk Backend"
    device_token: str = ""
    database_url: str = ""
    dingtalk_client_id: str = ""
    dingtalk_client_secret: str = ""


def get_settings() -> Settings:
    _load_local_env_file()
    database_url = os.getenv("DATABASE_URL", "").strip()
    if not database_url:
        raise RuntimeError("DATABASE_URL is required")
    return Settings(
        device_token=os.getenv("DEVICE_TOKEN", ""),
        database_url=database_url,
        dingtalk_client_id=os.getenv("DINGTALK_CLIENT_ID", os.getenv("Dingtalk_Client_ID", "")),
        dingtalk_client_secret=os.getenv(
            "DINGTALK_CLIENT_SECRET",
            os.getenv("Dingtalk_Client_Secret", ""),
        ),
    )


@lru_cache(maxsize=1)
def _load_local_env_file() -> None:
    if not _LOCAL_ENV_FILE.exists():
        return

    for raw_line in _LOCAL_ENV_FILE.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        key = key.strip()
        value = value.strip().strip("'").strip('"')
        normalized_key = key.upper()
        os.environ.setdefault(normalized_key, value)
        os.environ.setdefault(key, value)

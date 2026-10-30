from contextlib import asynccontextmanager

from fastapi import FastAPI

from .api import device_router
from .db import create_tables
from .config import get_settings


@asynccontextmanager
async def lifespan(app: FastAPI):
    create_tables()
    yield


app = FastAPI(title="PaperS3 DingTalk Backend", version="0.1.0", lifespan=lifespan)
app.include_router(device_router)


@app.get("/healthz")
def healthz() -> dict[str, object]:
    settings = get_settings()
    return {
        "status": "ok",
        "service": settings.app_name,
        "device_token_configured": bool(settings.device_token),
    }

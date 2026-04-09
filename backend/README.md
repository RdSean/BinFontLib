# PaperS3 DingTalk Backend

Minimal FastAPI backend for the PaperS3 DingTalk Stream relay.

## Local Setup

```bash
cd backend
python3 -m venv .venv
source .venv/bin/activate
pip install -e .
cp .env.example .env
```

## Run the API

```bash
cd backend
source .venv/bin/activate
uvicorn app.main:app --reload
```

## Run The Stream Worker

```bash
cd backend
source .venv/bin/activate
python run_stream_worker.py
```

## Dev Control Script

Use the single control script for everyday local debugging:

```bash
cd backend
./dev.sh
./dev.sh start
./dev.sh stop
./dev.sh restart
./dev.sh status
./dev.sh logs
./dev.sh logs api
./dev.sh logs worker
```

Notes:

- No argument defaults to `start`
- Runtime files are stored under `backend/run/`
- The script expects `.venv` and `.env.dingtalk`
- The worker is started with `NO_PROXY='*'` to avoid local proxy interference with DingTalk Stream

## Health Check

```bash
curl http://127.0.0.1:8000/healthz
```

## Local Config Files

- `backend/.env` is a convenient local override file for standard environment variables.
- `backend/.env.dingtalk` is also supported for DingTalk Stream credentials.
- The loader accepts both `DINGTALK_CLIENT_ID` / `DINGTALK_CLIENT_SECRET` and the local file keys `Dingtalk_Client_ID` / `Dingtalk_Client_Secret`.

## Environment Variables

- `DEVICE_TOKEN`
- `DATABASE_URL`
- `DINGTALK_CLIENT_ID`
- `DINGTALK_CLIENT_SECRET`

# Backend FRPC Exposure Design

**Goal:** Expose the local backend API on port `8000` through an `frpc` TCP proxy so the service can be reached from outside the local network during development.

## Architecture

`backend/dev.sh` remains the single entrypoint for local development. On `start`, it will verify the Python virtualenv, DingTalk env file, `frpc` binary, and `frpc` config file, then launch three processes:

- FastAPI on local port `8000`
- DingTalk stream worker
- `frpc` with a local TCP proxy targeting `127.0.0.1:8000`

The script will also manage PID files, logs, stop/restart/status commands, and expose `frpc` logs alongside the existing API and worker logs.

## Config File

A new `backend/frpc.toml` template will be committed with placeholder values for:

- `serverAddr`
- `serverPort`
- `auth.token`
- `proxies[].remotePort`

The local target will stay fixed at `127.0.0.1:8000`. This keeps the file simple and lets the user replace only the FRPS-specific fields.

## Error Handling

If `frpc` is missing, the config file is missing, or `frpc verify` fails, `dev.sh start` should fail early. If `frpc` fails after API and worker have already started, the script should stop the partially started local services instead of leaving the environment half-running.

## Testing

Add a targeted pytest for `backend/dev.sh` that runs the script in an isolated temporary directory with fake `python` and `frpc` executables. The test will assert:

- `start` creates and tracks the `frpc` process
- `status` reports `frpc`
- `stop` shuts it down cleanly

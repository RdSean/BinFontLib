#!/bin/zsh
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
RUN_DIR="$ROOT_DIR/run"
VENV_PYTHON="$ROOT_DIR/.venv/bin/python"
ENV_FILE="$ROOT_DIR/.env.dingtalk"
API_PID_FILE="$RUN_DIR/api.pid"
WORKER_PID_FILE="$RUN_DIR/worker.pid"
API_LOG_FILE="$RUN_DIR/api.log"
WORKER_LOG_FILE="$RUN_DIR/worker.log"
API_PORT=8000

mkdir -p "$RUN_DIR"

command_name="${1:-start}"
log_target="${2:-all}"

is_running() {
  local pid_file="$1"
  [[ -f "$pid_file" ]] || return 1
  local pid
  pid="$(cat "$pid_file" 2>/dev/null || true)"
  [[ -n "$pid" ]] || return 1
  kill -0 "$pid" 2>/dev/null
}

cleanup_pid_file() {
  local pid_file="$1"
  if ! is_running "$pid_file"; then
    rm -f "$pid_file"
  fi
}

require_file() {
  local path="$1"
  local label="$2"
  if [[ ! -e "$path" ]]; then
    echo "Missing $label: $path" >&2
    exit 1
  fi
}

ensure_ready() {
  require_file "$VENV_PYTHON" "virtualenv python"
  require_file "$ENV_FILE" "config file"
  cleanup_pid_file "$API_PID_FILE"
  cleanup_pid_file "$WORKER_PID_FILE"
}

check_port() {
  if lsof -iTCP:"$API_PORT" -sTCP:LISTEN -n -P >/dev/null 2>&1; then
    if ! is_running "$API_PID_FILE"; then
      echo "Port $API_PORT is already in use by another process." >&2
      exit 1
    fi
  fi
}

start_api() {
  if is_running "$API_PID_FILE"; then
    echo "API already running (pid $(cat "$API_PID_FILE"))."
    return
  fi

  (
    cd "$ROOT_DIR"
    NO_PROXY='*' no_proxy='*' "$VENV_PYTHON" -m uvicorn app.main:app --host 0.0.0.0 --port "$API_PORT" >>"$API_LOG_FILE" 2>&1
  ) &
  echo $! > "$API_PID_FILE"
  sleep 1
  if ! is_running "$API_PID_FILE"; then
    echo "Failed to start API. Check $API_LOG_FILE" >&2
    exit 1
  fi
  echo "API started (pid $(cat "$API_PID_FILE"))."
}

start_worker() {
  if is_running "$WORKER_PID_FILE"; then
    echo "Worker already running (pid $(cat "$WORKER_PID_FILE"))."
    return
  fi

  (
    cd "$ROOT_DIR/.."
    NO_PROXY='*' no_proxy='*' "$VENV_PYTHON" backend/run_stream_worker.py >>"$WORKER_LOG_FILE" 2>&1
  ) &
  echo $! > "$WORKER_PID_FILE"
  sleep 1
  if ! is_running "$WORKER_PID_FILE"; then
    echo "Failed to start worker. Check $WORKER_LOG_FILE" >&2
    exit 1
  fi
  echo "Worker started (pid $(cat "$WORKER_PID_FILE"))."
}

stop_one() {
  local name="$1"
  local pid_file="$2"
  cleanup_pid_file "$pid_file"
  if ! [[ -f "$pid_file" ]]; then
    echo "$name is not running."
    return
  fi
  local pid
  pid="$(cat "$pid_file")"
  kill "$pid" 2>/dev/null || true
  sleep 1
  if kill -0 "$pid" 2>/dev/null; then
    kill -9 "$pid" 2>/dev/null || true
  fi
  rm -f "$pid_file"
  echo "$name stopped."
}

show_status() {
  cleanup_pid_file "$API_PID_FILE"
  cleanup_pid_file "$WORKER_PID_FILE"

  if is_running "$API_PID_FILE"; then
    echo "api: running (pid $(cat "$API_PID_FILE")) log=$API_LOG_FILE"
  else
    echo "api: stopped"
  fi

  if is_running "$WORKER_PID_FILE"; then
    echo "worker: running (pid $(cat "$WORKER_PID_FILE")) log=$WORKER_LOG_FILE"
  else
    echo "worker: stopped"
  fi
}

show_logs() {
  case "$log_target" in
    api)
      touch "$API_LOG_FILE"
      tail -f "$API_LOG_FILE"
      ;;
    worker)
      touch "$WORKER_LOG_FILE"
      tail -f "$WORKER_LOG_FILE"
      ;;
    all)
      touch "$API_LOG_FILE" "$WORKER_LOG_FILE"
      tail -f "$API_LOG_FILE" "$WORKER_LOG_FILE"
      ;;
    *)
      echo "Unknown logs target: $log_target" >&2
      exit 1
      ;;
  esac
}

case "$command_name" in
  start)
    ensure_ready
    check_port
    start_api
    start_worker
    show_status
    ;;
  stop)
    stop_one "worker" "$WORKER_PID_FILE"
    stop_one "api" "$API_PID_FILE"
    ;;
  restart)
    "$0" stop
    "$0" start
    ;;
  status)
    show_status
    ;;
  logs)
    show_logs
    ;;
  *)
    echo "Usage: backend/dev.sh [start|stop|restart|status|logs [api|worker]]" >&2
    exit 1
    ;;
esac

from __future__ import annotations

import os
import shutil
import stat
import subprocess
from pathlib import Path


def _write_executable(path: Path, content: str) -> None:
    path.write_text(content, encoding="utf-8")
    path.chmod(path.stat().st_mode | stat.S_IXUSR)


def test_dev_script_manages_frpc_lifecycle(tmp_path: Path) -> None:
    repo_root = Path(__file__).resolve().parents[2]
    source_script = repo_root / "backend" / "dev.sh"

    backend_dir = tmp_path / "backend"
    backend_dir.mkdir()
    run_dir = backend_dir / "run"
    run_dir.mkdir()

    script_path = backend_dir / "dev.sh"
    shutil.copy2(source_script, script_path)
    script_path.chmod(script_path.stat().st_mode | stat.S_IXUSR)

    (backend_dir / ".env.dingtalk").write_text("DEVICE_TOKEN=test\n", encoding="utf-8")
    (backend_dir / "frpc.toml").write_text(
        'serverAddr = "example.com"\nserverPort = 7000\n\n[[proxies]]\n'
        'name = "backend"\ntype = "tcp"\nlocalIP = "127.0.0.1"\n'
        "localPort = 8000\nremotePort = 18000\n",
        encoding="utf-8",
    )

    fake_bin = tmp_path / "fake-bin"
    fake_bin.mkdir()

    _write_executable(
        fake_bin / "frpc",
        """#!/bin/sh
if [ "$1" = "verify" ]; then
  exit 0
fi
sleep 300
""",
    )
    _write_executable(
        fake_bin / "lsof",
        """#!/bin/sh
exit 1
""",
    )

    venv_bin = backend_dir / ".venv" / "bin"
    venv_bin.mkdir(parents=True)
    _write_executable(
        venv_bin / "python",
        """#!/bin/sh
sleep 300
""",
    )

    env = os.environ.copy()
    env["PATH"] = f"{fake_bin}:{env['PATH']}"

    try:
        start = subprocess.run(
            ["zsh", str(script_path), "start"],
            cwd=backend_dir,
            env=env,
            capture_output=True,
            text=True,
            check=True,
            timeout=10,
        )

        assert (run_dir / "api.pid").exists()
        assert (run_dir / "worker.pid").exists()
        assert (run_dir / "frpc.pid").exists()
        assert "frpc: running" in start.stdout

        status = subprocess.run(
            ["zsh", str(script_path), "status"],
            cwd=backend_dir,
            env=env,
            capture_output=True,
            text=True,
            check=True,
            timeout=10,
        )

        assert "frpc: running" in status.stdout
    finally:
        subprocess.run(
            ["zsh", str(script_path), "stop"],
            cwd=backend_dir,
            env=env,
            capture_output=True,
            text=True,
            timeout=10,
            check=False,
        )

    assert not (run_dir / "api.pid").exists()
    assert not (run_dir / "worker.pid").exists()
    assert not (run_dir / "frpc.pid").exists()

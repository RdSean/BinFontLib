from __future__ import annotations

import subprocess
from pathlib import Path
import textwrap


def test_papers3_edc_poll_defaults_and_migration(tmp_path: Path) -> None:
    repo_root = Path(__file__).resolve().parents[1]
    model_header = repo_root / "examples" / "PaperS3EDC" / "PaperS3EDCModel.h"
    helper_header = repo_root / "examples" / "PaperS3EDC" / "PaperS3EDCPollPolicy.h"

    model_text = model_header.read_text(encoding="utf-8")

    assert "pollIntervalChargingSec = 2" in model_text
    assert "pollIntervalBatterySec = 5" in model_text
    assert helper_header.exists(), "expected poll policy helper to exist"

    harness = tmp_path / "poll_policy_harness.cpp"
    harness.write_text(
        textwrap.dedent(
            """
            #include <cstdint>
            #include "examples/PaperS3EDC/PaperS3EDCPollPolicy.h"

            int main() {
                using namespace papers3edc;

                uint32_t charging = 4;
                uint32_t battery = 0;
                bool changed = normalizePollIntervals(charging, battery);
                if (!changed || charging != 2 || battery != 5) {
                    return 1;
                }

                charging = 2;
                battery = 10;
                changed = normalizePollIntervals(charging, battery);
                if (!changed || charging != 2 || battery != 5) {
                    return 2;
                }

                charging = 6;
                battery = 8;
                changed = normalizePollIntervals(charging, battery);
                if (changed || charging != 6 || battery != 8) {
                    return 3;
                }

                return 0;
            }
            """
        ),
        encoding="utf-8",
    )

    executable = tmp_path / "poll_policy_harness"
    compile_result = subprocess.run(
        ["c++", "-std=c++17", "-I", str(repo_root), str(harness), "-o", str(executable)],
        capture_output=True,
        text=True,
        check=False,
    )
    assert compile_result.returncode == 0, compile_result.stderr

    run_result = subprocess.run(
        [str(executable)],
        capture_output=True,
        text=True,
        check=False,
    )
    assert run_result.returncode == 0, run_result.stderr

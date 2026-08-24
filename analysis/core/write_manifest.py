"""Write reproducibility metadata beside generated simulation results."""

from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUTPUT = ROOT / "simulations/results/experiment_manifest.json"


def command_output(*command: str) -> str:
    try:
        completed = subprocess.run(
            command, cwd=ROOT, check=True, capture_output=True, text=True
        )
    except (FileNotFoundError, subprocess.CalledProcessError):
        return "unavailable"
    lines = (completed.stdout or completed.stderr).strip().splitlines()
    return lines[0] if lines else ""


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(65536), b""):
            digest.update(chunk)
    return digest.hexdigest()


def omnetpp_version() -> str:
    try:
        completed = subprocess.run(
            ("opp_run", "-h"), cwd=ROOT, check=True,
            capture_output=True, text=True,
        )
    except (FileNotFoundError, subprocess.CalledProcessError):
        return "unavailable"
    lines = (completed.stdout or completed.stderr).splitlines()
    return next((line.strip() for line in lines if line.startswith("Version:")),
                "unavailable")


def main() -> None:
    ini_hashes = {
        str(path.relative_to(ROOT)): sha256(path)
        for path in sorted((ROOT / "simulations").glob("*.ini"))
    }
    manifest = {
        "git_commit": command_output("git", "rev-parse", "HEAD"),
        "git_dirty": bool(command_output(
            "git", "status", "--porcelain", "--untracked-files=no"
        )),
        "omnetpp": omnetpp_version(),
        "inet": os.environ.get("INET_VERSION", "not_recorded"),
        "python": platform.python_version(),
        "config_sha256": ini_hashes,
    }
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    print(f"manifesto: {OUTPUT}")


if __name__ == "__main__":
    main()

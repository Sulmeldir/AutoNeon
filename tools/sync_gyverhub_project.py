from __future__ import annotations

import json
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
META_PATH = ROOT / "include" / "project_meta.h"
PROJECT_JSON_PATH = ROOT / "project.json"

DEFINE_RE = re.compile(r'^\s*#define\s+(\w+)\s+"([^"]*)"\s*$')


def read_defines() -> dict[str, str]:
    values: dict[str, str] = {}

    for line in META_PATH.read_text(encoding="utf-8").splitlines():
        match = DEFINE_RE.match(line)
        if match:
            values[match.group(1)] = match.group(2)

    required = {
        "PROJECT_GH_REPO",
        "PROJECT_FW_VERSION",
        "PROJECT_NAME",
        "PROJECT_DESCRIPTION",
        "PROJECT_RELEASE_NOTES",
        "PROJECT_GH_CHIP_FAMILY",
        "PROJECT_RELEASE_ASSET",
    }
    missing = sorted(required - values.keys())
    if missing:
        raise SystemExit(f"Missing defines in {META_PATH}: {', '.join(missing)}")

    return values


def build_project_json(values: dict[str, str]) -> dict[str, object]:
    return {
        "name": values["PROJECT_NAME"],
        "about": values["PROJECT_DESCRIPTION"],
        "version": values["PROJECT_FW_VERSION"],
        "notes": values["PROJECT_RELEASE_NOTES"],
        "builds": [
            {
                "chipFamily": values["PROJECT_GH_CHIP_FAMILY"],
                "parts": [
                    {
                        "path": f'https://github.com/{values["PROJECT_GH_REPO"]}/releases/latest/download/{values["PROJECT_RELEASE_ASSET"]}',
                        "offset": 0,
                    }
                ],
            }
        ],
    }


def main() -> None:
    values = read_defines()
    data = build_project_json(values)
    PROJECT_JSON_PATH.write_text(
        json.dumps(data, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )
    print(f"Updated {PROJECT_JSON_PATH.relative_to(ROOT)} from {META_PATH.relative_to(ROOT)}")


if __name__ == "__main__":
    main()

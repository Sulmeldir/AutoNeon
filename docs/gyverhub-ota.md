# GyverHub OTA setup

This project is configured for GyverHub OTA checks using:

- Firmware version reported from `src/hub/hub.cpp`
- Release manifest in generated `project.json`
- GitHub Actions workflow `.github/workflows/release-firmware.yml`
- Sync script `tools/sync_gyverhub_project.py`

## Current conventions

- Repository: `Sulmeldir/AutoNeon`
- Environment: `esp32-s3-devkitc-1`
- Release asset: `AutoNEON-esp32-s3-devkitc-1.bin`
- Chip family for GyverHub: `ESP32-S3`

## How to publish a new OTA version

1. Update `PROJECT_FW_VERSION` and, if needed, notes in `include/project_meta.h`.
2. Run `python tools/sync_gyverhub_project.py`.
3. Commit and push the changed `project_meta.h` and `project.json`.
4. Create a GitHub Release.
5. The workflow will verify `project.json`, build the firmware, and attach both `AutoNEON-esp32-s3-devkitc-1.bin` and `AutoNEON-project.json` to the release.

After that, GyverHub clients that open the device card should see the new version and offer OTA update.

## Why this avoids 404 during rollout

The release workflow now attaches both the firmware binary and a release manifest to the same GitHub release.
This keeps OTA metadata aligned with the binary published for that specific release and reduces rollout timing issues.

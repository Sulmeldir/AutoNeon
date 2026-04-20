# GyverHub OTA setup

This project is configured for GyverHub OTA checks using:

- Firmware version reported from `src/hub/hub.cpp`
- Repository metadata in generated `project.json`
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
5. The workflow will verify `project.json`, build the firmware, and attach `AutoNEON-esp32-s3-devkitc-1.bin` to the release.

After that, GyverHub clients that open the device card should see the new version and offer OTA update.

## OTA test cycle

If the board already runs version `0.1.1`, GyverHub will not offer OTA while GitHub still advertises `0.1.1`.
For the next OTA test you must publish a strictly newer version, for example `0.1.2`.

On boot the firmware now prints OTA diagnostics to Serial:

- Current GyverHub firmware version string
- Expected `project.json` URL
- Expected latest release asset URL

That makes it easier to verify that the device and repository metadata match before testing OTA.

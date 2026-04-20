#pragma once

// GyverHub OTA expects the firmware version in the form "owner/repo@version".
// Keep these values in sync with project.json.
#define PROJECT_GH_REPO "Sulmeldir/AutoNeon"
#define PROJECT_FW_VERSION "0.1.4"

// Human-readable metadata for the project repository.
#define PROJECT_NAME "AutoNEON"
#define PROJECT_DESCRIPTION "ESP32-S3 controller for FastLED neon strip effects with GyverHub UI"
#define PROJECT_RELEASE_NOTES "Manual OTA smoke test target"

// GyverHub repository metadata used to generate project.json.
#define PROJECT_GH_CHIP_FAMILY "ESP32-S3"
#define PROJECT_PIO_ENV "esp32-s3-devkitc-1"
#define PROJECT_RELEASE_ASSET "AutoNEON-esp32-s3-devkitc-1.bin"

#include <GyverHub.h>
#include <HTTPClient.h>
#include <Update.h>
#include <WiFiClientSecure.h>
#include "project_meta.h"

GyverHub hub("AutoNEON", "ESP32-S3", "");
extern int TypeAlg;
extern uint8_t Col;
extern String AlgStr;
extern int Vsp_INTERVAL;
int NLeds;

namespace HUB
{
  namespace
  {
    struct OtaState
    {
      String status = F("Idle");
      String availableVersion;
      String availableNotes;
      bool updateAvailable = false;
      bool checkRequested = false;
      bool installRequested = false;
      bool busy = false;
    } otaState;

    String firmwareVersion()
    {
      return String(F(PROJECT_GH_REPO "@")) + F(PROJECT_FW_VERSION);
    }

    String firmwareNumber()
    {
      return String(F(PROJECT_FW_VERSION));
    }

    String projectJsonUrl()
    {
      return String(F("https://raw.githubusercontent.com/")) + F(PROJECT_GH_REPO) + F("/main/project.json");
    }

    String releaseAssetUrl()
    {
      return String(F("https://github.com/")) + F(PROJECT_GH_REPO) + F("/releases/latest/download/") + F(PROJECT_RELEASE_ASSET);
    }

    String versionedReleaseAssetUrl(const String &version)
    {
      return String(F("https://github.com/")) + F(PROJECT_GH_REPO) + F("/releases/download/v") + version + F("/") + F(PROJECT_RELEASE_ASSET);
    }

    void logOta(const String &message)
    {
      Serial.println(String(F("[OTA] ")) + message);
    }

    void refreshUi()
    {
      hub.sendRefresh();
    }

    String extractJsonString(const String &json, const String &key, int fromIndex = 0)
    {
      String needle = String('\"') + key + '\"';
      int keyPos = json.indexOf(needle, fromIndex);
      if (keyPos < 0)
        return String();

      int colonPos = json.indexOf(':', keyPos + needle.length());
      if (colonPos < 0)
        return String();

      int firstQuote = json.indexOf('\"', colonPos + 1);
      if (firstQuote < 0)
        return String();

      int secondQuote = json.indexOf('\"', firstQuote + 1);
      if (secondQuote < 0)
        return String();

      return json.substring(firstQuote + 1, secondQuote);
    }

    int compareVersions(const String &left, const String &right)
    {
      int leftIndex = 0;
      int rightIndex = 0;

      while (leftIndex < left.length() || rightIndex < right.length())
      {
        long leftPart = 0;
        long rightPart = 0;

        while (leftIndex < left.length() && left[leftIndex] != '.')
        {
          if (isDigit(left[leftIndex]))
            leftPart = leftPart * 10 + (left[leftIndex] - '0');
          ++leftIndex;
        }

        while (rightIndex < right.length() && right[rightIndex] != '.')
        {
          if (isDigit(right[rightIndex]))
            rightPart = rightPart * 10 + (right[rightIndex] - '0');
          ++rightIndex;
        }

        if (leftPart < rightPart)
          return -1;
        if (leftPart > rightPart)
          return 1;

        if (leftIndex < left.length())
          ++leftIndex;
        if (rightIndex < right.length())
          ++rightIndex;
      }

      return 0;
    }

    bool fetchText(const String &url, String &payload, String &error)
    {
      WiFiClientSecure client;
      client.setInsecure();

      HTTPClient http;
      if (!http.begin(client, url))
      {
        error = F("HTTP begin failed");
        return false;
      }

      http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
      const int httpCode = http.GET();
      if (httpCode != HTTP_CODE_OK)
      {
        error = String(F("HTTP ")) + httpCode;
        http.end();
        return false;
      }

      payload = http.getString();
      http.end();
      return true;
    }

    bool downloadAndInstall(const String &url, String &error)
    {
      WiFiClientSecure client;
      client.setInsecure();

      HTTPClient http;
      if (!http.begin(client, url))
      {
        error = F("HTTP begin failed");
        return false;
      }

      http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
      const int httpCode = http.GET();
      if (httpCode != HTTP_CODE_OK)
      {
        error = String(F("HTTP ")) + httpCode;
        http.end();
        return false;
      }

      const int contentLength = http.getSize();
      if (contentLength <= 0)
      {
        error = F("Invalid content length");
        http.end();
        return false;
      }

      if (!Update.begin(contentLength))
      {
        error = Update.errorString();
        http.end();
        return false;
      }

      WiFiClient *stream = http.getStreamPtr();
      const size_t written = Update.writeStream(*stream);
      if (written != static_cast<size_t>(contentLength))
      {
        error = String(F("Written ")) + written + F(" of ") + contentLength;
        Update.abort();
        http.end();
        return false;
      }

      if (!Update.end())
      {
        error = Update.errorString();
        http.end();
        return false;
      }

      if (!Update.isFinished())
      {
        error = F("Update not finished");
        http.end();
        return false;
      }

      http.end();
      return true;
    }

    void requestCheck()
    {
      if (otaState.busy)
        return;

      otaState.status = F("Check requested");
      otaState.checkRequested = true;
      refreshUi();
    }

    void requestInstall()
    {
      if (otaState.busy || !otaState.updateAvailable)
        return;

      otaState.status = F("Install requested");
      otaState.installRequested = true;
      refreshUi();
    }

    void handleManifestCheck()
    {
      otaState.busy = true;
      otaState.checkRequested = false;
      otaState.updateAvailable = false;
      otaState.availableVersion = String();
      otaState.availableNotes = String();
      otaState.status = F("Checking GitHub...");
      refreshUi();
      logOta(String(F("Checking manifest: ")) + projectJsonUrl());

      String payload;
      String error;
      if (!fetchText(projectJsonUrl(), payload, error))
      {
        otaState.status = String(F("Check failed: ")) + error;
        otaState.busy = false;
        logOta(otaState.status);
        refreshUi();
        return;
      }

      const String remoteVersion = extractJsonString(payload, F("version"));
      const String remoteNotes = extractJsonString(payload, F("notes"));
      const int chipPos = payload.indexOf(String(F("\"chipFamily\": \"")) + F(PROJECT_GH_CHIP_FAMILY) + '\"');
      const String remotePath = chipPos >= 0 ? extractJsonString(payload, F("path"), chipPos) : String();

      if (!remoteVersion.length())
      {
        otaState.status = F("Check failed: no version");
      }
      else if (!remotePath.length())
      {
        otaState.status = F("Check failed: no build");
      }
      else
      {
        otaState.availableVersion = remoteVersion;
        otaState.availableNotes = remoteNotes;
        otaState.updateAvailable = compareVersions(firmwareNumber(), remoteVersion) < 0;

        if (otaState.updateAvailable)
          otaState.status = String(F("Update available: ")) + remoteVersion;
        else
          otaState.status = String(F("Already up to date: ")) + remoteVersion;
      }

      otaState.busy = false;
      logOta(otaState.status);
      refreshUi();
    }

    void handleInstall()
    {
      otaState.busy = true;
      otaState.installRequested = false;
      otaState.status = String(F("Installing ")) + otaState.availableVersion;
      refreshUi();

      const String downloadUrl = versionedReleaseAssetUrl(otaState.availableVersion);
      logOta(String(F("Installing from: ")) + downloadUrl);

      String error;
      if (!downloadAndInstall(downloadUrl, error))
      {
        otaState.status = String(F("Install failed: ")) + error;
        otaState.busy = false;
        logOta(otaState.status);
        refreshUi();
        return;
      }

      otaState.status = String(F("Install complete: ")) + otaState.availableVersion;
      logOta(otaState.status);
      refreshUi();
      delay(1000);
      ESP.restart();
    }
  }

  void build(gh::Builder &b)
  {
    b.Input(&NLeds).size(2);
    b.Select_("sel", &TypeAlg).text(AlgStr);
    b.Color(&Col);
    b.Slider(&Vsp_INTERVAL).text("Interval").range(1, 2000, 1);

    b.Title(F("OTA"));
    b.Display_(F("ota_fw"), firmwareNumber()).label(F("Current"));
    b.Display_(F("ota_remote"), otaState.availableVersion.length() ? otaState.availableVersion : String(F("-"))).label(F("Remote"));
    b.Display_(F("ota_notes"), otaState.availableNotes.length() ? otaState.availableNotes : String(F("-"))).label(F("Notes")).rows(2);
    b.Display_(F("ota_status"), otaState.status).label(F("Status")).rows(2);

    if (b.Button().label(F("Check OTA")).click())
      requestCheck();

    if (otaState.updateAvailable && b.Button().label(F("Install OTA")).click())
      requestInstall();
  }

  void setup()
  {
    hub.mqtt.config("m3.wqtt.ru", 14635, "u_9ICRMS", "n5V6oZGA");
    hub.setVersion(firmwareVersion());

    hub.onBuild(build);
    hub.begin();

    logOta(F("GyverHub OTA metadata"));
    logOta(String(F("Firmware: ")) + firmwareVersion());
    logOta(String(F("project.json: ")) + projectJsonUrl());
    logOta(String(F("release bin: ")) + releaseAssetUrl());
    Serial.println("hub begin");
    hub.tick();
  }

  void tick()
  {
    if (otaState.checkRequested)
      handleManifestCheck();

    if (otaState.installRequested)
      handleInstall();

    hub.tick();
  }
}

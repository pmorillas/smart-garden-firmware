#include "OtaUpdater.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>

bool OtaUpdater::update(const char* url) {
  Serial.printf("[OTA] Iniciant descàrrega: %s\n", url);

  HTTPClient http;
  http.begin(url);
  http.setTimeout(30000);
  // Segueix redireccions HTTP
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("[OTA] Error HTTP: %d\n", httpCode);
    http.end();
    return false;
  }

  int contentLength = http.getSize();
  if (contentLength <= 0) {
    Serial.println("[OTA] Content-Length desconegut o buit");
    http.end();
    return false;
  }

  Serial.printf("[OTA] Mida del firmware: %d bytes\n", contentLength);

  if (!Update.begin(contentLength)) {
    Serial.printf("[OTA] No hi ha prou espai: %s\n", Update.errorString());
    http.end();
    return false;
  }

  WiFiClient* stream = http.getStreamPtr();
  size_t written = Update.writeStream(*stream);

  if (written != (size_t)contentLength) {
    Serial.printf("[OTA] Escriptura incompleta: %u/%d bytes\n", written, contentLength);
    http.end();
    return false;
  }

  if (!Update.end()) {
    Serial.printf("[OTA] Error finalitzant: %s\n", Update.errorString());
    http.end();
    return false;
  }

  http.end();

  if (!Update.isFinished()) {
    Serial.println("[OTA] Actualització no completada");
    return false;
  }

  Serial.println("[OTA] Firmware aplicat. Reiniciant...");
  return true;
}

#ifndef ETHER_PRINTER_H
#define ETHER_PRINTER_H

#include <Arduino.h>
#include <Ethernet.h>
#include <SPI.h>
#include <SD.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "macro.h"

/*
 * OBO Ethernet/SD web interface
 *
 * SD card files:
 *   DATA.TXT   - device IP, MAC address and protected-settings credentials
 *   INDEX.HTM     - home page (8.3 filename for classic Arduino SD library)
 *   SETTINGS.HTM  - protected settings page
 *   STYLE.CSS  - page styling
 *   APP.JS     - page logic
 *
 * DATA.TXT format:
 *   line 1: device IPv4 address
 *   line 2: device MAC address (AA:BB:CC:DD:EE:FF)
 *   line 3: settings username
 *   line 4: settings password
 *
 * For migration, the loader also understands:
 *   - the older three-line format: IP, username, password
 *   - the older six-line format: IP, local port, remote IP, remote port,
 *     username, password
 * Both are normalized into the new four-line format.
 */

#define SD_CS_PIN 4
#define ETHERNET_CS_PIN 10
#define HTTP_PORT 80

#define WEB_MESSAGE_COUNT 50
#define WEB_MESSAGE_LENGTH 64
#define HTTP_LINE_LENGTH 128
#define HTTP_BODY_LENGTH 256
#define CONFIG_LINE_LENGTH 48
#define USERNAME_LENGTH 16
#define PASSWORD_LENGTH 32

static const char CONFIG_FILE[] = "DATA.TXT";
static const char HTML_FILE[] = "INDEX.HTM";
static const char SETTINGS_HTML_FILE[] = "SETTINGS.HTM";
static const char CSS_FILE[] = "STYLE.CSS";
static const char JS_FILE[] = "APP.JS";

static const char DEFAULT_DEVICE_IP[] = "192.168.1.177";
static const char DEFAULT_MAC_ADDRESS[] = "DE:AD:BE:EF:FE:E0";
static const char DEFAULT_USERNAME[] = "admin";
static const char DEFAULT_PASSWORD[] = "admin";

byte mac[6] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xE0 };
IPAddress ip;
EthernetServer server(HTTP_PORT);

static char webMessages[WEB_MESSAGE_COUNT][WEB_MESSAGE_LENGTH];
static uint8_t webMessageHead = 0;   // next write position
static uint8_t webMessageCount = 0;  // valid entries, max 50

static char settingsUsername[USERNAME_LENGTH + 1];
static char settingsPassword[PASSWORD_LENGTH + 1];

static bool pendingNetworkApply = false;
static unsigned long pendingNetworkApplyAt = 0;

static void safeCopy(char *destination, const char *source, size_t destinationSize) {
  if (destinationSize == 0) return;
  if (source == NULL) source = "";
  strncpy(destination, source, destinationSize - 1);
  destination[destinationSize - 1] = '\0';
}

static bool textEqualsIgnoreCase(const char *a, const char *b) {
  while (*a && *b) {
    if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return false;
    ++a;
    ++b;
  }
  return *a == '\0' && *b == '\0';
}

static bool startsWith(const char *text, const char *prefix) {
  return strncmp(text, prefix, strlen(prefix)) == 0;
}

static bool readConfigLine(File &file, char *output, size_t outputSize) {
  if (outputSize == 0) return false;

  size_t position = 0;
  bool readAnything = false;

  while (file.available()) {
    int value = file.read();
    if (value < 0) break;
    char c = (char)value;
    readAnything = true;

    if (c == '\n') break;
    if (c == '\r') continue;

    if (position + 1 < outputSize) {
      output[position++] = c;
    }
  }

  output[position] = '\0';
  return readAnything;
}

static bool parseIPv4(const char *text, IPAddress &result) {
  if (text == NULL || *text == '\0') return false;

  uint16_t octets[4] = {0, 0, 0, 0};
  uint8_t octetIndex = 0;
  uint8_t digits = 0;

  for (const char *cursor = text; ; ++cursor) {
    char c = *cursor;

    if (c >= '0' && c <= '9') {
      if (digits >= 3) return false;
      octets[octetIndex] = (uint16_t)(octets[octetIndex] * 10 + (c - '0'));
      if (octets[octetIndex] > 255) return false;
      ++digits;
    } else if (c == '.' || c == '\0') {
      if (digits == 0) return false;
      if (c == '\0') {
        if (octetIndex != 3) return false;
        break;
      }
      if (octetIndex >= 3) return false;
      ++octetIndex;
      digits = 0;
    } else {
      return false;
    }
  }

  result = IPAddress((uint8_t)octets[0], (uint8_t)octets[1],
                     (uint8_t)octets[2], (uint8_t)octets[3]);
  return true;
}

static bool isUnsignedNumber(const char *text) {
  if (text == NULL || *text == '\0') return false;
  while (*text) {
    if (*text < '0' || *text > '9') return false;
    ++text;
  }
  return true;
}

static int hexValue(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

static bool parseMacAddress(const char *text, byte result[6]) {
  if (text == NULL || *text == '\0') return false;

  byte parsed[6] = {0, 0, 0, 0, 0, 0};
  uint8_t byteIndex = 0;
  uint8_t nibbleCount = 0;

  for (const char *cursor = text; ; ++cursor) {
    char c = *cursor;
    int value = hexValue(c);

    if (value >= 0) {
      if (byteIndex >= 6 || nibbleCount >= 2) return false;
      parsed[byteIndex] = (byte)((parsed[byteIndex] << 4) | value);
      ++nibbleCount;
    } else if (c == ':' || c == '-' || c == '\0') {
      if (nibbleCount != 2 || byteIndex >= 6) return false;
      ++byteIndex;
      nibbleCount = 0;

      if (c == '\0') {
        if (byteIndex != 6) return false;
        break;
      }
    } else {
      return false;
    }
  }

  memcpy(result, parsed, 6);
  return true;
}

static bool macEquals(const byte a[6], const byte b[6]) {
  return memcmp(a, b, 6) == 0;
}

static void ipToText(const IPAddress &address, char *output, size_t outputSize) {
  snprintf(output, outputSize, "%u.%u.%u.%u",
           address[0], address[1], address[2], address[3]);
}

static void macToText(const byte address[6], char *output, size_t outputSize) {
  snprintf(output, outputSize, "%02X:%02X:%02X:%02X:%02X:%02X",
           address[0], address[1], address[2],
           address[3], address[4], address[5]);
}

static void loadDefaultConfiguration() {
  parseIPv4(DEFAULT_DEVICE_IP, ip);
  parseMacAddress(DEFAULT_MAC_ADDRESS, mac);
  safeCopy(settingsUsername, DEFAULT_USERNAME, sizeof(settingsUsername));
  safeCopy(settingsPassword, DEFAULT_PASSWORD, sizeof(settingsPassword));
}

static bool saveConfiguration() {
  char localIpText[16];
  char macText[18];
  ipToText(ip, localIpText, sizeof(localIpText));
  macToText(mac, macText, sizeof(macText));

  SD.remove(CONFIG_FILE);
  File file = SD.open(CONFIG_FILE, FILE_WRITE);
  if (!file) return false;

  file.println(localIpText);
  file.println(macText);
  file.println(settingsUsername);
  file.println(settingsPassword);
  file.flush();
  file.close();
  return true;
}

static void loadConfiguration() {
  loadDefaultConfiguration();

  File file = SD.open(CONFIG_FILE, FILE_READ);
  if (!file) {
    Serial.println(F("DATA.TXT not found; creating it with defaults."));
    saveConfiguration();
    return;
  }

  char lines[6][CONFIG_LINE_LENGTH];
  uint8_t lineCount = 0;
  while (lineCount < 6 && readConfigLine(file, lines[lineCount], sizeof(lines[lineCount]))) {
    ++lineCount;
  }
  file.close();

  IPAddress parsedAddress;
  if (lineCount > 0 && parseIPv4(lines[0], parsedAddress)) {
    ip = parsedAddress;
  }

  // Old six-line layout: IP, local port, remote IP, remote port, user, password.
  bool legacySixLine = false;
  if (lineCount >= 4 && isUnsignedNumber(lines[1]) &&
      parseIPv4(lines[2], parsedAddress) && isUnsignedNumber(lines[3])) {
    legacySixLine = true;
  }

  if (legacySixLine) {
    if (lineCount > 4 && lines[4][0] != '\0') {
      safeCopy(settingsUsername, lines[4], sizeof(settingsUsername));
    }
    if (lineCount > 5 && lines[5][0] != '\0') {
      safeCopy(settingsPassword, lines[5], sizeof(settingsPassword));
    }
    Serial.println(F("Migrating six-line DATA.TXT to IP/MAC/user/password."));
  } else if (lineCount >= 4) {
    byte parsedMac[6];
    if (parseMacAddress(lines[1], parsedMac)) {
      memcpy(mac, parsedMac, sizeof(mac));
    } else {
      Serial.println(F("Invalid MAC in DATA.TXT; using the default MAC."));
    }

    if (lines[2][0] != '\0') {
      safeCopy(settingsUsername, lines[2], sizeof(settingsUsername));
    }
    if (lines[3][0] != '\0') {
      safeCopy(settingsPassword, lines[3], sizeof(settingsPassword));
    }
  } else {
    // Previous three-line layout: IP, user, password.
    if (lineCount > 1 && lines[1][0] != '\0') {
      safeCopy(settingsUsername, lines[1], sizeof(settingsUsername));
    }
    if (lineCount > 2 && lines[2][0] != '\0') {
      safeCopy(settingsPassword, lines[2], sizeof(settingsPassword));
    }
    Serial.println(F("Migrating three-line DATA.TXT to IP/MAC/user/password."));
  }

  // Normalize all accepted layouts into the current four-line format.
  saveConfiguration();
}

static void addBufferMessage(const char *message) {
  safeCopy(webMessages[webMessageHead], message, WEB_MESSAGE_LENGTH);
  webMessageHead = (uint8_t)((webMessageHead + 1) % WEB_MESSAGE_COUNT);
  if (webMessageCount < WEB_MESSAGE_COUNT) ++webMessageCount;
}

static void translateMessage(const char *buffer, unsigned long bufferSize,
                             char *output, size_t outputSize) {
  if (buffer == NULL || bufferSize == 0) {
    safeCopy(output, "Empty message", outputSize);
    return;
  }

  const uint8_t code = (uint8_t)buffer[0];
  const uint8_t value1 = bufferSize > 1 ? (uint8_t)buffer[1] : 0;
  const uint8_t value2 = bufferSize > 2 ? (uint8_t)buffer[2] : 0;
  const unsigned long seconds = millis() / 1000UL;

  switch (code) {
    case 0:
      snprintf(output, outputSize, "[%lus] Scale OK: %u kg", seconds, value1);
      break;
    case 1:
      snprintf(output, outputSize, "[%lus] Door %u is open", seconds, value1);
      break;
    case 2:
      snprintf(output, outputSize, "[%lus] Scale error: %u kg", seconds, value1);
      break;
    case 8:
      snprintf(output, outputSize, "[%lus] Cycle start dir=%u weight=%u kg",
               seconds, value1, value2);
      break;
    case 9:
      snprintf(output, outputSize, "[%lus] Cycle finished, status=%u", seconds, value1);
      break;
    case 10:
      snprintf(output, outputSize, "[%lus] Door closed, weight=%u kg", seconds, value1);
      break;
    case 11:
      snprintf(output, outputSize, "[%lus] Class mismatch: class=%u raw=%u",
               seconds, value1, value2);
      break;
    case 12:
      snprintf(output, outputSize, "[%lus] Cycle request cancelled", seconds);
      break;
    default:
      snprintf(output, outputSize, "[%lus] Message code=%u a=%u b=%u",
               seconds, code, value1, value2);
      break;
  }
}

static void sendStatus(EthernetClient &client, const __FlashStringHelper *status,
                       const __FlashStringHelper *contentType) {
  client.print(F("HTTP/1.1 "));
  client.println(status);
  client.print(F("Content-Type: "));
  client.println(contentType);
  client.println(F("Cache-Control: no-store"));
  client.println(F("Connection: close"));
  client.println();
}

static void sendJsonError(EthernetClient &client, const __FlashStringHelper *status,
                          const __FlashStringHelper *message) {
  sendStatus(client, status, F("application/json; charset=utf-8"));
  client.print(F("{\"ok\":false,\"message\":\""));
  client.print(message);
  client.println(F("\"}"));
}

static void sendNotFound(EthernetClient &client) {
  sendJsonError(client, F("404 Not Found"), F("Not found"));
}

static void sendStaticFile(EthernetClient &client, const char *filename,
                           const __FlashStringHelper *contentType) {
  File file = SD.open(filename, FILE_READ);
  if (!file) {
    sendJsonError(client, F("500 Internal Server Error"), F("SD file missing"));
    return;
  }

  client.println(F("HTTP/1.1 200 OK"));
  client.print(F("Content-Type: "));
  client.println(contentType);
  client.print(F("Content-Length: "));
  client.println(file.size());
  client.println(F("Cache-Control: no-cache"));
  client.println(F("Connection: close"));
  client.println();

  uint8_t chunk[64];
  while (file.available()) {
    int count = file.read(chunk, sizeof(chunk));
    if (count <= 0) break;
    client.write(chunk, (size_t)count);
  }
  file.close();
}

static void printJsonEscaped(EthernetClient &client, const char *text) {
  while (*text) {
    char c = *text++;
    switch (c) {
      case '\\': client.print(F("\\\\")); break;
      case '"':  client.print(F("\\\"")); break;
      case '\n': client.print(F("\\n")); break;
      case '\r': break;
      case '\t': client.print(F("\\t")); break;
      default:
        if ((uint8_t)c >= 0x20) client.write((uint8_t)c);
        break;
    }
  }
}

static void sendMessagesJson(EthernetClient &client) {
  char ipText[16];
  char macText[18];
  ipToText(ip, ipText, sizeof(ipText));
  macToText(mac, macText, sizeof(macText));

  sendStatus(client, F("200 OK"), F("application/json; charset=utf-8"));
  client.print(F("{\"ip\":\""));
  client.print(ipText);
  client.print(F("\",\"mac\":\""));
  client.print(macText);
  client.print(F("\",\"count\":"));
  client.print(webMessageCount);
  client.print(F(",\"capacity\":"));
  client.print(WEB_MESSAGE_COUNT);
  client.print(F(",\"uptimeSeconds\":"));
  client.print(millis() / 1000UL);
  client.print(F(",\"messages\":["));

  const uint8_t oldest = (uint8_t)((webMessageHead + WEB_MESSAGE_COUNT - webMessageCount)
                                    % WEB_MESSAGE_COUNT);
  for (uint8_t i = 0; i < webMessageCount; ++i) {
    if (i > 0) client.write(',');
    client.write('"');
    const uint8_t messageIndex = (uint8_t)((oldest + i) % WEB_MESSAGE_COUNT);
    printJsonEscaped(client, webMessages[messageIndex]);
    client.write('"');
  }

  client.println(F("]}"));
}

static void urlDecode(const char *source, char *destination, size_t destinationSize) {
  if (destinationSize == 0) return;
  size_t out = 0;

  while (*source && out + 1 < destinationSize) {
    if (*source == '+') {
      destination[out++] = ' ';
      ++source;
    } else if (*source == '%' && source[1] && source[2]) {
      int high = hexValue(source[1]);
      int low = hexValue(source[2]);
      if (high >= 0 && low >= 0) {
        destination[out++] = (char)((high << 4) | low);
        source += 3;
      } else {
        destination[out++] = *source++;
      }
    } else {
      destination[out++] = *source++;
    }
  }

  destination[out] = '\0';
}

static bool getFormValue(const char *body, const char *key,
                         char *output, size_t outputSize) {
  const size_t keyLength = strlen(key);
  const char *cursor = body;

  while (*cursor) {
    const char *pairEnd = strchr(cursor, '&');
    if (pairEnd == NULL) pairEnd = cursor + strlen(cursor);

    const char *equals = (const char *)memchr(cursor, '=', (size_t)(pairEnd - cursor));
    if (equals != NULL && (size_t)(equals - cursor) == keyLength &&
        strncmp(cursor, key, keyLength) == 0) {
      char encoded[HTTP_BODY_LENGTH];
      size_t valueLength = (size_t)(pairEnd - equals - 1);
      if (valueLength >= sizeof(encoded)) valueLength = sizeof(encoded) - 1;
      memcpy(encoded, equals + 1, valueLength);
      encoded[valueLength] = '\0';
      urlDecode(encoded, output, outputSize);
      return true;
    }

    cursor = *pairEnd == '&' ? pairEnd + 1 : pairEnd;
  }

  if (outputSize > 0) output[0] = '\0';
  return false;
}

static bool validCredentialText(const char *text, size_t maxLength) {
  size_t length = strlen(text);
  if (length == 0 || length > maxLength) return false;

  for (size_t i = 0; i < length; ++i) {
    uint8_t c = (uint8_t)text[i];
    if (c < 0x21 || c > 0x7E || c == '&' || c == '=') return false;
  }
  return true;
}

static void handleSettingsPost(EthernetClient &client, const char *body) {
  char username[USERNAME_LENGTH + 1];
  char password[PASSWORD_LENGTH + 1];
  char newIpText[16];
  char newMacText[18];
  char newUsername[USERNAME_LENGTH + 1];
  char newPassword[PASSWORD_LENGTH + 1];

  if (!getFormValue(body, "username", username, sizeof(username)) ||
      !getFormValue(body, "password", password, sizeof(password))) {
    sendJsonError(client, F("400 Bad Request"), F("Current credentials are required"));
    return;
  }

  if (strcmp(username, settingsUsername) != 0 ||
      strcmp(password, settingsPassword) != 0) {
    sendJsonError(client, F("401 Unauthorized"), F("Invalid username or password"));
    return;
  }

  getFormValue(body, "ip", newIpText, sizeof(newIpText));
  getFormValue(body, "mac", newMacText, sizeof(newMacText));
  getFormValue(body, "newUsername", newUsername, sizeof(newUsername));
  getFormValue(body, "newPassword", newPassword, sizeof(newPassword));

  IPAddress requestedIp = ip;
  byte requestedMac[6];
  memcpy(requestedMac, mac, sizeof(requestedMac));
  bool ipChanged = false;
  bool macChanged = false;

  if (newIpText[0] != '\0') {
    if (!parseIPv4(newIpText, requestedIp)) {
      sendJsonError(client, F("400 Bad Request"), F("Invalid IPv4 address"));
      return;
    }
    ipChanged = (requestedIp[0] != ip[0] || requestedIp[1] != ip[1] ||
                 requestedIp[2] != ip[2] || requestedIp[3] != ip[3]);
  }

  if (newMacText[0] != '\0') {
    if (!parseMacAddress(newMacText, requestedMac)) {
      sendJsonError(client, F("400 Bad Request"), F("Invalid MAC address"));
      return;
    }
    macChanged = !macEquals(requestedMac, mac);
  }

  if (newUsername[0] != '\0' && !validCredentialText(newUsername, USERNAME_LENGTH)) {
    sendJsonError(client, F("400 Bad Request"), F("Invalid new username"));
    return;
  }

  if (newPassword[0] != '\0' && !validCredentialText(newPassword, PASSWORD_LENGTH)) {
    sendJsonError(client, F("400 Bad Request"), F("Invalid new password"));
    return;
  }

  IPAddress oldIp = ip;
  byte oldMac[6];
  memcpy(oldMac, mac, sizeof(oldMac));
  char oldUsername[USERNAME_LENGTH + 1];
  char oldPassword[PASSWORD_LENGTH + 1];
  safeCopy(oldUsername, settingsUsername, sizeof(oldUsername));
  safeCopy(oldPassword, settingsPassword, sizeof(oldPassword));

  ip = requestedIp;
  memcpy(mac, requestedMac, sizeof(mac));
  if (newUsername[0] != '\0') safeCopy(settingsUsername, newUsername, sizeof(settingsUsername));
  if (newPassword[0] != '\0') safeCopy(settingsPassword, newPassword, sizeof(settingsPassword));

  if (!saveConfiguration()) {
    ip = oldIp;
    memcpy(mac, oldMac, sizeof(mac));
    safeCopy(settingsUsername, oldUsername, sizeof(settingsUsername));
    safeCopy(settingsPassword, oldPassword, sizeof(settingsPassword));
    sendJsonError(client, F("500 Internal Server Error"), F("Could not save DATA.TXT"));
    return;
  }

  char ipText[16];
  char macText[18];
  ipToText(ip, ipText, sizeof(ipText));
  macToText(mac, macText, sizeof(macText));

  sendStatus(client, F("200 OK"), F("application/json; charset=utf-8"));
  client.print(F("{\"ok\":true,\"ip\":\""));
  client.print(ipText);
  client.print(F("\",\"mac\":\""));
  client.print(macText);
  client.print(F("\",\"ipChanged\":"));
  client.print(ipChanged ? F("true") : F("false"));
  client.print(F(",\"macChanged\":"));
  client.print(macChanged ? F("true") : F("false"));
  client.println(F(",\"message\":\"Settings saved\"}"));

  if (ipChanged || macChanged) {
    pendingNetworkApply = true;
    pendingNetworkApplyAt = millis() + 1500UL;
  }
}

static bool readHttpLine(EthernetClient &client, char *output, size_t outputSize,
                         unsigned long timeoutMs) {
  if (outputSize == 0) return false;

  size_t position = 0;
  unsigned long start = millis();

  while (client.connected() && millis() - start < timeoutMs) {
    while (client.available()) {
      char c = (char)client.read();
      if (c == '\n') {
        output[position] = '\0';
        return true;
      }
      if (c != '\r' && position + 1 < outputSize) {
        output[position++] = c;
      }
      start = millis();
    }
  }

  output[position] = '\0';
  return position > 0;
}

static void handleHttpClient(EthernetClient &client) {
  char requestLine[HTTP_LINE_LENGTH];
  if (!readHttpLine(client, requestLine, sizeof(requestLine), 1000UL)) return;

  char method[8] = {0};
  char path[64] = {0};
  if (sscanf(requestLine, "%7s %63s", method, path) != 2) {
    sendJsonError(client, F("400 Bad Request"), F("Malformed request"));
    return;
  }

  int contentLength = 0;
  char line[HTTP_LINE_LENGTH];

  while (readHttpLine(client, line, sizeof(line), 1000UL)) {
    if (line[0] == '\0') break;
    if (startsWith(line, "Content-Length:") || startsWith(line, "content-length:")) {
      const char *value = strchr(line, ':');
      if (value != NULL) contentLength = atoi(value + 1);
    }
  }

  // Ignore a query string when matching local routes.
  char *queryString = strchr(path, '?');
  if (queryString != NULL) *queryString = '\0';

  if (textEqualsIgnoreCase(method, "GET")) {
    if (strcmp(path, "/") == 0 || strcmp(path, "/index.htm") == 0 ||
        strcmp(path, "/INDEX.HTM") == 0) {
      sendStaticFile(client, HTML_FILE, F("text/html; charset=utf-8"));
    } else if (strcmp(path, "/settings") == 0 || strcmp(path, "/settings/") == 0 ||
               strcmp(path, "/settings.htm") == 0 || strcmp(path, "/SETTINGS.HTM") == 0) {
      sendStaticFile(client, SETTINGS_HTML_FILE, F("text/html; charset=utf-8"));
    } else if (strcmp(path, "/style.css") == 0 || strcmp(path, "/STYLE.CSS") == 0) {
      sendStaticFile(client, CSS_FILE, F("text/css; charset=utf-8"));
    } else if (strcmp(path, "/app.js") == 0 || strcmp(path, "/APP.JS") == 0) {
      sendStaticFile(client, JS_FILE, F("application/javascript; charset=utf-8"));
    } else if (strcmp(path, "/api/messages") == 0) {
      sendMessagesJson(client);
    } else {
      sendNotFound(client);
    }
    return;
  }

  if (textEqualsIgnoreCase(method, "POST") && strcmp(path, "/api/settings") == 0) {
    if (contentLength < 0 || contentLength >= HTTP_BODY_LENGTH) {
      sendJsonError(client, F("413 Payload Too Large"), F("Request body too large"));
      return;
    }

    char body[HTTP_BODY_LENGTH];
    int received = 0;
    unsigned long start = millis();

    while (received < contentLength && client.connected() && millis() - start < 1500UL) {
      while (client.available() && received < contentLength) {
        body[received++] = (char)client.read();
        start = millis();
      }
    }
    body[received] = '\0';

    if (received != contentLength) {
      sendJsonError(client, F("400 Bad Request"), F("Incomplete request body"));
      return;
    }

    handleSettingsPost(client, body);
    return;
  }

  sendJsonError(client, F("405 Method Not Allowed"), F("Method not allowed"));
}

static void applyPendingNetworkIfNeeded() {
  if (pendingNetworkApply && (long)(millis() - pendingNetworkApplyAt) >= 0) {
    pendingNetworkApply = false;
    Ethernet.begin(mac, ip);
    delay(50);
    server.begin();
    Serial.print(F("Ethernet restarted. Web server: http://"));
    Serial.print(Ethernet.localIP());
    Serial.println('/');
  }
}

void client_setup() {
  Serial.print(F("Initializing SD card..."));

  pinMode(ETHERNET_CS_PIN, OUTPUT);
  digitalWrite(ETHERNET_CS_PIN, HIGH);
  pinMode(SD_CS_PIN, OUTPUT);

  if (!SD.begin(SD_CS_PIN)) {
    Serial.println(F("failed."));
    while (true) delay(1);
  }
  Serial.println(F("done."));

  loadConfiguration();

  char configuredMac[18];
  macToText(mac, configuredMac, sizeof(configuredMac));
  Serial.print(F("Device IP: "));
  Serial.println(ip);
  Serial.print(F("Device MAC: "));
  Serial.println(configuredMac);

  Ethernet.begin(mac, ip);

  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println(F("Ethernet shield was not found."));
    while (true) delay(1);
  }

  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println(F("Ethernet cable is not connected."));
  }

  server.begin();
  Serial.print(F("Web server: http://"));
  Serial.print(Ethernet.localIP());
  Serial.println('/');

  addBufferMessage("System started");
}

/*
 * Must be called frequently from the main Arduino loop().
 * Do not run Ethernet or SD access from a timer interrupt.
 */
void client_loop() {
  applyPendingNetworkIfNeeded();

  EthernetClient client = server.available();
  if (client) {
    handleHttpClient(client);
    delay(1);
    client.stop();
  }
}

void send_message(const char *buffer, unsigned long bufferSize) {
  char translated[WEB_MESSAGE_LENGTH];
  translateMessage(buffer, bufferSize, translated, sizeof(translated));
  Serial.println(translated);
  addBufferMessage(translated);
}

#endif // ETHER_PRINTER_H

#include "homeplate.h"
#include <esp_chip_info.h>

#define REDRAW_NETWORK 0
#define REDRAW_WIFI    1
#define REDRAW_MQTT    2

// Use font's yAdvance to prevent line overlap on smaller displays
const static int lineHeight = (int)FONT_BODY.yAdvance + 2;

static uint32_t col1NameX()
{
  uint32_t width = hpDisplayWidth();
  return width < 800 ? 1 * (width / 32) : 1 * (width / 8);
}

static uint32_t col1DataX()
{
  uint32_t width = hpDisplayWidth();
  return width < 800 ? 7 * (width / 32) : 2 * (width / 8);
}

static uint32_t col2NameX()
{
  uint32_t width = hpDisplayWidth();
  return width < 800 ? 17 * (width / 32) : 5 * (width / 8);
}

static uint32_t col2DataX()
{
  uint32_t width = hpDisplayWidth();
  return width < 800 ? 23 * (width / 32) : 6 * (width / 8);
}

// truncateToFit: returns s unchanged if it fits in max_width pixels at the
// current font, otherwise returns a pointer to a static buffer containing
// "<head>..." truncated to fit. Measures against display's current font.
// Single static buffer — not safe to nest calls in one print statement.
static const char *truncateToFit(const char *s, uint16_t max_width)
{
    static char buf[128];
    if (!s) return "";
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
    if (w <= max_width) return s;
    // Doesn't fit. Try progressively shorter "<head>..." until it does.
    for (size_t n = strlen(s); n > 0; n--)
    {
        snprintf(buf, sizeof(buf), "%.*s...", (int)n, s);
        display.getTextBounds(buf, 0, 0, &x1, &y1, &w, &h);
        if (w <= max_width) return buf;
    }
    return "...";
}

const char *wl_status_to_string(wl_status_t status)
{
  switch (status)
  {
  case WL_NO_SHIELD:
    return "NO_SHIELD";
  case WL_IDLE_STATUS:
    return "IDLE_STATUS";
  case WL_NO_SSID_AVAIL:
    return "NO_SSID_AVAIL";
  case WL_SCAN_COMPLETED:
    return "SCAN_COMPLETED";
  case WL_CONNECTED:
    return "CONNECTED";
  case WL_CONNECT_FAILED:
    return "CONNECT_FAILED";
  case WL_CONNECTION_LOST:
    return "CONNECTION_LOST";
  case WL_DISCONNECTED:
    return "DISCONNECTED";
  }
  return "UNKNOWN";
}

void displayBoundaryBox()
{
  int bw = max(hpScaleX(10), 2);
  int16_t width = hpDisplayWidth();
  int16_t height = hpDisplayHeight();
  display.fillRect(0, 0, bw, height, HP_FG);                // left
  display.fillRect(width - bw, 0, bw, height, HP_FG);       // right
  display.fillRect(0, 0, width, bw, HP_FG);                 // top
  display.fillRect(0, height - bw, width, bw, HP_FG);       // bottom
}

void cleanField(uint32_t x, uint32_t y)
{
  display.fillRect(x, y - lineHeight, (hpDisplayWidth() / 8), lineHeight, HP_BG);
  //Serial.printf("fillRect(x:%u, y:%u, w:%u, h:%u)\n", x, y, (hpDisplayWidth() / 8), lineHeight);
}

void drawNetwork(uint32_t *yref, bool clean)
{
  uint32_t y = *yref;

  // network
  display.setCursor(col2NameX(), y);
  display.print("Hostname:");
  if (clean) { cleanField(col2DataX(), y); };
  display.setCursor(col2DataX(), y);
  display.print(WiFi.getHostname());
  // ip
  y += lineHeight;
  display.setCursor(col2NameX(), y);
  display.print("IP Address:");
  if (clean) { cleanField(col2DataX(), y); };
  display.setCursor(col2DataX(), y);
  display.print(WiFi.localIP().toString().c_str());
  // netmask
  y += lineHeight;
  display.setCursor(col2NameX(), y);
  display.print("Subnet Mask:");
  if (clean) { cleanField(col2DataX(), y); };
  display.setCursor(col2DataX(), y);
  display.print(WiFi.subnetMask().toString().c_str());
  // gateway
  y += lineHeight;
  display.setCursor(col2NameX(), y);
  display.print("Gateway:");
  if (clean) { cleanField(col2DataX(), y); };
  display.setCursor(col2DataX(), y);
  display.print(WiFi.gatewayIP().toString().c_str());
  // dns
  y += lineHeight;
  display.setCursor(col2NameX(), y);
  display.print("DNS:");
  if (clean) { cleanField(col2DataX(), y); };
  display.setCursor(col2DataX(), y);
  display.print(WiFi.dnsIP(0).toString().c_str());
  // mac
  y += lineHeight;
  display.setCursor(col2NameX(), y);
  display.print("MAC Address:");
  if (clean) { cleanField(col2DataX(), y); };
  display.setCursor(col2DataX(), y);
  display.print(WiFi.macAddress().c_str());

  *yref = y;
}

void drawWiFi(uint32_t *yref, bool clean)
{
  uint32_t y = *yref;

  display.setCursor(col2NameX(), y);
  display.print("SSID:");
  if (clean) { cleanField(col2DataX(), y); };
  display.setCursor(col2DataX(), y);
  display.print(WiFi.SSID().c_str());
  // bssid
  y += lineHeight;
  display.setCursor(col2NameX(), y);
  display.print("BSSID:");
  if (clean) { cleanField(col2DataX(), y); };
  display.setCursor(col2DataX(), y);
  display.print(WiFi.BSSIDstr().c_str());
  // signal
  y += lineHeight;
  display.setCursor(col2NameX(), y);
  display.print("Signal:");
  if (clean) { cleanField(col2DataX(), y); };
  display.setCursor(col2DataX(), y);
  display.printf("%d dBm", WiFi.RSSI());
  // wifi status
  y += lineHeight;
  display.setCursor(col2NameX(), y);
  display.print("WiFi Status:");
  if (clean) { cleanField(col2DataX(), y); };
  display.setCursor(col2DataX(), y);
  display.print(wl_status_to_string(WiFi.status()));

  *yref = y;
}

void drawMQTT(uint32_t *yref, bool clean)
{
  uint32_t y = *yref;

  display.setCursor(col2NameX(), y);
  display.printf("MQTT Server:");
  if (clean) { cleanField(col2DataX(), y); };
  display.setCursor(col2DataX(), y);
  display.print(plateCfg.mqttHost);
  // MQTT status
  y += lineHeight;
  display.setCursor(col2NameX(), y);
  display.printf("MQTT Status:");
  if (clean) { cleanField(col2DataX(), y); };
  display.setCursor(col2DataX(), y);
  display.printf(mqttConnected() ? "OK" : "Error");

  *yref = y;
}

void displayInfoScreen()
{
  Serial.println("[INFO] Rendering info page");
  static char buff[1024];
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);

  i2cStart();
  displayStart();
#ifdef INKPLATE_HAS_DISPLAY_MODES
  display.selectDisplayMode(INKPLATE_1BIT);
#endif
  display.setTextColor(HP_FG, HP_BG);
  display.clearDisplay();

  // Title
  display.setFont(&FONT_HEADING);
  display.setTextSize(1);
  uint32_t displayWidth = hpDisplayWidth();
  uint32_t displayHeight = hpDisplayHeight();
  uint32_t y = centerTextX("HomePlate Info", 0, displayWidth, hpScaleY(100), false);
  display.setFont(&FONT_BODY);
  // version
  snprintf(buff, 1024, "Version: [%s]", VERSION);
  y = centerTextX(buff, 0, displayWidth, y + hpScaleY(110), false);

  // column 1
  // Model
  y = hpScaleY(250);
  display.setCursor(col1NameX(), y);
  display.print("Model:");
  display.setCursor(col1DataX(), y);
  display.print(DEVICE_MODEL);
  // CPU
  y += lineHeight;
  display.setCursor(col1NameX(), y);
  display.print("CPU:");
  display.setCursor(col1DataX(), y);
  display.printf("%u core(s)", chip_info.cores);
  // frequency
  y += lineHeight;
  display.setCursor(col1NameX(), y);
  display.print("Frequency:");
  display.setCursor(col1DataX(), y);
  display.printf("%u Mhz", getCpuFrequencyMhz());
  // Features
  y += lineHeight;
  display.setCursor(col1NameX(), y);
  display.print("Features:");
  display.setCursor(col1DataX(), y);
  display.printf("%s%s%s", (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi" : "", (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "", (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");
  // Flash
  y += lineHeight;
  display.setCursor(col1NameX(), y);
  display.print("Flash:");
  display.setCursor(col1DataX(), y);
  display.printf("%dMB %s", ESP.getFlashChipSize() / (1024 * 1024), (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
  // Heap
  y += lineHeight;
  display.setCursor(col1NameX(), y);
  display.print("Min Heap Free:");
  display.setCursor(col1DataX(), y);
  display.printf("%.2fMB", (esp_get_minimum_free_heap_size()) / (1024.0F * 1024.0F));
  // Tasks
  y += lineHeight;
  display.setCursor(col1NameX(), y);
  display.print("Tasks:");
  display.setCursor(col1DataX(), y);
  display.printf("%d", uxTaskGetNumberOfTasks());
  // Display
  y += lineHeight;
  display.setCursor(col1NameX(), y);
  display.print("Display:");
  display.setCursor(col1DataX(), y);
  display.printf("%dx%d", displayWidth, displayHeight);
  // Dither
  y += lineHeight;
  display.setCursor(col1NameX(), y);
  display.print("Dither:");
  display.setCursor(col1DataX(), y);
  display.print(ditherKernelName(plateCfg.ditherKernel));

  // bootCount
  y += lineHeight * 2;
  display.setCursor(col1NameX(), y);
  display.print("Boot #:");
  display.setCursor(col1DataX(), y);
  display.printf("%u", bootCount);
  // loopCount
  y += lineHeight;
  display.setCursor(col1NameX(), y);
  display.print("Activity #:");
  display.setCursor(col1DataX(), y);
  display.printf("%u", activityCount);
  // wake reason
  y += lineHeight;
  display.setCursor(col1NameX(), y);
  display.print("Wake Reason:");
  display.setCursor(col1DataX(), y);
  display.printf("%s", bootReason());
  // battery
  y += lineHeight;
  double voltage = display.readBattery();
  int percent = getBatteryPercent(voltage);
  display.setCursor(col1NameX(), y);
  display.print("Battery:");
  display.setCursor(col1DataX(), y);
  display.printf("%d%% (%.2fv)", percent, voltage);
#ifdef INKPLATE_HAS_TEMPERATURE
  // temp
  y += lineHeight;
  int temp = display.readTemperature();
  int tempF = (temp * 9 / 5) + 32;
  display.setCursor(col1NameX(), y);
  display.print("Temperature:");
  display.setCursor(col1DataX(), y);
  display.printf("%dC (%dF)", temp, tempF);
#endif

  if (strlen(plateCfg.trmnlId) > 0) {
  // TRMNL
  y += lineHeight * 2;
  display.setCursor(col1NameX(), y);
  display.print("TRMNL ID:");
  display.setCursor(col1DataX(), y);
  display.print(plateCfg.trmnlId);
  y += lineHeight;
  display.setCursor(col1NameX(), y);
  display.print("TRMNL URL:");
  display.setCursor(col1DataX(), y);
  // Truncate to stay within the left column — otherwise on small displays
  // the URL bleeds rightward into the right column's data.
  display.print(truncateToFit(plateCfg.trmnlUrl, col2NameX() - col1DataX() - hpScaleX(20)));
  }

  // column 2
  y = hpScaleY(250);
  // network
  uint32_t yNetwork = y;
  drawNetwork(&y, false);

  // ssid
  y += lineHeight * 2;
  uint32_t yWiFi = y;
  bool stateWiFi = WiFi.isConnected();
  drawWiFi(&y, false);

  bool stateMQTT = true;
  uint32_t yMQTT = 0;
  if (strlen(plateCfg.mqttHost) > 0) {
  // MQTT server
  y += lineHeight * 2;
  yMQTT = y;
  stateMQTT = mqttConnected();
  drawMQTT(&y, false);
  }

  if (strlen(plateCfg.ntpServer) > 0) {
  // time
  y += lineHeight * 2;
  display.setCursor(col2NameX(), y);
  display.print("Time:");
  display.setCursor(col2DataX(), y);
  display.print(shortDateTimeString());
  // Timezone
  y += lineHeight;
  display.setCursor(col2NameX(), y);
  display.print("Timezone:");
  display.setCursor(col2DataX(), y);
  display.print(plateCfg.timezone);
  // NTP
  y += lineHeight;
  display.setCursor(col2NameX(), y);
  display.print("NTP Server:");
  display.setCursor(col2DataX(), y);
  display.print(plateCfg.ntpServer);
  // NTP status
  y += lineHeight;
  display.setCursor(col2NameX(), y);
  display.print("NTP Status:");
  display.setCursor(col2DataX(), y);
  display.print(getNTPSynced() ? "OK" : "False");

  // RTC
  y += lineHeight;
  display.setCursor(col2NameX(), y);
  display.printf("Ext RTC:");
  display.setCursor(col2DataX(), y);
  display.printf(display.rtc.isSet() ? "OK" : "Error");
  }

  displayBoundaryBox();
  displayRefresh();
  displayEnd();
  i2cEnd();

#ifdef INKPLATE_HAS_PARTIAL_UPDATE
  // check WiFi and MQTT state
  uint8_t needsRedraw = 0;
  for (int i = 0; i < 20; i++)
  {
    // only check if WiFi was not connected already
    if (!stateWiFi)
    {
      if (WiFi.isConnected())
      {
        // schedule redraw for network and wifi
	bitSet(needsRedraw, REDRAW_NETWORK);
	bitSet(needsRedraw, REDRAW_WIFI);
	stateWiFi = true;
      }
    }

    // only check if MQTT was not connected already
    if (strlen(plateCfg.mqttHost) > 0 && !stateMQTT)
    {
      if (mqttConnected())
      {
        // schedule redraw for mqtt
	bitSet(needsRedraw, REDRAW_MQTT);
	stateMQTT = true;
      }
    }

    // finish loop early if both are connected
    if (stateWiFi && stateMQTT)
    {
      break;
    }

    // wait 100ms
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }

  // check for redraw
  if (needsRedraw)
  {
    // start partial update
    i2cStart();
    displayStart();
    display.selectDisplayMode(INKPLATE_1BIT);
    display.setTextColor(HP_FG, HP_BG);
    display.setTextSize(1);
    display.setFont(&FONT_BODY);
    display.partialUpdate(sleepBoot);

    if (bitRead(needsRedraw, REDRAW_NETWORK))
    {
      // redraw network
      Serial.println("[INFO] Partial update: Network");
      drawNetwork(&yNetwork, true);
    }
    if (bitRead(needsRedraw, REDRAW_WIFI))
    {
      // redraw wifi
      Serial.println("[INFO] Partial update: WiFi");
      drawWiFi(&yWiFi, true);
    }
    if (strlen(plateCfg.mqttHost) > 0 && bitRead(needsRedraw, REDRAW_MQTT))
    {
      // redraw mqtt
      Serial.println("[INFO] Partial update: MQTT");
      drawMQTT(&yMQTT, true);
    }

    // send to display
    display.partialUpdate(sleepBoot);
    displayEnd();
    i2cEnd();
  }
#endif // INKPLATE_HAS_PARTIAL_UPDATE
}

#if ARDUINO_USB_MODE
#warning This sketch should be used when USB is in OTG mode
void setup() {}
void loop() {}
#else

#include <uptime.h>
#ifndef NO_USB_HID
#include <Adafruit_TinyUSB.h>
#endif

#include "Devices/Platform/BoardSupport.h"
#include "Devices/Button/HardwareButton.h"
#include "Devices/LED/HardwareLED.h"
#include "Devices/TFT/HardwareTFT.h"
#include "Devices/Storage/HardwareStorage.h"
#include "Devices/USB/USBCore.h"
#include "Devices/WiFi/HardwareWiFi.h"
#include "Devices/Microphone/HardwareMicrophone.h"
#include "Devices/IR/HardwareIR.h"
#include "Devices/Touch/HardwareTouch.h"

#include "Attacks/Marauder/Marauder.h"
#include "Attacks/Ducky/DuckyPayload.h"
#include "Attacks/Agent/Agent.h"

#include "Debug/Logging.h"
#include "AuxiliaryComponent/Auxiliary.h"

#include "Utilities/Format.h"
#include "version.h"

#define TAG "main"

static Preferences prefs;
static Auxiliary aux;

static int currentLine = 0;

// ── Multi-page display system ─────────────────────────────────────────────────
static int g_displayPage = 0;
static int g_animTick    = 0;
static unsigned long g_lastDisplayUpdate = 0;
#ifdef CYD_ESP32
  #define TOTAL_PAGES 4
#else
  #define TOTAL_PAGES 3
#endif
#define PAGE_REFRESH_MS 2000

static std::string pbar(int pct, int width = 8)
{
  if (pct < 0)   pct = 0;
  if (pct > 100) pct = 100;
  int filled = (pct * width) / 100;
  std::string bar = "[";
  for (int i = 0; i < width; i++) bar += (i < filled) ? '#' : '.';
  bar += "]";
  return bar;
}

static std::string clamp(const std::string& s, int maxLen)
{
  return s.size() > (size_t)maxLen ? s.substr(0, maxLen - 1) + "~" : s;
}

static const char* spinners[] = {"|", "/", "-", "\\"};

#ifdef CYD_ESP32
// ── CYD touch state ───────────────────────────────────────────────────────────
static std::vector<std::string> g_plList;
static int  g_plBrowserPage = 0;
static bool g_plLoaded      = false;

static void ensurePLLoaded()
{
  if (g_plLoaded) return;
  g_plList.clear();
  if (Devices::Storage.isRunning()) {
    for (const auto& f : Devices::Storage.listFiles()) {
      std::string fn(f.c_str());
      if (fn.size() > 3 && fn.substr(fn.size()-3) == ".ds")
        g_plList.push_back(fn);
    }
  }
  g_plLoaded = true;
}
#endif // CYD_ESP32

// ── Boot splash ───────────────────────────────────────────────────────────────
#ifndef NO_TFT
static void showBootSplash()
{
  const int BLACK  = Devices::TFT.convertStringToColor("BLACK");
  const int GREEN  = Devices::TFT.convertStringToColor("GREEN");
  const int WHITE  = Devices::TFT.convertStringToColor("WHITE");
  const int CYAN   = Devices::TFT.convertStringToColor("CYAN");
  const int TEAL   = Devices::TFT.convertStringToColor("DARKCYAN");

  Devices::TFT.setBackgroundColor(BLACK);
  Devices::TFT.clearScreen();

#ifdef CYD_ESP32
  // ── CYD 320×240 ────────────────────────────────────────────────────────────
  // Top accent bar
  Devices::TFT.setForegroundColor(GREEN);
  Devices::TFT.displayRectangle(0, 0, 320, 4);

  // Left accent stripe
  Devices::TFT.setForegroundColor(TEAL);
  Devices::TFT.displayRectangle(0, 4, 3, 236);

  // Right accent stripe
  Devices::TFT.displayRectangle(317, 4, 3, 236);

  // "G4" — large, green  (size 4 → 24×32px per char)
  Devices::TFT.setForegroundColor(GREEN);
  Devices::TFT.setTextSize(4);
  Devices::TFT.display(72, 30, "G4");

  // "MEOVER" — large, cyan  (size 4, same row)
  Devices::TFT.setForegroundColor(CYAN);
  Devices::TFT.display(72 + 2*24, 30, "MEOVER");

  // Underline below logo text
  Devices::TFT.setForegroundColor(GREEN);
  Devices::TFT.displayRectangle(60, 96, 200, 2);

  // "RadioRemote"  (size 2 → 12×16px per char, 11 chars = 132px, center = 94)
  Devices::TFT.setForegroundColor(WHITE);
  Devices::TFT.setTextSize(2);
  Devices::TFT.display(94, 110, "RadioRemote");

  // "by G4MEOVER18"  (size 1 → 6×8px, 18 chars = 108px, center = 106)
  Devices::TFT.setForegroundColor(TEAL);
  Devices::TFT.setTextSize(1);
  Devices::TFT.display(106, 134, "by G4MEOVER18");

  // Separator line
  Devices::TFT.setForegroundColor(TEAL);
  Devices::TFT.displayRectangle(60, 148, 200, 1);

  // Version
  Devices::TFT.setForegroundColor(TEAL);
  Devices::TFT.display(100, 158, std::string("v") + GIT_COMMIT_HASH);

  // Animated booting dots
  for (int i = 0; i < 4; i++)
  {
    Devices::TFT.setForegroundColor(GREEN);
    std::string dots(i + 1, '.');
    Devices::TFT.display(118, 200, "Booting" + dots + "   ");
    delay(300);
  }

  // Bottom accent bar
  Devices::TFT.setForegroundColor(GREEN);
  Devices::TFT.displayRectangle(0, 236, 320, 4);

  delay(600);
  Devices::TFT.setTextSize(1);
  Devices::TFT.setBackgroundColor(BLACK);
  Devices::TFT.clearScreen();

#else
  // ── T-Dongle 160×80 ────────────────────────────────────────────────────────
  // Top accent bar
  Devices::TFT.setForegroundColor(GREEN);
  Devices::TFT.displayRectangle(0, 0, 160, 2);

  // "G4" green, "MEOVER" cyan  (size 2 → 12×16px per char)
  Devices::TFT.setForegroundColor(GREEN);
  Devices::TFT.setTextSize(2);
  Devices::TFT.display(16, 8, "G4");
  Devices::TFT.setForegroundColor(CYAN);
  Devices::TFT.display(16 + 2*12, 8, "MEOVER");

  // Underline
  Devices::TFT.setForegroundColor(GREEN);
  Devices::TFT.displayRectangle(10, 28, 140, 1);

  // "RadioRemote"  (size 1 → 6×8px, 11 chars = 66px, center = 47)
  Devices::TFT.setForegroundColor(WHITE);
  Devices::TFT.setTextSize(1);
  Devices::TFT.display(47, 34, "RadioRemote");

  // "G4MEOVER18"  (14 chars × 6 = 84px, center = 38)
  Devices::TFT.setForegroundColor(TEAL);
  Devices::TFT.display(38, 46, "G4MEOVER18");

  // Animated dots
  for (int i = 0; i < 3; i++)
  {
    Devices::TFT.setForegroundColor(GREEN);
    std::string dots(i + 1, '.');
    Devices::TFT.display(44, 60, "Boot" + dots + "  ");
    delay(300);
  }

  // Bottom bar
  Devices::TFT.setForegroundColor(GREEN);
  Devices::TFT.displayRectangle(0, 78, 160, 2);

  delay(300);
  Devices::TFT.setTextSize(1);
  Devices::TFT.setBackgroundColor(BLACK);
  Devices::TFT.clearScreen();
#endif
}
#endif // NO_TFT

// ── renderDisplayPage ─────────────────────────────────────────────────────────
#ifndef NO_TFT
static void renderDisplayPage(int page, int tick)
{
  const int BLACK      = Devices::TFT.convertStringToColor("BLACK");
  const int WHITE      = Devices::TFT.convertStringToColor("WHITE");
  const int GREEN      = Devices::TFT.convertStringToColor("GREEN");
  const int CYAN       = Devices::TFT.convertStringToColor("CYAN");
  const int YELLOW     = Devices::TFT.convertStringToColor("YELLOW");
  const int ORANGE     = Devices::TFT.convertStringToColor("ORANGE");
  const int LIGHTGREY  = Devices::TFT.convertStringToColor("LIGHTGREY");
  const int TEAL       = Devices::TFT.convertStringToColor("DARKCYAN");
  const char* sp       = spinners[tick % 4];

  Devices::TFT.setBackgroundColor(BLACK);
  Devices::TFT.clearScreen();

#ifdef CYD_ESP32
  // ── CYD 320×240, 6px/char, 8px/line ───────────────────────────────────────
  // Use textSize=1 (6×8 px per char) → 53 chars wide, 30 rows tall
  Devices::TFT.setTextSize(1);

  if (page == 0)
  {
    // Page 0 — Dashboard
    Devices::TFT.setForegroundColor(GREEN);
    Devices::TFT.displayRectangle(0, 0, 320, 2);
    Devices::TFT.display(4, 4, "G4MEOVER RadioRemote");
    Devices::TFT.setForegroundColor(CYAN);
    Devices::TFT.display(258, 4, sp);
    Devices::TFT.setForegroundColor(TEAL);
    Devices::TFT.displayRectangle(0, 14, 320, 1);

    bool wifiUp = Devices::WiFi.getState();
    Devices::TFT.setForegroundColor(LIGHTGREY);
    Devices::TFT.display(4, 18, "WiFi:  ");
    Devices::TFT.setForegroundColor(wifiUp ? GREEN : ORANGE);
    Devices::TFT.display(64, 18, wifiUp ? "UP   4.3.2.1:8080" : "DOWN");

    bool sdUp = Devices::Storage.isRunning();
    Devices::TFT.setForegroundColor(LIGHTGREY);
    Devices::TFT.display(4, 28, "SD:    ");
    Devices::TFT.setForegroundColor(sdUp ? GREEN : ORANGE);
    Devices::TFT.display(64, 28, sdUp ? "OK" : "no card");

    std::string usbMode = "None";
    if (Devices::USB::Core.currentClassType() == USBClassType::HID)          usbMode = "HID";
    else if (Devices::USB::Core.currentClassType() == USBClassType::Storage)  usbMode = "Storage";
    else if (Devices::USB::Core.currentDeviceType() == USBDeviceType::NCM)   usbMode = "NCM";
    else if (Devices::USB::Core.currentDeviceType() == USBDeviceType::Serial) usbMode = "Serial";
    Devices::TFT.setForegroundColor(LIGHTGREY);
    Devices::TFT.display(4, 38, "USB:   ");
    Devices::TFT.setForegroundColor(WHITE);
    Devices::TFT.display(64, 38, usbMode + " (no HID on CYD)");

    uint32_t heapFree = ESP.getFreeHeap();
    uint32_t heapTot  = ESP.getHeapSize();
    int heapPct = heapTot > 0 ? (int)((heapFree * 100UL) / heapTot) : 0;
    Devices::TFT.setForegroundColor(LIGHTGREY);
    Devices::TFT.display(4, 48, "RAM:   ");
    Devices::TFT.setForegroundColor(heapPct > 30 ? GREEN : ORANGE);
    Devices::TFT.display(64, 48, pbar(heapPct) + " " + std::to_string(heapFree/1024) + "/" + std::to_string(heapTot/1024) + "k");

    std::string upStr = std::to_string(uptime::getDays()) + "d "
                      + std::to_string(uptime::getHours()) + "h "
                      + std::to_string(uptime::getMinutes()) + "m "
                      + std::to_string(uptime::getSeconds()) + "s";
    Devices::TFT.setForegroundColor(LIGHTGREY);
    Devices::TFT.display(4, 58, "Uptime:");
    Devices::TFT.setForegroundColor(WHITE);
    Devices::TFT.display(64, 58, upStr);

    std::string pSt = Attacks::Ducky.getPayloadRunningStatus();
    Devices::TFT.setForegroundColor(LIGHTGREY);
    Devices::TFT.display(4, 68, "Payload:");
    Devices::TFT.setForegroundColor(pSt == "Running" ? YELLOW : (pSt == "Error" ? ORANGE : LIGHTGREY));
    Devices::TFT.display(64, 68, pSt);

    Devices::TFT.setForegroundColor(TEAL);
    Devices::TFT.displayRectangle(0, 78, 320, 1);

    Devices::TFT.setForegroundColor(CYAN);
    Devices::TFT.display(4, 81, "Network");
    Devices::TFT.setForegroundColor(LIGHTGREY);
    Devices::TFT.display(4, 91,  "SSID:  USB-Penetrator");
    Devices::TFT.display(4, 101, "IP:    4.3.2.1        Port: 8080");
    Devices::TFT.display(4, 111, "WebUI: http://4.3.2.1:8080");

    Devices::TFT.setForegroundColor(TEAL);
    Devices::TFT.displayRectangle(0, 122, 320, 1);

    Devices::TFT.setForegroundColor(CYAN);
    Devices::TFT.display(4, 125, "System");
    Devices::TFT.setForegroundColor(LIGHTGREY);
    Devices::TFT.display(4, 135, "Board:   CYD ESP32-2432S028R");
    Devices::TFT.display(4, 145, "Chip:    ESP32 @ 240MHz");
    Devices::TFT.display(4, 155, std::string("FW:      ") + GIT_COMMIT_HASH);

    Devices::TFT.setForegroundColor(TEAL);
    Devices::TFT.displayRectangle(0, 165, 320, 1);

    Devices::TFT.setForegroundColor(TEAL);
    Devices::TFT.display(4, 168, "Touch -> Quick Launch   BTN -> Next Page");
    Devices::TFT.setForegroundColor(LIGHTGREY);
    Devices::TFT.display(4, 180, "by G4MEOVER18");

    Devices::TFT.setForegroundColor(GREEN);
    Devices::TFT.displayRectangle(0, 238, 320, 2);
  }
  else if (page == 1)
  {
    // Page 1 — Payload Status
    // Quick Launch — 3 cols x 2 rows of touch buttons
    // Col x0=(i%3)*107+1, bw=105; Row y0=16+(i/3)*112, bh=110
    Devices::TFT.setForegroundColor(GREEN);
    Devices::TFT.displayRectangle(0, 0, 320, 2);
    Devices::TFT.display(4, 4, "QUICK LAUNCH");
    Devices::TFT.setForegroundColor(TEAL);
    Devices::TFT.display(108, 4, "Touch to run  |  BTN=next page");
    Devices::TFT.displayRectangle(0, 14, 320, 1);

    static const char* btnL1[] = { "WiFi",    "BLE",    "Deauth",  "Ward-",   "Evil",    "Payload" };
    static const char* btnL2[] = { "Scan",    "Spam",   "All",     "rive",    "Portal",  "List >"  };
    const int btnColors[6]     = { GREEN,      CYAN,     ORANGE,    GREEN,     ORANGE,    CYAN      };

    for (int i = 0; i < 6; i++)
    {
      int bx = (i % 3) * 107 + 1;
      int by = 16 + (i / 3) * 112;
      int bw = 105;
      int bh = 110;

      Devices::TFT.setForegroundColor(btnColors[i]);
      Devices::TFT.displayRectangle(bx,          by,          bw, 2);
      Devices::TFT.displayRectangle(bx,          by + bh - 2, bw, 2);
      Devices::TFT.displayRectangle(bx,          by + 2,      2,  bh - 4);
      Devices::TFT.displayRectangle(bx + bw - 2, by + 2,      2,  bh - 4);

      Devices::TFT.setTextSize(2);
      std::string l1(btnL1[i]);
      std::string l2(btnL2[i]);
      int textTop = by + (bh - 34) / 2;
      Devices::TFT.setForegroundColor(btnColors[i]);
      Devices::TFT.display(bx + (bw - (int)l1.size() * 12) / 2, textTop,      l1);
      Devices::TFT.display(bx + (bw - (int)l2.size() * 12) / 2, textTop + 18, l2);
    }
    Devices::TFT.setTextSize(1);

    Devices::TFT.setForegroundColor(GREEN);
    Devices::TFT.displayRectangle(0, 238, 320, 2);
  }
  else if (page == 2)
  {
    // Page 2 — Network
    // Payload Browser — 5 items per page, touch to run
    // Items at y=15+(i*40), nav bar at y=215
    ensurePLLoaded();
    int total    = (int)g_plList.size();
    int perPage  = 5;
    int startIdx = g_plBrowserPage * perPage;
    int numPages = total > 0 ? ((total + perPage - 1) / perPage) : 1;

    Devices::TFT.setForegroundColor(GREEN);
    Devices::TFT.displayRectangle(0, 0, 320, 2);
    Devices::TFT.display(4, 4, "PAYLOAD BROWSER");
    Devices::TFT.setForegroundColor(TEAL);
    Devices::TFT.display(220, 4, "pg " + std::to_string(g_plBrowserPage + 1) + "/" + std::to_string(numPages));
    Devices::TFT.displayRectangle(0, 14, 320, 1);

    if (total == 0)
    {
      Devices::TFT.setForegroundColor(ORANGE);
      Devices::TFT.display(4, 30, "No .ds payloads found on SD.");
      Devices::TFT.display(4, 42, "Insert SD with payload files.");
    }
    else
    {
      for (int i = 0; i < perPage; i++)
      {
        int realIdx = startIdx + i;
        int iy      = 15 + i * 40;

        Devices::TFT.setForegroundColor(TEAL);
        Devices::TFT.displayRectangle(0, iy, 320, 1);

        if (realIdx < total)
        {
          std::string fn = g_plList[realIdx];
          if (!fn.empty() && fn[0] == '/') fn = fn.substr(1);
          if (fn.size() > 3 && fn.substr(fn.size() - 3) == ".ds")
            fn = fn.substr(0, fn.size() - 3);
          if (fn.size() > 34) fn = fn.substr(0, 33) + "~";

          Devices::TFT.setForegroundColor(WHITE);
          Devices::TFT.display(4, iy + 5, fn);
          Devices::TFT.setForegroundColor(GREEN);
          Devices::TFT.display(288, iy + 5, "RUN");
          Devices::TFT.setForegroundColor(TEAL);
          Devices::TFT.display(4, iy + 18, "> touch row to run");
        }
        else
        {
          Devices::TFT.setForegroundColor(LIGHTGREY);
          Devices::TFT.display(4, iy + 5, "---");
        }
      }
    }

    Devices::TFT.setForegroundColor(TEAL);
    Devices::TFT.displayRectangle(0, 215, 320, 1);
    Devices::TFT.setForegroundColor(startIdx > 0 ? GREEN : LIGHTGREY);
    Devices::TFT.display(4, 219, "< PREV");
    Devices::TFT.setForegroundColor(TEAL);
    Devices::TFT.display(134, 219, "BTN=back");
    Devices::TFT.setForegroundColor(startIdx + perPage < total ? GREEN : LIGHTGREY);
    Devices::TFT.display(248, 219, "NEXT >");

    Devices::TFT.setForegroundColor(GREEN);
    Devices::TFT.displayRectangle(0, 238, 320, 2);
  }
  else if (page == 3)
  {
    // Page 3 — System
    Devices::TFT.setForegroundColor(YELLOW);
    Devices::TFT.displayRectangle(0, 0, 320, 2);
    Devices::TFT.display(4, 4, "SYSTEM INFO");
    Devices::TFT.display(306, 4, sp);
    Devices::TFT.setForegroundColor(TEAL);
    Devices::TFT.displayRectangle(0, 14, 320, 1);

    Devices::TFT.setForegroundColor(LIGHTGREY);
    Devices::TFT.display(4, 18, "Board:   CYD ESP32-2432S028R");
    Devices::TFT.display(4, 28, "Chip:    ESP32 Xtensa LX6 @ 240MHz");
    Devices::TFT.display(4, 38, "Flash:   4MB SPI");
    Devices::TFT.display(4, 48, "Display: ILI9341  320x240  Touch");
    Devices::TFT.display(4, 58, "Touch:   XPT2046 resistive");
    Devices::TFT.display(4, 68, "SD slot: SPI (VSPI bus)");
    Devices::TFT.display(4, 78, "USB HID: not supported on CYD");

    Devices::TFT.setForegroundColor(TEAL);
    Devices::TFT.displayRectangle(0, 90, 320, 1);

    uint32_t heapFree = ESP.getFreeHeap();
    uint32_t heapTot  = ESP.getHeapSize();
    int heapPct = heapTot > 0 ? (int)((heapFree * 100UL) / heapTot) : 0;
    Devices::TFT.setForegroundColor(LIGHTGREY);
    Devices::TFT.display(4, 94,  "RAM: " + std::to_string(heapFree/1024) + "k free / " + std::to_string(heapTot/1024) + "k total");
    Devices::TFT.display(4, 104, "     " + pbar(heapPct, 24) + " " + std::to_string(heapPct) + "%");

    uint32_t psramFree = ESP.getFreePsram();
    uint32_t psramTot  = ESP.getPsramSize();
    if (psramTot > 0)
      Devices::TFT.display(4, 114, "PSRAM: " + std::to_string(psramFree/1024) + "k / " + std::to_string(psramTot/1024) + "k");

    std::string upStr = std::to_string(uptime::getDays()) + "d "
                      + std::to_string(uptime::getHours()) + "h "
                      + std::to_string(uptime::getMinutes()) + "m "
                      + std::to_string(uptime::getSeconds()) + "s";

    Devices::TFT.setForegroundColor(TEAL);
    Devices::TFT.displayRectangle(0, 126, 320, 1);

    Devices::TFT.setForegroundColor(LIGHTGREY);
    Devices::TFT.display(4, 130, "Uptime:  " + upStr);
    Devices::TFT.display(4, 140, std::string("Version: ") + GIT_COMMIT_HASH);
    Devices::TFT.display(4, 150, "AP SSID: USB-Penetrator");
    Devices::TFT.display(4, 160, "AP IP:   4.3.2.1:8080");

    Devices::TFT.setForegroundColor(TEAL);
    Devices::TFT.displayRectangle(0, 172, 320, 1);

    Devices::TFT.setTextSize(2);
    Devices::TFT.setForegroundColor(CYAN);
    Devices::TFT.display(4, 176, "by G4MEOVER18");
    Devices::TFT.setTextSize(1);
    Devices::TFT.setForegroundColor(TEAL);
    Devices::TFT.display(4, 198, "G4MEOVER RadioRemote");
    Devices::TFT.display(4, 208, "github.com/G4MEOVER18");

    Devices::TFT.setForegroundColor(YELLOW);
    Devices::TFT.displayRectangle(0, 238, 320, 2);
  }

#else
  // ── T-Dongle 160×80 ─────────────────────────────────────────────────────────
  Devices::TFT.setTextSize(1);

  if (page == 0)
  {
    // Page 0 — Status
    Devices::TFT.setForegroundColor(GREEN);
    Devices::TFT.display(0, 0,  "G4MEOVER RadioRemote");

    bool wifiUp = Devices::WiFi.getState();
    Devices::TFT.setForegroundColor(LIGHTGREY);
    Devices::TFT.display(0, 12, "WiFi:");
    Devices::TFT.setForegroundColor(wifiUp ? GREEN : ORANGE);
    Devices::TFT.display(36, 12, wifiUp ? "UP 4.3.2.1" : "DOWN");

    bool sdUp = Devices::Storage.isRunning();
    Devices::TFT.setForegroundColor(LIGHTGREY);
    Devices::TFT.display(0, 24, "SD:");
    Devices::TFT.setForegroundColor(sdUp ? GREEN : ORANGE);
    Devices::TFT.display(36, 24, sdUp ? "OK" : "none");

    std::string usbMode = "None";
    if (Devices::USB::Core.currentClassType() == USBClassType::HID)          usbMode = "HID";
    else if (Devices::USB::Core.currentClassType() == USBClassType::Storage)  usbMode = "Storage";
    else if (Devices::USB::Core.currentDeviceType() == USBDeviceType::NCM)   usbMode = "NCM";
    else if (Devices::USB::Core.currentDeviceType() == USBDeviceType::Serial) usbMode = "Serial";
    Devices::TFT.setForegroundColor(LIGHTGREY);
    Devices::TFT.display(0, 36, "USB:" + usbMode);

    std::string upStr = std::to_string(uptime::getHours())+"h "
                       +std::to_string(uptime::getMinutes())+"m";
    Devices::TFT.setForegroundColor(LIGHTGREY);
    Devices::TFT.display(0, 48, "Up:" + upStr);

    Devices::TFT.setForegroundColor(GREEN);
    Devices::TFT.display(154, 72, sp);
  }
  else if (page == 1)
  {
    // Page 1 — Payload
    Devices::TFT.setForegroundColor(ORANGE);
    Devices::TFT.display(0, 0,  "PAYLOAD");

    std::string pSt = Attacks::Ducky.getPayloadRunningStatus();
    bool running = (pSt == "Running");
    Devices::TFT.setForegroundColor(running ? YELLOW : GREEN);
    Devices::TFT.display(0, 12, pSt);

    uint32_t heapFree = ESP.getFreeHeap();
    uint32_t heapTot  = ESP.getHeapSize();
    int heapPct = heapTot > 0 ? (int)((heapFree * 100UL) / heapTot) : 0;
    Devices::TFT.setForegroundColor(LIGHTGREY);
    Devices::TFT.display(0, 44, "RAM:" + pbar(heapPct, 6) + std::to_string(heapFree/1024) + "k");

    Devices::TFT.setForegroundColor(ORANGE);
    Devices::TFT.display(154, 72, sp);
  }
  else if (page == 2)
  {
    // Page 2 — System
    Devices::TFT.setForegroundColor(CYAN);
    Devices::TFT.display(0, 0,  "SYSTEM");

    Devices::TFT.setForegroundColor(LIGHTGREY);
    Devices::TFT.display(0, 12, "Board: T-Dongle-S3");
    Devices::TFT.display(0, 24, std::string("Ver: ") + GIT_COMMIT_HASH);

    std::string upStr = std::to_string(uptime::getDays())+"d "
                       +std::to_string(uptime::getHours())+"h "
                       +std::to_string(uptime::getMinutes())+"m";
    Devices::TFT.display(0, 36, "Up:" + upStr);
    Devices::TFT.display(0, 48, "G4MEOVER18");

    Devices::TFT.setForegroundColor(CYAN);
    Devices::TFT.display(154, 72, sp);
  }
#endif // CYD_ESP32
}
#endif // NO_TFT
// ─────────────────────────────────────────────────────────────────────────────

static void displayMessage(const char* heading, const char* value = nullptr, bool warning = false)
{
  const auto WHITE = Devices::TFT.convertStringToColor("WHITE");
  const auto LIGHTGREY = Devices::TFT.convertStringToColor("LIGHTGREY");
  const auto RED = Devices::TFT.convertStringToColor("RED");

  Devices::TFT.setForegroundColor(LIGHTGREY);
  Devices::TFT.display(0, currentLine * 8, heading);

  if (value != nullptr)
  {
    Devices::TFT.setForegroundColor(warning ? RED : WHITE);
    Devices::TFT.display((strlen(heading) + 1) * 6, currentLine * 8, value);
  }

  Devices::TFT.setForegroundColor(WHITE);
  currentLine++;
}

void setup()
{
  // first thing, tear USB down. We don't know what state the
  // boot loader could have been left it in. Stop responding and the host OS should
  // forget about us and tear down
#ifndef NO_USB_HID
  tud_disconnect();
#endif

  prefs.begin("usbarmyknife");

#ifdef CYD_ESP32
  Serial.begin(115200); // early serial for CYD debug (before TFT init)
  // CYD has no native USB — force USB device/class prefs to None so display shows correct state
  prefs.putUShort("usbDeviceType", (uint16_t)USBDeviceType::None);
  prefs.putUShort("usbClassType",  (uint16_t)USBClassType::None);
#endif

  // First set up our core components / hw
  Debug::Log.begin(prefs);

  // set up underlying platform hardware
  Devices::Board.begin(prefs);

  Devices::Storage.begin(prefs);
#ifdef CYD_ESP32
  Serial.printf("[DBG] Storage done, running=%d\n", Devices::Storage.isRunning());
#endif
  // ESP32 Marauder uses a BT library that gets stuck in an infinite loop if it
  // fails to init. We init Marauder early as this means we should have as few tasks
  // as possible up in the air
  // If you plug in and don't see any LEDs, try commenting this line out
#ifndef CYD_ESP32
  Attacks::Marauder.begin(prefs);
#endif

  Devices::TFT.begin(prefs);
#ifndef NO_TFT
  showBootSplash();
#endif
  Devices::LED.begin(prefs);
  Devices::Button.begin(prefs);
  Devices::Mic.begin(prefs);

  Devices::USB::Core.begin(prefs);
  Devices::WiFi.begin(prefs);
#ifdef CYD_ESP32
  Serial.println("[DBG] WiFi done");
#endif
  Devices::IR.begin(prefs);
  Devices::Touch.begin(prefs);

  Attacks::Ducky.begin(prefs);
  Attacks::Agent.begin(prefs);

  if (!Devices::Storage.isRunning())
  {
#ifndef NO_SD
  #ifdef CYD_ESP32
    // CYD: no SD card — log only, keep running (WiFi/WebUI still works without SD)
    Debug::Log.error(TAG, "SD card not found — DuckyScript payloads unavailable");
  #else
    AskFormatSD(prefs);
  #endif
#endif
  }
  else if (!Attacks::Marauder.isRunning())
  {
#ifndef CYD_ESP32
    // Most users won't see this error as devices without an SD won't have a screen either
    Devices::TFT.display(0, 0, "Error FlashFS invalid");
    for (int x = 0; x < 5; x++)
    {
      // They might see/report some debug output
      Devices::LED.changeLEDState(true, 0, 100, 100, x % 2 == 0 ? 255 : 0); // flash RED led
      Debug::Log.error(TAG, "Flash filesystem is invalid, upload new FS image");
      delay(1000);
    }
#ifdef ARDUINO_ARCH_ESP32
    // Other platforms don't implement ESP32 Marauder so we don't have to worry about a pi
    ESP.restart();
#endif
#endif // CYD_ESP32
  }

  displayMessage("Device now running");
  Debug::Log.info(TAG, "Running!");

  if (Devices::USB::Core.currentDeviceType() == USBDeviceType::Serial)
  {
    displayMessage("USB MODE:", "Serial");
  }
  else if (Devices::USB::Core.currentDeviceType() == USBDeviceType::NCM)
  {
    displayMessage("USB MODE:", "NCM");
  }
  else
  {
    displayMessage("USB MODE:", "Disabled", true);
  }

  if (Devices::USB::Core.currentClassType() == USBClassType::HID)
  {
    displayMessage("USB CLASS:", "HID");
  }
  else if (Devices::USB::Core.currentClassType() == USBClassType::Storage)
  {
    displayMessage("USB CLASS:", "Storage");
  }
  else
  {
    displayMessage("USB CLASS:", "None", true);
  }

  Debug::Log.info(TAG, DEVICE_MAKE_MODEL);

  displayMessage("Version:", GIT_COMMIT_HASH);
  Debug::Log.info(TAG, std::string("Version: ")+GIT_COMMIT_HASH);

#ifdef CYD_ESP32
  displayMessage("WebUI:", "4.3.2.1:8080");
  displayMessage("SD:", Devices::Storage.isRunning() ? "OK" : "not found", !Devices::Storage.isRunning());
#endif

  aux.begin(prefs);
#ifdef CYD_ESP32
  Serial.println("[DBG] setup complete");
#endif
}

#ifdef CYD_ESP32
static void handleCYDTouch()
{
  int16_t tx, ty;
  if (!Devices::TFT.getTouch(tx, ty)) return;

  if (g_displayPage == 1)
  {
    // Quick Launch — check which button was touched
    // Button geometry: col x0=(i%3)*107+1, bw=105; row y0=16+(i/3)*112, bh=110
    for (int i = 0; i < 6; i++)
    {
      int bx = (i % 3) * 107 + 1;
      int by = 16 + (i / 3) * 112;
      if (tx >= bx && tx < bx + 105 && ty >= by && ty < by + 110)
      {
        if (i == 5)
        {
          // "Payload List" — go to browser
          g_plLoaded = false;
          g_plBrowserPage = 0;
          g_displayPage = 2;
        }
        else
        {
          static const char* quickPayloads[] = {
            "/wifi_scan_ap.ds",
            "/ble_spam_all.ds",
            "/wifi_deauth_all.ds",
            "/wifi_wardrive.ds",
            "/wifi_evil_portal.ds"
          };
          Attacks::Ducky.setPayload(quickPayloads[i]);
          g_displayPage = 0;
        }
        renderDisplayPage(g_displayPage, g_animTick);
        g_lastDisplayUpdate = millis();
        return;
      }
    }
  }
  else if (g_displayPage == 2)
  {
    // Payload Browser — nav or item tap
    ensurePLLoaded();
    int total    = (int)g_plList.size();
    int perPage  = 5;
    int startIdx = g_plBrowserPage * perPage;

    if (ty >= 215)
    {
      // Nav bar: left half = prev, right half = next
      if (tx < 160 && startIdx > 0)
        g_plBrowserPage--;
      else if (tx >= 160 && startIdx + perPage < total)
        g_plBrowserPage++;
    }
    else if (ty >= 15)
    {
      // Item rows: y=15+(i*40), height=40px each
      int itemIdx = (ty - 15) / 40;
      int realIdx = startIdx + itemIdx;
      if (itemIdx < perPage && realIdx < total)
      {
        Attacks::Ducky.setPayload(g_plList[realIdx]);
        g_displayPage = 0;
      }
    }
    renderDisplayPage(g_displayPage, g_animTick);
    g_lastDisplayUpdate = millis();
  }
  else
  {
    // Any touch on dashboard/system page -> go to Quick Launch
    g_displayPage = 1;
    renderDisplayPage(g_displayPage, g_animTick);
    g_lastDisplayUpdate = millis();
  }
}
#endif // CYD_ESP32

void loop()
{
  uptime::calculateUptime();
  Devices::Board.loop(prefs);

  Debug::Log.loop(prefs);

  Devices::USB::Core.loop(prefs);
  Devices::WiFi.loop(prefs);
  Devices::Storage.loop(prefs);
  Devices::Button.loop(prefs);
#ifndef NO_TFT
  if (Devices::Button.hasButtonBeenPressed())
  {
    Devices::Button.resetButtonPressedState();
    g_displayPage = (g_displayPage + 1) % TOTAL_PAGES;
    renderDisplayPage(g_displayPage, ++g_animTick);
    g_lastDisplayUpdate = millis();
  }
  else
  {
    unsigned long nowMs = millis();
    if (nowMs - g_lastDisplayUpdate >= PAGE_REFRESH_MS)
    {
      g_animTick++;
      renderDisplayPage(g_displayPage, g_animTick);
      g_lastDisplayUpdate = nowMs;
    }
  }
#endif
#ifdef CYD_ESP32
  handleCYDTouch();
#endif
  Devices::LED.loop(prefs);
  Devices::TFT.loop(prefs);
  Devices::Mic.loop(prefs);
  Devices::IR.loop(prefs);
  Devices::Touch.loop(prefs);

  Attacks::Ducky.loop(prefs);
#ifndef CYD_ESP32
  Attacks::Marauder.loop(prefs);
#endif
  Attacks::Agent.loop(prefs);

  aux.loop(prefs);
}

#endif /* ARDUINO_USB_MODE */

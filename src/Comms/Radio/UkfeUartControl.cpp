#ifdef G4MEOVER_UKFE_UART

#include "UkfeUartControl.h"
#include <ArduinoJson.h>
#include "../../Attacks/Ducky/DuckyPayload.h"
#include "../../Debug/Logging.h"

#define TAG "UkfeUart"

static HardwareSerial uart(UKFE_UART_NUM);
static String lineBuf;

// Payload-Tabelle — identisch zur RF-Variante (ein Vokabular).
static const char *payloadTable[] = {
    "/wifi_scan_ap.ds", "/wifi_deauth_all.ds", "/ble_spam_all.ds",
    "/wifi_evil_portal.ds", "/wifi_wardrive.ds"};

// Verarbeitet eine JSON-Zeile aus dem Flipper (uart_comm.c-Format).
static void handleJson(const String &line)
{
    JsonDocument doc;
    if (deserializeJson(doc, line))
        return; // Parse-Fehler ignorieren
    const char *cmd = doc["cmd"] | "";

    if (strcmp(cmd, "trigger") == 0)
    {
        uint8_t id = doc["id"] | 0;
        if (id < (sizeof(payloadTable) / sizeof(payloadTable[0])))
            Attacks::Ducky.setPayload(payloadTable[id]);
    }
    else if (strcmp(cmd, "wifi_scan") == 0)
    {
        Attacks::Ducky.setPayload("/wifi_scan_ap.ds");
    }
    else if (strcmp(cmd, "wifi_deauth") == 0)
    {
        Attacks::Ducky.setPayload("/wifi_deauth_all.ds");
    }
    else if (strcmp(cmd, "abort") == 0)
    {
        Attacks::Ducky.setPayload(""); // laufende Payload stoppen
    }
    // status / payload_list / lora_scan: nur quittieren (WiFi/WebUI liefert Details)
}

namespace Comms
{
    UkfeUartControl UkfeUart;

    void UkfeUartControl::begin(Preferences &prefs)
    {
        (void)prefs;
        uart.begin(115200, SERIAL_8N1, UKFE_UART_RX, UKFE_UART_TX);
        Debug::Log.info(TAG, "ukfe UART/JSON bereit");
    }

    void UkfeUartControl::loop(Preferences &prefs)
    {
        (void)prefs;
        while (uart.available())
        {
            char c = (char)uart.read();
            if (c == '\n')
            {
                if (lineBuf.length() > 0)
                {
                    handleJson(lineBuf);
                    lineBuf = "";
                }
            }
            else if (c != '\r')
            {
                if (lineBuf.length() < 256)
                    lineBuf += c;
            }
        }
    }
}

#endif // G4MEOVER_UKFE_UART

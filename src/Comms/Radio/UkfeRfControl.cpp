#ifdef G4MEOVER_UKFE_RF

#include "UkfeRfControl.h"
#include <SPI.h>
#include <RadioLib.h>
#include "../../Attacks/Ducky/DuckyPayload.h"
#include "../../Debug/Logging.h"

extern "C" {
#include "ukfe_rf.h"
}

#define TAG "UkfeRf"

// Heltec WiFi LoRa 32 V3 — SX1262 an HSPI (Pins via platformio build_flags)
static SPIClass loraSpi(HSPI);
static SX1262 radio = new Module(LORA_NSS, LORA_DIO1, LORA_RST, LORA_BUSY, loraSpi);

// Geheimnis MUSS mit RF_SECRET in lora-ukfe/rf/rf_comm.c uebereinstimmen.
static const uint8_t UKFE_SECRET[UKFE_RF_SECRET_LEN] = {
    0x47, 0x34, 0x4D, 0x45, 0x4F, 0x56, 0x45, 0x52,
    0xA5, 0x5A, 0xC3, 0x3C, 0x0F, 0xF0, 0x99, 0x66,
};

static volatile bool s_rxFlag = false;
IRAM_ATTR static void onDio1() { s_rxFlag = true; }

// Funk-Befehl -> vorhandene .ds-Payload / Ducky-HID (dieselben Pfade wie die Touch-UI).
static void dispatch(const UkfeRfMessage *m)
{
    switch (m->cmd)
    {
    case UkfeRfCmdWifiDeauth:
        Attacks::Ducky.setPayload("/wifi_deauth_all.ds");
        break;
    case UkfeRfCmdEvilPortal:
        Attacks::Ducky.setPayload("/wifi_evil_portal.ds");
        break;
    case UkfeRfCmdBeaconSpam:
        Attacks::Ducky.setPayload("/ble_spam_all.ds");
        break;
    case UkfeRfCmdTrigger:
    case UkfeRfCmdPayloadRun:
    {
        // id/idx -> Payload-Tabelle (WebUI-artiges Funk-Menue)
        static const char *table[] = {
            "/wifi_scan_ap.ds", "/wifi_deauth_all.ds", "/ble_spam_all.ds",
            "/wifi_evil_portal.ds", "/wifi_wardrive.ds"};
        uint8_t id = (m->arg_len > 0) ? m->args[0] : 0;
        if (id < (sizeof(table) / sizeof(table[0])))
            Attacks::Ducky.setPayload(table[id]);
        else
            Attacks::Ducky.setPayloadCmdLine(std::string("STRING G4MEOVER-HID online"));
        break;
    }
    default:
        break; // Status/Unbekannt: nur ignorieren
    }
}

namespace Comms
{
    UkfeRfControl UkfeRf;

    void UkfeRfControl::begin(Preferences &prefs)
    {
        (void)prefs;
        loraSpi.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);
        int st = radio.beginFSK(
            UKFE_RF_FREQUENCY_HZ / 1e6f, // MHz
            UKFE_RF_BITRATE / 1000.0f,   // kbps
            47.6f,                       // Dev kHz (Flipper-Preset 2FSKDev476)
            117.3f,                      // RX-BW kHz
            10, 16);
        if (st != RADIOLIB_ERR_NONE)
        {
            Debug::Log.error(TAG, "SX1262 beginFSK failed");
            return;
        }
        radio.setTCXO(1.8);
        radio.setDio2AsRfSwitch(true);
        uint8_t sync[2] = {(UKFE_RF_SYNC_WORD >> 8) & 0xFF, UKFE_RF_SYNC_WORD & 0xFF};
        radio.setSyncWord(sync, 2);
        radio.fixedPacketLengthMode(UKFE_RF_MAX_FRAME);
        radio.setCRC(0); // CRC macht ukfe_rf selbst
        radio.setDio1Action(onDio1);
        radio.startReceive();
        _ready = true;
        Debug::Log.info(TAG, "ukfe_rf RX bereit (868.35 MHz 2FSK)");
    }

    void UkfeRfControl::loop(Preferences &prefs)
    {
        (void)prefs;
        if (!_ready || !s_rxFlag)
            return;
        s_rxFlag = false;

        uint8_t frame[UKFE_RF_MAX_FRAME];
        int len = radio.readData(frame, UKFE_RF_MAX_FRAME);
        if (len == RADIOLIB_ERR_NONE)
        {
            UkfeRfMessage msg;
            size_t real_len = (size_t)frame[0] + 1; // echte Framelaenge (LEN+1)
            if (real_len > UKFE_RF_MAX_FRAME)
                real_len = UKFE_RF_MAX_FRAME;
            if (ukfe_rf_parse_frame(UKFE_SECRET, frame, real_len, &msg, &_lastCounter))
            {
                Debug::Log.info(TAG, "gueltiger Funk-Befehl");
                dispatch(&msg);
            }
        }
        radio.startReceive();
    }
}

#endif // G4MEOVER_UKFE_RF

#pragma once
#ifdef G4MEOVER_UKFE_UART

#include <Preferences.h>
#include "../../USBArmyKnifeCapability.h"

// G4MEOVER: WROOM-WiFi-Arm. Empfaengt das vorhandene Flipper-JSON-UART-Protokoll
// ({"cmd":"trigger","id":N}, {"cmd":"wifi_scan"}, ...) und loest dieselben
// .ds-Payloads aus wie die RF-Variante — ein Vokabular ueber Funk UND UART.
namespace Comms
{
    class UkfeUartControl : public USBArmyKnifeCapability
    {
    public:
        void begin(Preferences &prefs) override;
        void loop(Preferences &prefs) override;
    };

    extern UkfeUartControl UkfeUart;
}

#endif // G4MEOVER_UKFE_UART

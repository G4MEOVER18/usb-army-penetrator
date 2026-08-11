#pragma once
#ifdef G4MEOVER_UKFE_RF

#include <Preferences.h>
#include "../../USBArmyKnifeCapability.h"

// G4MEOVER: Funk-Fernsteuerung (868-FSK ukfe_rf, SX1262) fuer den Heltec.
// Empfaengt signierte Befehle vom Flipper-RF-Console und loest die vorhandenen
// .ds-Payloads / Ducky-HID ueber Attacks::Ducky aus. Reiht sich als globales
// Capability-Objekt in main.cpp ein (begin/loop), wie Attacks::Ducky.
namespace Comms
{
    class UkfeRfControl : public USBArmyKnifeCapability
    {
    public:
        void begin(Preferences &prefs) override;
        void loop(Preferences &prefs) override;

    private:
        bool _ready = false;
        uint32_t _lastCounter = 0; // Anti-Replay-Fenster
    };

    extern UkfeRfControl UkfeRf;
}

#endif // G4MEOVER_UKFE_RF

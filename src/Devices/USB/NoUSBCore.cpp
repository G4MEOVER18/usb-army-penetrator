#ifdef NO_USB_HID
// Stub implementations for boards without native USB (e.g. CYD ESP32-2432S028R)
// All USB operations silently do nothing.

#include "USBCore.h"
#include "USBCDC.h"
#include "USBHID.h"
#include "USBMSC.h"
#include "USBNCM.h"

#include <functional>
#include <string>

// ─── USBCore ──────────────────────────────────────────────────────────────────

namespace Devices::USB { USBCore Core; }

USBCore::USBCore() {}
void USBCore::loop(Preferences &) {}
void USBCore::begin(Preferences &) {}
void USBCore::end() {}
void USBCore::reset() {}
void USBCore::changeUSBMode(
    DuckyInterpreter::USB_MODE &,
    const uint16_t &, const uint16_t &,
    const std::string &, const std::string &, const std::string &) {}

// ─── USBCDCWrapper ────────────────────────────────────────────────────────────

namespace Devices::USB { USBCDCWrapper CDC; }

USBCDCWrapper::USBCDCWrapper() {}
void USBCDCWrapper::begin(const unsigned long &) {}
void USBCDCWrapper::loop(Preferences &) {}
void USBCDCWrapper::begin(Preferences &) {}
void USBCDCWrapper::end() {}
void USBCDCWrapper::setCallback(const HostCommand &, std::function<void(uint8_t *, const size_t)>) {}
bool USBCDCWrapper::isConnectionEstablished() { return false; }
void USBCDCWrapper::writeBinary(const HostCommand &, const uint8_t *, const size_t &) {}
void USBCDCWrapper::writeDebugString(const std::string &) {}

// ─── USBHID ───────────────────────────────────────────────────────────────────

namespace Devices::USB { USBHID HID; }

USBHID::USBHID() {}
void USBHID::loop(Preferences &) {}
void USBHID::begin(Preferences &) {}
bool USBHID::IsQueueEmpty() { return true; }
void USBHID::keyboard_press(const uint8_t, const uint8_t, const uint8_t, const uint8_t, const uint8_t, const uint8_t, const uint8_t) {}
void USBHID::keyboard_release() {}
void USBHID::consumer_device_keypress(const uint16_t) {}
void USBHID::mouseMove(int8_t, int8_t) {}

// ─── USBMSC ───────────────────────────────────────────────────────────────────

namespace Devices::USB { USBMSC MSC; }

USBMSC::USBMSC() {}
void USBMSC::loop(Preferences &) {}
void USBMSC::begin(Preferences &) {}
void USBMSC::end() {}
bool USBMSC::mountDiskImage(const std::string &, bool) { return false; }
bool USBMSC::mountSD() { return false; }

// ─── USBNCM ───────────────────────────────────────────────────────────────────

namespace Devices::USB { USBNCM NCM; }

USBNCM::USBNCM() {}
void USBNCM::loop(Preferences &) {}
void USBNCM::begin(Preferences &) {}
void USBNCM::end() {}
void USBNCM::startPacketCollection() {}
void USBNCM::stopPacketCollection() {}

#endif // NO_USB_HID

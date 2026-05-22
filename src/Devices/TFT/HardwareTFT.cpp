#ifndef NO_TFT
#include "HardwareTFT.h"

#include "../../Debug/Logging.h"
#include "../../Attacks/Ducky/DuckyPayload.h"

#include <LovyanGFX.hpp>
#ifdef CYD_ESP32
#include "driver/gpio.h"
static void cyd_gpio_high(int pin) {
    gpio_reset_pin((gpio_num_t)pin);
    gpio_set_direction((gpio_num_t)pin, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)pin, 1);
}
static void cyd_gpio_low(int pin) {
    gpio_reset_pin((gpio_num_t)pin);
    gpio_set_direction((gpio_num_t)pin, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)pin, 0);
}
#endif

#include <unordered_map>

#define LOG_TFT "TFT"
#define MAX_RADIUS 4
static uint8_t heatBeatRadius = MAX_RADIUS;
static int heatBeatColor = TFT_WHITE;
static unsigned long previousMillis = 0;

static int foregroundColor = TFT_WHITE;
static int backgroundColor = TFT_BLACK;

// In the future if you are trying to add another board configuration here it would make sense
// To have multiple lgfx::LGFX_Device objects wrapped in ifdefs rather that they single instance we have now
// Right now we are using DISPLAY_TYPE_ST7735S to refer to a device instead of a panel. As such if a new
// board with a ST7735S comes on board we might not have the right x,y offsets
class LGFX_Panel : public lgfx::LGFX_Device
{
#ifdef LILYGO_T_DONGLE_S3
    lgfx::Panel_ST7735S _panel_instance;
#elif defined(WAVESHARE_ESP32_S3_LCD_147)
    lgfx::Panel_ST7789 _panel_instance;
#elif defined(WAVESHARE_RP2040_GEEK)
    lgfx::Panel_ST7789 _panel_instance;
#elif defined(WAVESHARE_ESP32_S3_GEEK)
    lgfx::Panel_ST7789 _panel_instance;
#elif defined(LILYGO_T_WATCH_S3)
    lgfx::Panel_ST7789 _panel_instance;
#elif defined(POCKET_DONGLE_S3)
    lgfx::Panel_ST7735S _panel_instance;
#elif defined(CYD_ESP32)
    lgfx::Panel_ILI9341  _panel_instance;
    lgfx::Touch_XPT2046  _touch_instance;
#else
#error Invalid display type // Full list here: https://github.com/lovyan03/LovyanGFX/tree/master/src/lgfx/v1/panel
#endif
    lgfx::Bus_SPI _bus_instance;
    lgfx::Light_PWM _light_instance;

public:
    LGFX_Panel(void)
    {
        {
            auto cfg = _bus_instance.config();
            cfg.spi_mode = 0;            // Set SPI communication mode (0 ~ 3)
#ifdef CYD_ESP32
            cfg.freq_write = 27000000;   // conservative for reliable init
            cfg.freq_read  = 8000000;
#else
            cfg.freq_write = 27000000;   // SPI clock when sending (max 80MHz, rounded to 80MHz divided by an integer)
            cfg.freq_read = 16000000;    // SPI clock when receiving
#endif
            cfg.pin_sclk = DISPLAY_SCLK; // set SPI SCLK pin number
            cfg.pin_mosi = DISPLAY_MOSI; // Set MOSI pin number for SPI
            cfg.pin_miso = DISPLAY_MISO; // Set MISO pin for SPI (-1 = disable)
            cfg.pin_dc = DISPLAY_DC;     // Set SPI D/C pin number (-1 = disable)

#ifdef ARDUINO_ARCH_ESP32
#ifdef CYD_ESP32
            cfg.spi_host = SPI2_HOST;          // CYD: display on HSPI (SPI2, pins 13/12/14/15); SD uses SPI3 (VSPI, pins 23/19/18/5) — no conflict
            cfg.spi_3wire = false;             // CYD: MOSI(13) and MISO(12) are separate — standard 4-wire SPI
#else
            cfg.spi_host = SPI3_HOST;          // SPI2_HOST is in use by the RGB led
            cfg.spi_3wire = true;              // Set true when receiving on the MOSI pin
#endif
            cfg.use_lock = true;               // Set true when using transaction lock
            cfg.dma_channel = SPI_DMA_CH_AUTO; // Set the DMA channel to use (0=not use DMA / 1=1ch / 2=ch / SPI_DMA_CH_AUTO=auto setting)
#endif
            _bus_instance.config(cfg);              // Apply the setting value to the bus.
            _panel_instance.setBus(&_bus_instance); // Sets the bus to the panel.
        }
        {
            auto cfg = _panel_instance.config(); // Obtain the structure for display panel settings.
            cfg.pin_cs = DISPLAY_CS;             // Pin number to which CS is connected (-1 = disable)
            cfg.pin_rst = DISPLAY_RST;           // pin number where RST is connected (-1 = disable)
            cfg.pin_busy = DISPLAY_BUSY;         // pin number to which BUSY is connected (-1 = disable)
            cfg.panel_width = DISPLAY_HEIGHT;    // actual displayable width. Note: width/height swapped due to the rotation
            cfg.panel_height = DISPLAY_WIDTH;    // Actual displayable height Note: width/height swapped due to the rotation
            cfg.offset_rotation = 1;             // Rotation direction value offset 0~7 (4~7 are upside down)
#ifdef CYD_ESP32
            cfg.readable = false;                // CYD: disable read-back to simplify SPI init
#else
            cfg.readable = true;                 // set to true if data can be read
#endif
#ifdef CYD_ESP32
            cfg.invert = false;                  // ILI9341 does not need inversion
#else
            cfg.invert = true;
#endif
            cfg.rgb_order = false;
            cfg.dlen_16bit = false; // Set to true for panels that transmit data length in 16-bit units with 16-bit parallel or SPI
#ifdef CYD_ESP32
            cfg.bus_shared = false; // SD uses SPI3 (VSPI), display uses SPI2 (HSPI) — separate buses
#else
            cfg.bus_shared = true;  // If the bus is shared with the SD card, set to true
#endif

#ifdef LILYGO_T_DONGLE_S3
            cfg.offset_x = 26;        // Panel offset in X direction
            cfg.offset_y = 1;         // Y direction offset amount of the panel
            cfg.dummy_read_pixel = 8; // Number of bits for dummy read before pixel read
            cfg.dummy_read_bits = 1;  // Number of dummy read bits before non-pixel data read
            // Please set the following only when the display is shifted with a driver with a variable number of pixels such as ST7735 or ILI9163.
            cfg.memory_width = 132;  // Maximum width supported by driver IC
            cfg.memory_height = 160; // Maximum height supported by driver IC
#elif defined(WAVESHARE_ESP32_S3_LCD_147)
            cfg.offset_x = 34;        // Panel offset in X direction
            cfg.offset_y = 0;         // Y direction offset amount of the panel
            cfg.dummy_read_pixel = 8; // Number of bits for dummy read before pixel read
            cfg.dummy_read_bits = 1;  // Number of dummy read bits before non-pixel data read
#elif defined(WAVESHARE_RP2040_GEEK)
            cfg.offset_x = 52;        // Panel offset in X direction
            cfg.offset_y = 40;        // Y direction offset amount of the panel
            cfg.dummy_read_pixel = 8; // Number of bits for dummy read before pixel read
            cfg.dummy_read_bits = 1;  // Number of dummy read bits before non-pixel data read
#elif defined(WAVESHARE_ESP32_S3_GEEK)
            cfg.offset_x = 52;        // Panel offset in X direction
            cfg.offset_y = 40;        // Y direction offset amount of the panel
            cfg.dummy_read_pixel = 8; // Number of bits for dummy read before pixel read
            cfg.dummy_read_bits = 1;  // Number of dummy read bits before non-pixel data read
#elif defined(LILYGO_T_WATCH_S3)
            cfg.offset_rotation = 2;
            cfg.offset_x = 0;         // Panel offset in X direction
            cfg.offset_y = 0;         // Y direction offset amount of the panel
            cfg.dummy_read_pixel = 8; // Number of bits for dummy read before pixel read
            cfg.dummy_read_bits = 1;  // Number of dummy read bits before non-pixel data read
#elif defined(POCKET_DONGLE_S3)
            cfg.offset_x = 26;        // Panel offset in X direction
            cfg.offset_y = 1;         // Y direction offset amount of the panel
            cfg.dummy_read_pixel = 8; // Number of bits for dummy read before pixel read
            cfg.dummy_read_bits = 1;  // Number of dummy read bits before non-pixel data read
            // Please set the following only when the display is shifted with a driver with a variable number of pixels such as ST7735 or ILI9163.
            cfg.memory_width = 132;  // Maximum width supported by driver IC
            cfg.memory_height = 160; // Maximum height supported by driver IC
#elif defined(CYD_ESP32)
            cfg.panel_width  = 320;  // CYD: ILI9341 nativ Querformat (320 Spalten × 240 Zeilen)
            cfg.panel_height = 240;  // override der globalen width/height-Vertauschung oben
            cfg.offset_rotation = 0;
            cfg.offset_x = 0;
            cfg.offset_y = 0;
            cfg.dummy_read_pixel = 8;
            cfg.dummy_read_bits = 1;
            cfg.memory_width  = 320;
            cfg.memory_height = 240;
#else
#error Invalid display type // Full list here: https://github.com/lovyan03/LovyanGFX/tree/master/src/lgfx/v1/panel
#endif
            _panel_instance.config(cfg);
        }
#ifndef CYD_ESP32
        // CYD: backlight is GPIO-controlled directly (LEDC/PWM broken in Arduino ESP32 3.x for this board)
        {
            auto cfg = _light_instance.config();
            cfg.pin_bl = DISPLAY_LEDA;
            cfg.invert = true;         // true to invert backlight brightness
            cfg.freq = 12000;
            cfg.pwm_channel = 7;
            _light_instance.config(cfg);
            _panel_instance.setLight(&_light_instance);
        }
#endif
#if defined(CYD_ESP32)
        // Always configure XPT2046 touch via LovyanGFX (independent of NO_TOUCH HW class)
        {
            auto cfg = _touch_instance.config();
            cfg.x_min         = 300;
            cfg.x_max         = 3900;
            cfg.y_min         = 300;
            cfg.y_max         = 3900;
            cfg.pin_int       = TOUCH_IRQ;
            cfg.pin_sclk      = TOUCH_CLK;
            cfg.pin_mosi      = TOUCH_MOSI;
            cfg.pin_miso      = TOUCH_MISO;
            cfg.pin_cs        = TOUCH_CS;
            cfg.offset_rotation = 0;
            _touch_instance.config(cfg);
            _panel_instance.setTouch(&_touch_instance);
        }
#endif
        setPanel(&_panel_instance);
    }
};

static LGFX_Panel lcd;
static int16_t xpos = 0;
static int16_t ypos = 0;

static std::unordered_map<std::string, int> colorLookup{
    {"BLACK", TFT_BLACK},
    {"NAVY", TFT_NAVY},
    {"DARKGREEN", TFT_DARKGREEN},
    {"DARKCYAN", TFT_DARKCYAN},
    {"MAROON", TFT_MAROON},
    {"PURPLE", TFT_PURPLE},
    {"OLIVE", TFT_OLIVE},
    {"LIGHTGREY", TFT_LIGHTGREY},
    {"LIGHTGRAY", TFT_LIGHTGRAY},
    {"DARKGREY", TFT_DARKGREY},
    {"DARKGRAY", TFT_DARKGRAY},
    {"BLUE", TFT_BLUE},
    {"GREEN", TFT_GREEN},
    {"CYAN", TFT_CYAN},
    {"RED", TFT_RED},
    {"MAGENTA", TFT_MAGENTA},
    {"YELLOW", TFT_YELLOW},
    {"WHITE", TFT_WHITE},
    {"ORANGE", TFT_ORANGE},
    {"GREENYELLOW", TFT_GREENYELLOW},
    {"PINK", TFT_PINK},
    {"BROWN", TFT_BROWN},
    {"GOLD", TFT_GOLD},
    {"SILVER", TFT_SILVER},
    {"SKYBLUE", TFT_SKYBLUE},
    {"VIOLET", TFT_VIOLET},
};

namespace Devices
{
    HardwareTFT TFT;
}

void HardwareTFT::display(const int &x, const int &y, const std::string &str)
{
    lcd.setCursor(x, y);
    lcd.setTextColor(foregroundColor, backgroundColor);
    lcd.println(str.c_str());
    lcd.display();
}

void HardwareTFT::clearScreen()
{
    lcd.clear(backgroundColor);
    lcd.display();
}

void HardwareTFT::powerOff()
{
    lcd.clearDisplay();
#ifdef CYD_ESP32
    cyd_gpio_low(DISPLAY_LEDA);
#else
    lcd.setBrightness(0);
#endif
    lcd.sleep();
}

void HardwareTFT::powerOn()
{
    lcd.wakeup();
#ifdef CYD_ESP32
    cyd_gpio_high(DISPLAY_LEDA);
#else
    lcd.setBrightness(100);
#endif
}

void HardwareTFT::displayPng(HardwareStorage &storage, const std::string &filename)
{
    xpos = 0;
    ypos = 0;
    size_t size = storage.getFileSize(filename);

    if (size == 0)
    {
        Debug::Log.info(LOG_TFT, "invalid file size");
    }

    uint8_t *data = storage.readFileAsBinary(filename);

    if (data == NULL)
    {
        Debug::Log.info(LOG_TFT, "invalid data");
        return;
    }

    lcd.drawPng((uint8_t *)data, size);

    free(data);
}

void HardwareTFT::displayRectangle(const int &x, const int &y, const int &width, const int &height)
{
    lcd.fillRect(x, y, width, height, foregroundColor);
    lcd.display();
}

void HardwareTFT::setTextSize(int size)
{
    lcd.setTextSize(size);
}

bool HardwareTFT::getTouch(int16_t &x, int16_t &y)
{
#ifdef CYD_ESP32
    uint16_t tx, ty;
    if (lcd.getTouch(&tx, &ty)) {
        x = (int16_t)tx;
        y = (int16_t)ty;
        return true;
    }
#endif
    return false;
}

void HardwareTFT::setForegroundColor(int color)
{
    foregroundColor = color;
}

void HardwareTFT::setBackgroundColor(int color)
{
    backgroundColor = color;
}

int HardwareTFT::convertStringToColor(const std::string &color)
{
    // Check if it's a named color
    const auto &it = colorLookup.find(color);
    if (it != colorLookup.cend())
    {
        return it->second;
    }

    // Check if it's an RGB888 hex color code (e.g., "0xFFFFFF" or "FFFFFF")
    std::string hexStr = color;
    if (hexStr.length() >= 2 && hexStr[0] == '0' && (hexStr[1] == 'x' || hexStr[1] == 'X'))
    {
        hexStr = hexStr.substr(2);
    }

    char* end;
    errno = 0;
    const unsigned long value = std::strtoul(hexStr.c_str(), &end, 16);

    if (errno != 0 || end == hexStr.c_str()) {
        return TFT_BLACK;
    }

    return lcd.color24to16(value);
}

HardwareTFT::HardwareTFT()
{
}

void HardwareTFT::loop(Preferences &prefs)
{
#ifndef NO_TFT_HEARTBEAT
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis <= 250)
    {
        return;
    }

    previousMillis = currentMillis;

    lcd.drawCircle(DISPLAY_WIDTH - MAX_RADIUS - 1, DISPLAY_HEIGHT - MAX_RADIUS - 1, heatBeatRadius, heatBeatColor);

    heatBeatRadius = heatBeatColor == foregroundColor ? heatBeatRadius - 1 : heatBeatRadius + 1;

    if (heatBeatRadius > MAX_RADIUS)
    {
        heatBeatColor = foregroundColor;
        heatBeatRadius = MAX_RADIUS;
    }
    else if (heatBeatRadius <= 0)
    {
        heatBeatColor = backgroundColor;
        heatBeatRadius = 1;
    }
#endif
}

void HardwareTFT::begin(Preferences &prefs)
{
#ifdef CYD_ESP32
    Serial.begin(115200);
    // Keep backlight OFF during init to hide ILI9341 GRAM garbage
    cyd_gpio_low(DISPLAY_LEDA);
#endif
    lcd.init();
#ifdef CYD_ESP32
    // ILI9341 GRAM is 240×320 (portrait). fillScreen() in landscape (320×240) may not
    // reach every GRAM cell. Force portrait first to clear ALL 76800 cells, then restore.
    // CYD: panel_width=320, panel_height=240 → nativ Querformat
    // setRotation(4) = H-Spiegel hebt physischen Board-Spiegel auf
    lcd.setRotation(4);
    lcd.fillScreen(TFT_BLACK);
    lcd.fillScreen(backgroundColor);
    cyd_gpio_high(DISPLAY_LEDA); // backlight ON only after full GRAM clear
    Serial.println("[TFT] init done");
#else
    lcd.setBrightness(128);
#endif
    lcd.clear(backgroundColor);
    lcd.display();

    Attacks::Ducky.registerDynamicVariable([this]()
                                           { return std::pair("#_DISPLAY_WIDTH_", std::to_string(DISPLAY_WIDTH)); });
    Attacks::Ducky.registerDynamicVariable([this]()
                                           { return std::pair("#_DISPLAY_HEIGHT_", std::to_string(DISPLAY_HEIGHT)); });
}
#endif
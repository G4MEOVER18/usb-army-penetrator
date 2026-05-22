#include "HardwareStorage.h"
#include "../../Debug/Logging.h"

#ifdef NO_SD
    #ifdef ARDUINO_ARCH_RP2040
        #include <VFS.h>
        #include <LittleFS.h>
        #define FILE_INTERFACE LittleFS
    #else
        #include <SPIFFS.h>
        #define FILE_INTERFACE SPIFFS
    #endif
#elif ARDUINO_ARCH_RP2040
    #include "RP2040/SDClassWrapper.h"
    using namespace fs;
    #define FILE_INTERFACE SDCard
#elif USE_SD_INTERFACE
    // generic esp32 SD interface
    #include "SD.h"
    using namespace fs;
    #define FILE_INTERFACE SD
#else
    #include "ESP32/SDMMCFS2.h"
    #include "driver/sdmmc_host.h"
    #include "driver/sdspi_host.h"
    #include "sdmmc_cmd.h"
    using namespace fs;
    #define FILE_INTERFACE SD_MMC_2

    fs::SDMMCFS getFileInterface()
    {
        return FILE_INTERFACE;
    }

#endif

#define LOG_MMC "MMC"

static uint8_t cachedCapacity = 0;
static std::vector<std::string> filesCache;

namespace Devices
{
    HardwareStorage Storage;
}

std::string HardwareStorage::readFile(fs::FS &fs, const char *path)
{
    File file = fs.open(path, "r");
    if (!file)
    {
        return "";
    }
    std::string line;
    while (file.available())
    {
        line = line + (char)file.read();
    }
    return line;
}

void HardwareStorage::writeFileData(const std::string& filename, const uint8_t *buffer, const size_t size)
{
    File file = FILE_INTERFACE.open(filename.c_str(), "w");
    if (!file)
    {
        Debug::Log.info(LOG_MMC, "Could not open file");
        return;
    }
    
    if (file.write(buffer, size) != size)
    {
        Debug::Log.info(LOG_MMC, "write err");
    }

    file.close();
    refreshCache();
}

File HardwareStorage::openFile(const std::string& filename, const char* mode)
{
    File file = FILE_INTERFACE.open(filename.c_str(), mode);
    if (!file)
    {
        Debug::Log.error(LOG_MMC, "Could not open file");
        return File();
    }

    return file;
}

std::size_t HardwareStorage::getFileSize(const std::string& filename)
{
    File file = FILE_INTERFACE.open(filename.c_str(), "r");
    if (!file)
    {
        Debug::Log.info(LOG_MMC, "Could not open file: " + filename);
        return 0;
    }

    // Get the file size
    int ret = file.size();
    file.close();
    return ret;
}

bool HardwareStorage::doesFileExist(const std::string& filename)
{
    return FILE_INTERFACE.exists(filename.c_str());
}

uint8_t* HardwareStorage::readFileAsBinary(const std::string& filename)
{
    File file = FILE_INTERFACE.open(filename.c_str(), "r");
    if (!file)
    {
        Debug::Log.info(LOG_MMC, "Could not open file: " + filename);
        return 0;
    }

    // Get the file size
    size_t fileSize = file.size();

    // Allocate memory for the file content
    uint8_t *fileContent = (uint8_t *)malloc(fileSize + 1); // +1 for null terminator
    if (!fileContent)
    {
        Debug::Log.info(LOG_MMC, "Could not alloc");
        file.close();
        return 0;
    }

    // Read the entire file into the buffer
    if (file.read((uint8_t *)fileContent, fileSize) != fileSize)
    {
        Debug::Log.info(LOG_MMC, "Could not read");
        free(fileContent);
        file.close();
        return 0;
    }

    // Null-terminate the buffer
    fileContent[fileSize] = '\0';

    return fileContent;
}

std::string HardwareStorage::readLineFromFile(const std::string &filename, const int lineNumber)
{
    File file = FILE_INTERFACE.open(filename.c_str(), "r");
    if (!file)
    {
        Debug::Log.info(LOG_MMC, "Could not open file: " + filename);
        return "";
    }

    int currentLine = 0;
    String line;
    while (file.available())
    {
        line = file.readStringUntil('\n'); // todo read up to EOF
        if (currentLine == lineNumber)
        {
            break;
        }
        line = "";
        currentLine++;
    }

    std::string ret = std::string(line.c_str());

    if (ret.length() == 1 && ret[0] == '\n')
    {
        return "";
    }
    if (ret.length() == 2 && ret[0] == '\r' && ret[1] == '\n')
    {
        return "";
    }

    return ret;
}

static void listDir(std::vector<std::string> &files, fs::FS &fs, const char *dirname)
{
    File root = fs.open(dirname, "r");
    if (!root)
    {
        return;
    }
    if (!root.isDirectory())
    {
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            const char* path =
#ifdef ARDUINO_ARCH_RP2040
            file.fullName();
#else
            file.path();
#endif
            listDir(files, fs, path);
        }
        else
        {
            std::string filename = std::string(file.name());
            files.emplace_back("/" + filename);
        }
        file = root.openNextFile();
    }
}

std::vector<std::string> HardwareStorage::listFiles()
{
    if (filesCache.size() != 0)
    {
        return filesCache;
    }

    if (running)
    {
        listDir(filesCache, FILE_INTERFACE, "/");
    }

    return filesCache;
}

uint8_t HardwareStorage::usedPercentage()
{
    if (!running) return 0;
    if (capacityCached) return cachedCapacity;
#if defined(NO_SD) && defined(ARDUINO_ARCH_RP2040)
    fs::FSInfo info;
    if (FILE_INTERFACE.info(info))
    {
        if (info.totalBytes == 0) { cachedCapacity = 0; }
        else { cachedCapacity = (uint8_t)((info.usedBytes * 100ULL) / info.totalBytes); }
    }
    else
    {
        cachedCapacity = 0;
    }
#else
    {
        uint64_t total = FILE_INTERFACE.totalBytes();
        if (total == 0) { cachedCapacity = 0; }
        else { cachedCapacity = (uint8_t)((FILE_INTERFACE.usedBytes() * 100ULL) / total); }
    }
#endif
    capacityCached = true;
    return cachedCapacity;
}

bool HardwareStorage::createEmptyFile(const std::string &filename)
{
    File file = FILE_INTERFACE.open(filename.c_str(), "w");
    if (!file)
    {
        return false;
    }
    file.close();
    refreshCache();
    return true;
}

bool HardwareStorage::deleteFile(const std::string& filename)
{
    refreshCache();
    return FILE_INTERFACE.remove(filename.c_str());
}

HardwareStorage::HardwareStorage()
{
#ifndef NO_SD
#if !defined(ARDUINO_ARCH_RP2040) && !defined(USE_SD_INTERFACE)
    SD_MMC = FILE_INTERFACE;
#endif
#endif
}

bool HardwareStorage::isRawAccessSupported()
{
#if defined(USE_SD_INTERFACE) or defined(USE_SD_MMC_INTERFACE)
    return true;
#else
    return false;
#endif
}

size_t HardwareStorage::sectorSize()
{
#if defined(USE_SD_INTERFACE)
    return FILE_INTERFACE.sectorSize();
#elif defined (USE_SD_MMC_INTERFACE)
    if (FILE_INTERFACE.getCard() == nullptr)
    {
        return 0;
    }
    return FILE_INTERFACE.getCard()->csd.sector_size;
#else
    return 0;
#endif
}

size_t HardwareStorage::deviceCapacity()
{
#if defined(USE_SD_INTERFACE)
    return FILE_INTERFACE.cardSize();
#elif defined (USE_SD_MMC_INTERFACE)
    return FILE_INTERFACE.getCard()->csd.capacity;
#else
    return 0;
#endif
}

int32_t HardwareStorage::readRawSectors(uint8_t* buffer, uint32_t lba, uint32_t sectors)
{
#if defined(USE_SD_INTERFACE)
    return FILE_INTERFACE.readRAW((uint8_t*) buffer, lba);
#elif defined (USE_SD_MMC_INTERFACE)
    if (sdmmc_read_sectors(FILE_INTERFACE.getCard(), buffer, lba, sectors) == ESP_OK)
    {
        return sectors * FILE_INTERFACE.getCard()->csd.sector_size;
    }
    return -1;
#else
    return -1;
#endif
}

int32_t HardwareStorage::writeRawSectors(uint8_t* buffer, uint32_t lba, uint32_t sectors)
{
#if defined(USE_SD_INTERFACE)
    return FILE_INTERFACE.writeRAW(buffer, lba);
#elif defined (USE_SD_MMC_INTERFACE)
    if (sdmmc_write_sectors(FILE_INTERFACE.getCard(), buffer, lba, sectors) == ESP_OK)
    {
        return sectors * FILE_INTERFACE.getCard()->csd.sector_size;
    }
    return -1;
#else
    return -1;
#endif
}

void HardwareStorage::flush()
{
    // not currently supported
}

void HardwareStorage::refreshCache()
{
    filesCache.clear();
    capacityCached = false;
}

void HardwareStorage::loop(Preferences &prefs)
{
}

void HardwareStorage::begin(Preferences &prefs)
{
    this->begin(prefs, false);
}

void HardwareStorage::begin(Preferences &prefs, bool format)
{
#ifdef NO_SD
    #ifdef ARDUINO_ARCH_RP2040
    if (!FILE_INTERFACE.begin())
    {
        Debug::Log.info(LOG_MMC, "FILE_INTERFACE could not be started");
    }
    else
    {
        running = true;
        VFS.root(FILE_INTERFACE);
    }
    
    #else
    if (!FILE_INTERFACE.begin(format, "/sdcard"))
    {
        Debug::Log.info(LOG_MMC, "FILE_INTERFACE could not be started");
    }
    else
    {
        running = true;
    }
    #endif
#elif defined (SD_MMC_D1_PIN)
    FILE_INTERFACE.setPins(SD_MMC_CLK_PIN, SD_MMC_CMD_PIN, SD_MMC_D0_PIN, SD_MMC_D1_PIN, SD_MMC_D2_PIN, SD_MMC_D3_PIN);
    {
        const uint32_t speeds[] = {SDMMC_FREQ_52M, SDMMC_FREQ_26M, 4000000};
        for (size_t i = 0; i < 3; i++) {
            if (i > 0) { FILE_INTERFACE.end(); delay(200); }
            if (FILE_INTERFACE.begin("/sdcard", false, format, speeds[i])) {
                if (FILE_INTERFACE.cardType() != CARD_NONE) {
                    running = true;
                    Debug::Log.info(LOG_MMC, std::string("SD OK at speed index ") + std::to_string(i));
                    break;
                }
                FILE_INTERFACE.end();
            }
        }
        if (!running) {
            Debug::Log.info(LOG_MMC, "FILE_INTERFACE could not be started at any speed");
        }
    }
#elif defined(ARDUINO_ARCH_RP2040)
    if (!FILE_INTERFACE.begin(format))
    {
        Debug::Log.info(LOG_MMC, "FILE_INTERFACE could not be started");
    }
    else
    {
        running = true;
    }
#elif defined(USE_SD_INTERFACE)
    pinMode(SD_MISO_PIN, INPUT_PULLUP);
#ifdef CYD_ESP32
    // CYD: deassert CS and allow SD card to power up before touching the bus
    pinMode(SD_CS_PIN, OUTPUT);
    digitalWrite(SD_CS_PIN, HIGH);
    pinMode(SD_MISO_PIN, INPUT_PULLUP);
    delay(500); // SD card power-up time
#endif
    SPI.begin(SD_SCLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);

#ifdef CYD_ESP32
    // CYD: retry SD init at multiple speeds; SD.end() between retries resets internal state
    const uint32_t speeds[] = {400000, 1000000, 4000000};
    for (size_t i = 0; i < 3; i++) {
        if (i > 0) {
            FILE_INTERFACE.end();
            delay(100);
        }
        if (FILE_INTERFACE.begin(SD_CS_PIN, SPI, speeds[i])) {
            running = true;
            break;
        }
    }
    if (!running) {
        Debug::Log.info(LOG_MMC, "FILE_INTERFACE could not be started");
    }
#else
    if (!FILE_INTERFACE.begin(SD_CS_PIN))
    {
        Debug::Log.info(LOG_MMC, "FILE_INTERFACE could not be started");
    }
    else
    {
        running = true;
    }
#endif
#endif
}
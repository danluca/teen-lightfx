//
// Copyright (c) 2023,2024 by Dan Luca. All rights reserved.
//

#include "util.h"
#include "utility/ECCX08DefaultTLSConfig.h"
// increase the amount of space for file system to 128kB (default 64kB)
#define RP2040_FS_SIZE_KB   (128)
#include <LittleFSWrapper.h>

#define FILE_BUF_SIZE   256
const uint maxAdc = 1 << ADC_RESOLUTION;
const char stateFileName[] = LITTLEFS_FILE_PREFIX "/state.json";

static uint8_t sysStatus = 0x00;    //system status bit array
FixedQueue<TimeSync, 8> timeSyncs;

LittleFSWrapper *fsPtr;

inline static float toFahrenheit(float celsius) {
    return celsius * 9.0f / 5 + 32;
}

float boardTemperature(bool bFahrenheit) {
    if (IMU.temperatureAvailable()) {
        float tempC = 0.0f;
        IMU.readTemperatureFloat(tempC);
#ifndef DISABLE_LOGGING
        // Serial console doesn't seem to work well with UTF-8 chars, hence not using ° symbol for degree.
        // Can also try using wchar_t type. Unsure ArduinoLog library supports it well. All in all, not worth digging much into it - only used for troubleshooting
        Log.infoln(F("Board temperature %D 'C (%D 'F)"), tempC, toFahrenheit(tempC));
#endif
        return bFahrenheit ? toFahrenheit(tempC) : tempC;
    }
    return IMU_TEMPERATURE_NOT_AVAILABLE;
}

float controllerVoltage() {
    const uint avgSize = 8;  //we'll average 8 readings back to back
    uint valSum = 0;
    for (uint x = 0; x < avgSize; x++)
        valSum += analogRead(A0);
    Log.traceln(F("Voltage %d average reading: %d"), avgSize, valSum/avgSize);
    valSum = valSum*MV3_3/avgSize;
    valSum = valSum/VCC_DIV_R5*(VCC_DIV_R5+VCC_DIV_R4)/maxAdc;  //watch out not to exceed uint range, these are large numbers. operations order tuned to avoid overflow
    return (float)valSum/1000.0f;
}

float chipTemperature(bool bFahrenheit) {
    uint curAdc = adc_get_selected_input();
    const uint avgSize = 8;   //we'll average 8 readings back to back

    adc_select_input(4);    //internal temperature sensor is on ADC channel 4
    uint valSum = 0;
    for (uint x = 0; x < avgSize; x++)
        valSum += adc_read();
    adc_select_input(curAdc);   //restore the ADC input selection
    Log.traceln(F("Internal Temperature %d average reading: %d"), avgSize, valSum/avgSize);

    auto tV = (float)(valSum*MV3_3/avgSize/maxAdc);   //voltage in mV
    //per RP2040 documentation - datasheet, section 4.9.5 Temperature Sensor, page 565 - the formula is 27 - (ADC_Voltage - 0.706)/0.001721
    //the Vtref is typical of 0.706V at 27'C with a slope of -1.721mV per degree Celsius
    float temp = 27.0f - (tV - CHIP_RP2040_TEMP_SENSOR_VOLTAGE_27)/CHIP_RP2040_TEMP_SENSOR_VOLTAGE_SLOPE;
    return bFahrenheit ? toFahrenheit(temp) : temp;
}

/**
 * Adapted from article https://rheingoldheavy.com/better-arduino-random-values/
 * <p>Not fast, timed at 64ms on Arduino Uno allegedly</p>
 * <p>The residual floating voltage of the unconnected AD pin A1 is read multiple times and assembled into a 32 bit value</p>
 * @return a good 32bit random value from the residual electric signal on an Analog-Digital unconnected (floating) pin
 */
ulong adcRandom() {
    uint8_t  seedBitValue  = 0;
    uint8_t  seedByteValue = 0;
    uint32_t seedWordValue = 0;

    for (uint8_t wordShift = 0; wordShift < 4; wordShift++) {       // 4 bytes in a 32 bit word
        for (uint8_t byteShift = 0; byteShift < 8; byteShift+=2) {  // 8 bits in a byte, advance by 2 - we collect 2 bits (last 2 LSB) with each analog read
            for (uint8_t bitSum = 0; bitSum < 8; bitSum++) {        // 8 samples of analog pin
                seedBitValue += (analogRead(A1) & 0x03);  // Flip the coin 8 times, adding the results together
            }
            delay(1);                                               // Delay a single millisecond to allow the pin to fluctuate
            seedByteValue |= ((seedBitValue & 0x03) << byteShift);  // Build a stack of eight flipped coins
            seedBitValue = 0;                                       // Clear out the previous coin value
        }
        seedWordValue |= ((uint32_t)seedByteValue << (8 * wordShift));    // Build a stack of four sets of 8 coins (shifting right creates a larger number so cast to 32bit)
        seedByteValue = 0;                                                // Clear out the previous stack value
    }
    return (seedWordValue);
}

/**
 * Setup the on-board status LED
 */
void setupStateLED() {
    pinMode(LEDR, OUTPUT);
    pinMode(LEDG, OUTPUT);
    pinMode(LEDB, OUTPUT);
    updateStateLED(0, 0, 0);    //black, turned off
}
/**
 * Controls the on-board status LED
 * @param color
 */
void updateStateLED(uint32_t colorCode) {
    uint8_t r = (colorCode >> 16) & 0xFF;
    uint8_t g = (colorCode >>  8) & 0xFF;
    uint8_t b = (colorCode >>  0) & 0xFF;
    updateStateLED(r, g, b);
}

void updateStateLED(uint8_t red, uint8_t green, uint8_t blue) {
    analogWrite(LEDR, 255 - red);
    analogWrite(LEDG, 255 - green);
    analogWrite(LEDB, 255 - blue);
}

void fsInit() {
#ifndef DISABLE_LOGGING
    mbed::BlockDevice *bd = mbed::BlockDevice::get_default_instance();
    bd->init();
    Log.infoln(F("Default BlockDevice type %s, size %u B, read size %u B, program size %u B, erase size %u B"),
               bd->get_type(), bd->size(), bd->get_read_size(), bd->get_program_size(), bd->get_erase_size());
    bd->deinit();
#endif

    fsPtr = new LittleFSWrapper();
    if (fsPtr->init()) {
        setSysStatus(SYS_STATUS_FILESYSTEM);
        Log.infoln("Filesystem OK");
    }

#ifndef DISABLE_LOGGING
    Log.infoln(F("Root FS %s contents:"), LittleFSWrapper::getRoot());
    DIR *d = opendir(LittleFSWrapper::getRoot());
    struct dirent *e = nullptr;
    while (e = readdir(d))
        Log.infoln(F("  %s [%d]"), e->d_name, e->d_type);
    Log.infoln(F("Dir complete."));
#endif
}

/**
 * Reads a text file - if it exists - into a string object
 * @param fname name of the file to read
 * @param s string to store contents into
 * @return number of characters in the file; 0 if file is empty or doesn't exist
 */
size_t readTextFile(const char *fname, String *s) {
    FILE *f = fopen(fname, "r");
    size_t fsize = 0;
    if (f) {
        char buf[FILE_BUF_SIZE];
        memset(buf, 0, FILE_BUF_SIZE);
        size_t cread = 1;
        while (cread = fread(buf, 1, FILE_BUF_SIZE, f)) {
            s->concat(buf, cread);
            fsize += cread;
        }
        fclose(f);
#ifndef DISABLE_LOGGING
        Log.infoln(F("Read %d bytes from %s file"), fsize, fname);
        Log.traceln(F("File %s content [%d]: %s"), fname, fsize, s->c_str());
#endif
    } else
        Log.errorln(F("Text file %s was not found/could not read"), fname);
    return fsize;
}

/**
 * Writes (overrides if already exists) a file using the string content
 * @param fname file name to write
 * @param s contents to write
 * @return number of bytes written
 */
size_t writeTextFile(const char *fname, String *s) {
    size_t fsize = 0;
    FILE *f = fopen(fname, "w");
    if (f) {
        fsize = fwrite(s->c_str(), sizeof(s->charAt(0)), s->length(), f);
        fclose(f);
        Log.infoln(F("File %s has been saved, size %d bytes"), fname, s->length());
    } else
        Log.errorln(F("Failed to create file %s for writing"), fname);
    return fsize;
}

bool removeFile(const char *fname) {
    FILE *f = fopen(fname, "r");
    if (f) {
        fclose(f);
        return lfs.remove(fname) == 0;
    }
    //file does not exist - return true to the caller, the intent is already fulfilled
    return true;
}

/**
 * Multiply function for color blending purposes - assumes the operands are fractional (n/256) and the result
 * of multiplying 2 fractional numbers is less than both numbers (e.g. 0.2 x 0.3 = 0.06). Special handling for operand values of 255 (i.e. 1.0)
 * <p>f(a,b) = a*b</p>
 * @param a first operand, range [0,255] inclusive - mapped as range [0,1.0] inclusive
 * @param b second operand, range [0,255] inclusive - mapped as range [0,1.0] inclusive
 * @return (a*b)/256
 * @see https://en.wikipedia.org/wiki/Blend_modes
 */
uint8_t bmul8(uint8_t a, uint8_t b) {
    if (a==255)
        return b;
    if (b==255)
        return a;
    return ((uint16_t)a*(uint16_t)b)/256;
}

/**
 * Screen two 8 bit operands for color blending purposes - assumes the operands are fractional (n/256)
 * <p>f(a,b)=1-(1-a)*(1-b)</p>
 * @param a first operand, range [0,255] inclusive - mapped as range [0,1.0] inclusive
 * @param b second operand, range [0,255] inclusive - mapped as range [0,1.0] inclusive
 * @return 255-bmul8(255-a, 255-b)
 * @see https://en.wikipedia.org/wiki/Blend_modes
 */
uint8_t bscr8(uint8_t a, uint8_t b) {
    return 255-bmul8(255-a, 255-b);
}

/**
 * Overlay two 8 bit operands for color blending purposes - assumes the operands are fractional (n/256)
 * <p>f(a,b)=2*a*b, if a&lt;0.5; 1-2*(1-a)*(1-b), otherwise</p>
 * @param a first operand, range [0,255] inclusive - mapped as range [0,1.0] inclusive
 * @param b second operand, range [0,255] inclusive - mapped as range [0,1.0] inclusive
 * @return the 8 bit value per formula above
 * @see https://en.wikipedia.org/wiki/Blend_modes
 */
uint8_t bovl8(uint8_t a, uint8_t b) {
    if (a < 128)
        return bmul8(a, b)*2;
    return 255-bmul8(255-a, 255-b)*2;
}

const uint8_t setSysStatus(uint8_t bitMask) {
    sysStatus |= bitMask;
    return sysStatus;
}

const uint8_t resetSysStatus(uint8_t bitMask) {
    sysStatus &= (~bitMask);
    return sysStatus;
}

bool isSysStatus(uint8_t bitMask) {
    return (sysStatus & bitMask);
}

const uint8_t getSysStatus() {
    return sysStatus;
}

/**
 * Are we in DST (Daylight Savings Time) at this time?
 * @param time
 * @return
 */
bool isDST(const time_t time) {
//    const uint16_t md = encodeMonthDay(time);
    // switch the time offset for CDT between March 12th and Nov 5th - these are chosen arbitrary (matches 2023 dates) but close enough
    // to the transition, such that we don't need to implement complex Sunday counting rules
//    return md > 0x030C && md < 0x0B05;
    int mo = month(time);
    int dy = day(time);
    int dow = weekday(time);
// DST runs from second Sunday of March to first Sunday of November
    // Never in January, February or December
    if (mo < 3 || mo > 11) { return false; }
    // Always in April to October
    if (mo > 3 && mo < 11) { return true; }
    // In March, DST if previous Sunday was on or after the 8th.
    // Begins at 2am on second Sunday in March
    int previousSunday = dy - dow;
    if (mo == 3) { return previousSunday >= 7; }
    // Otherwise November, DST if before the first Sunday, i.e. the previous Sunday must be before the 1st
    return previousSunday < 0;

}
/**
 * Leverages ECC608B's High-Quality NIST SP 800-90A/B/C Random Number Generator
 * <p>It is slow - takes about 30ms</p>
 * @param minLim minimum value, defaults to 0
 * @param maxLim maximum value, defaults to max unsigned 8 bits (UINT8_MAX)
 * @return a high quality random number in the range specified
 * @see secRandom(uint32_t, uint32_t)
 */
 uint8_t secRandom8(uint8_t minLim, uint8_t maxLim) {
    return secRandom(minLim, maxLim > 0 ? maxLim : UINT8_MAX);
}

/**
 * Leverages ECC608B's High-Quality NIST SP 800-90A/B/C Random Number Generator
 * <p>It is slow - takes about 30ms</p>
 * @param minLim minimum value, defaults to 0
 * @param maxLim maximum value, defaults to max unsigned 16 bits (UINT16_MAX)
 * @return a high quality random number in the range specified
 * @see secRandom(uint32_t, uint32_t)
 */
uint16_t secRandom16(const uint16_t minLim, const uint16_t maxLim) {
    return secRandom(minLim, maxLim > 0 ? maxLim : UINT16_MAX);
}

/**
 * Leverages ECC608B's High-Quality NIST SP 800-90A/B/C Random Number Generator
 * <p>It is slow - takes about 30ms</p>
 * @param minLim minimum value, defaults to 0
 * @param maxLim maximum value, defaults to max signed 32 bits (INT32_MAX)
 * @return a high quality random number in the range specified
 */
uint32_t secRandom(const uint32_t minLim, const uint32_t maxLim) {
    long low = (long)minLim;
    long high = maxLim > 0 ? (long)maxLim : INT32_MAX;
    return isSysStatus(SYS_STATUS_ECC) ? ECCX08.random(low, high) : random(low, high);
}

bool secElement_setup() {
    if (!ECCX08.begin()) {
        Log.errorln(F("No ECC608 chip present on the RP2040 board (or failed communication)!"));
        return false;
    }
    const char* eccSerial = ECCX08.serialNumber().c_str();
    if (!ECCX08.locked()) {
        Log.warningln(F("The ECCX08 s/n %s on your board is not locked - proceeding with default TLS configuration locking."), eccSerial);
        if (!ECCX08.writeConfiguration(ECCX08_DEFAULT_TLS_CONFIG)) {
            Log.errorln(F("Writing ECCX08 default TLS configuration FAILED for s/n %s! Secure Element functions (RNG, etc.) NOT available"), eccSerial);
            return false;
        }
        if (!ECCX08.lock()) {
            Log.errorln(F("Locking ECCX08 configuration FAILED for s/n %s! Secure Element functions (RNG, etc.) NOT available"), eccSerial);
            return false;
        }
        Log.infoln(F("ECCX08 secure element s/n %s has been locked successfully!"), eccSerial);
    }
    Log.infoln(F("ECCX08 secure element OK! (s/n %s)"), eccSerial);
    setSysStatus(SYS_STATUS_ECC);
    return true;
}



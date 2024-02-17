//
// Copyright (c) 2023,2024 by Dan Luca. All rights reserved
//
/**
 * Category C of light effects
 *
 */

#include "fxC.h"

using namespace FxC;
using namespace colTheme;

//~ Effect description strings stored in flash
const char fxc1Desc[] PROGMEM = "FXC1: blend between two concurrent animations";
const char fxc2Desc[] PROGMEM = "FXC2: blur function";

void FxC::fxRegister() {
    static FxC1 fxC1;
    static FxC2 fxC2;
}

/**
 * aanimations
 * By: Can't recall where I found this. Maybe Stefan Petrick.
 * Modified by: Andrew Tuline
 *
 * Date: January, 2017
 * This sketch demonstrates how to blend between two animations running at the same time.
 */
FxC1::FxC1() : LedEffect(fxc1Desc), setA(frame(0, FRAME_SIZE-1)), setB(leds, FRAME_SIZE) {
}

void FxC1::setup() {
    LedEffect::setup();
    brightness = 176;
}

void FxC1::run() {
    animationA();
    animationB();
    CRGBSet others(leds, setB.size(), NUM_PIXELS);

    //combine all into setB (it is backed by the strip)
    uint8_t ratio = beatsin8(2);
    for (uint16_t x = 0; x < setB.size(); x++) {
        setB[x] = blend(setA[x], setB[x], ratio);
    }
    replicateSet(setB, others);

    FastLED.show(stripBrightness);
}

void FxC1::animationA() {
    for (uint16_t x = 0; x<setA.size(); x++) {
        uint8_t clrIndex = (millis() / 10) + (x * 12);    // speed, length
        if (clrIndex > 128) clrIndex = 0;
        setA[x] = ColorFromPalette(palette, clrIndex, dim8_raw(clrIndex << 1), LINEARBLEND);
    }
}

void FxC1::animationB() {
    for (uint16_t x = 0; x<setB.size(); x++) {
        uint8_t clrIndex = (millis() / 5) - (x * 12);    // speed, length
        if (clrIndex > 128) clrIndex = 0;
        setB[x] = ColorFromPalette(palette, 255-clrIndex, dim8_raw(clrIndex << 1), LINEARBLEND);
    }
}

bool FxC1::windDown() {
    return transEffect.offWipe(true);
}

uint8_t FxC1::selectionWeight() const {
    return 35;
}
//Fx C2
/**
 * blur
 * By: ????
 * Modified by: Andrew Tuline
 *
 * Date: July, 2015
 * Let's try the blur function. If you look carefully at the animation, sometimes there's a smooth gradient between each LED.
 * Other times, the difference between them is quite significant. Thanks for the blur.
 *
 */

FxC2::FxC2() : LedEffect(fxc2Desc) {}

//void FxC2::setup() {
//    LedEffect::setup();
//}

void FxC2::run() {
    uint8_t blurAmount = dim8_raw( beatsin8(3,64, 192) );       // A sinewave at 3 Hz with values ranging from 64 to 192.
    tpl.blur1d(blurAmount);                        // Apply some blurring to whatever's already on the strip, which will eventually go black.

    uint16_t  i = beatsin16(  9, 0, tpl.size()-1);
    uint16_t  j = beatsin16( 7, 0, tpl.size()-1);
    uint16_t  k = beatsin16(  5, 0, tpl.size()-1);

    // The color of each point shifts over time, each at a different speed.
    uint16_t ms = millis();
    leds[(i+j)/2] = paletteFactory.isHolidayLimitedHue() ? ColorFromPalette(palette, ms/29) : CHSV( ms / 29, 200, 255);
    leds[(j+k)/2] = paletteFactory.isHolidayLimitedHue() ? ColorFromPalette(palette, ms/41) : CHSV( ms / 41, 200, 255);
    leds[(k+i)/2] = paletteFactory.isHolidayLimitedHue() ? ColorFromPalette(palette, ms/73) : CHSV( ms / 73, 200, 255);
    leds[(k+i+j)/3] = paletteFactory.isHolidayLimitedHue() ? ColorFromPalette(palette, ms/53) : CHSV( ms / 53, 200, 255);

    replicateSet(tpl, others);
    FastLED.show(stripBrightness);
}

void FxC2::windDownPrep() {
    transEffect.prepare(SELECTOR_SPOTS);
}

uint8_t FxC2::selectionWeight() const {
    return 5;
}


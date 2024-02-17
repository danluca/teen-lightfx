//
// Copyright (c) 2023,2024 by Dan Luca. All rights reserved
//
/**
 * Category B of light effects
 *
 */
#include "fxB.h"

//~ Global variables definition for FxB
using namespace FxB;
using namespace colTheme;

//~ Effect description strings stored in flash
const char fxb1Desc[] PROGMEM = "FXB1: rainbow";
const char fxb2Desc[] PROGMEM = "FXB2: rainbow with glitter";
const char fxb3Desc[] PROGMEM = "FXB3: confetti B";

void FxB::fxRegister() {
    static FxB1 fxb1;
    static FxB2 fxB2;
    static FxB3 fxB3;
}

//FXB1
FxB1::FxB1() : LedEffect(fxb1Desc) {}

void FxB1::setup() {
    LedEffect::setup();
    hue = 0;
    brightness = 148;
    transEffect.prepare(random8());
}

void FxB1::run() {
    EVERY_N_MILLISECONDS(60) {
        rainbow();
        FastLED.show(stripBrightness);
        hue += 2;
    }
}

void FxB::rainbow() {
    if (paletteFactory.isHolidayLimitedHue())
        tpl.fill_gradient_RGB(ColorFromPalette(palette, hue, brightness),
                              ColorFromPalette(palette, hue + 128, brightness),
                              ColorFromPalette(palette, 255 - hue, brightness));
    else {
        tpl.fill_rainbow(hue, 7);
        tpl.nscale8(brightness);
    }
    replicateSet(tpl, others);
}

JsonObject &FxB1::describeConfig(JsonArray &json) const {
    JsonObject obj = LedEffect::describeConfig(json);
    obj["brightness"] = brightness;
    return obj;
}

uint8_t FxB1::selectionWeight() const {
    return 15;
}

//FXB2
FxB2::FxB2() : LedEffect(fxb2Desc) {}

void FxB2::setup() {
    LedEffect::setup();
    hue = 0;
    brightness = 148;
    transEffect.prepare(random8());
}

void FxB2::run() {
    EVERY_N_MILLISECONDS(60) {
        rainbowWithGlitter();
        FastLED.show(stripBrightness);
        hue += 2;
    }
}

/**
 * Built-in FastLED rainbow, plus some random sparkly glitter.
 */
void FxB::rainbowWithGlitter() {
    rainbow();
    addGlitter(80);
}

void FxB::addGlitter(fract8 chanceOfGlitter) {
    if (random8() < chanceOfGlitter) {
        leds[random16(NUM_PIXELS)] += CRGB::White;
    }
}

uint8_t FxB2::selectionWeight() const {
    return 40;
}

//FXB3
FxB3::FxB3() : LedEffect(fxb3Desc) {}

void FxB3::setup() {
    LedEffect::setup();
    hue = 0;
    brightness = 148;
    transEffect.prepare(random8());
}

void FxB3::run() {
    if (mode == TurnOff) {
        if (transEffect.transition())
            mode = Chase;
        else
            return;
    }

    EVERY_N_MILLISECONDS(50) {
        fxb_confetti();
    }
    EVERY_N_SECONDS(133) {
        if (countPixelsBrighter(&tpl) > 10)
            mode = TurnOff;
    }
}

void FxB::fxb_confetti() {
    // Random colored speckles that blink in and fade smoothly.
    tpl.fadeToBlackBy(10);
    uint16_t pos = random16(tpl.size());
    if (paletteFactory.isHolidayLimitedHue())
        tpl[pos] += ColorFromPalette(palette, hue + random8(64));
    else
        tpl[pos] += CHSV(hue + random8(64), 200, 255);
    replicateSet(tpl, others);
    FastLED.show(stripBrightness);
    hue += 2;
}

JsonObject &FxB3::describeConfig(JsonArray &json) const {
    JsonObject obj = LedEffect::describeConfig(json);
    obj["brightness"] = brightness;
    return obj;
}

uint8_t FxB3::selectionWeight() const {
    return 24;
}


//
// Copyright (c) 2023,2024 by Dan Luca. All rights reserved
//
/**
 * Category A of light effects
 * 
 */
#include "fxA.h"

//~ Global variables definition for FxA
using namespace FxA;
using namespace colTheme;

//~ Effect description strings stored in flash
const char fxa1Desc[] PROGMEM = FX_SLEEPLIGHT_ID ": Sleep light";
const char fxa2Desc[] PROGMEM = FX_QUIET_ID ": Quiet";

void FxA::fxRegister() {
    static SleepLight sleepLight;
    static Quiet quiet;
}

// SleepLight
SleepLight::SleepLight() : LedEffect(fxa1Desc) {
    fxRegistry.registerEffect(this);
}

void SleepLight::setup() {
    LedEffect::setup();
    color = colorIndex;
    lightIntensity = brightness;
}

void SleepLight::run() {
    EVERY_N_SECONDS(1) {
        CRGBSet strip(leds, NUM_PIXELS);
        strip = ColorFromPalette(paletteFactory.sleepPalette(), color, brightness, LINEARBLEND);
    }
    EVERY_N_MINUTES(2) {
        lightIntensity = lightIntensity <= minBrightness ? minBrightness : lightIntensity - 3;
    }
    EVERY_N_SECONDS(5) {
        color++;
    }
    EVERY_N_MILLIS(100) {
        //linear interpolation of beat amplitude - our raw light intensities are between 128 and 32
        uint8_t variation = (brighten8_raw(lightIntensity) - 31)*(25-5)/(128-32);
        FastLED.show(lightIntensity + beatsin8(7, 0, variation));
    }
}

uint8_t SleepLight::selectionWeight() const {
    return LedEffect::selectionWeight();
}

void SleepLight::windDownPrep() {
    transEffect.prepare(SELECTOR_FADE);
}

// Quiet
void Quiet::setup() {
    LedEffect::setup();
}

void Quiet::run() {
    EVERY_N_SECONDS(30) {
        CRGBSet strip(leds, NUM_PIXELS);
        strip = CRGB::Black;
        FastLED.show(0);
    }
}

uint8_t Quiet::selectionWeight() const {
    return LedEffect::selectionWeight();
}

Quiet::Quiet() : LedEffect(fxa2Desc) {
    fxRegistry.registerEffect(this);
}

void Quiet::windDownPrep() {
    transEffect.prepare(SELECTOR_FADE);
}

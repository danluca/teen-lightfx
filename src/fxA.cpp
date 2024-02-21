//
// Copyright (c) 2023,2024 by Dan Luca. All rights reserved
//
/**
 * Category A of light effects
 * 
 */
#include "fxA.h"
#include "log.h"

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
    clrX = random8();
    lightIntensity = brightness-16;
    lightVar = 0;
    overlay = 48;
    colorBuf = ColorFromPalette(paletteFactory.sleepPalette(), clrX, lightIntensity, LINEARBLEND);
}

void SleepLight::run() {
    EVERY_N_MINUTES(2) {
        lightIntensity = lightIntensity <= minBrightness ? minBrightness : lightIntensity - 3;
        lightVar = 3 + (lightIntensity - minBrightness)*(32-5)/(brightness-minBrightness);
        colorBuf = ColorFromPalette(paletteFactory.sleepPalette(), ++clrX, brightness, LINEARBLEND);
        leds[0] = colorBuf;
        Log.infoln(F("SleepLight parameters: lightIntensity=%d, lightVariance=%d, colorIndex=%d, color=%r"), lightIntensity, lightVar, clrX, colorBuf);
    }
    EVERY_N_MILLIS(60) {
        //linear interpolation of beat amplitude
        CRGBSet strip(leds, NUM_PIXELS);
        uint8_t lightDelta = beatsin8(5, 0, lightVar);
        CRGB curClrBuf = colorBuf;
        colorBuf = ColorFromPalette(paletteFactory.sleepPalette(), clrX, lightIntensity+lightDelta, LINEARBLEND);
        overlay = colorBuf == curClrBuf ? overlay : 48;

        CRGB &curClr = strip[0];
        nblend(curClr, colorBuf, overlay);
        overlay = qadd8(overlay, 16);
        strip = curClr;
        FastLED.show();
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

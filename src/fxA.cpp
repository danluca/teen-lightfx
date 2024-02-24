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

uint8_t excludeActiveColors(uint8_t hue) {
    uint8_t res = hue;
    if (res <= HUE_ORANGE)
        res = HUE_ORANGE*2-res;
    if (res >= HUE_BLUE && res <= HUE_PURPLE)
        res = HUE_BLUE*2-res;
    if (res >= (HUE_PINK + 16))
        res -= 16;
    return res;
}

void SleepLight::setup() {
    LedEffect::setup();
    colTheme::PaletteFactory::toHSVPalette(palette, colTheme::PaletteFactory::sleepPalette());
    hue = excludeActiveColors(secRandom8());
    sat = secRandom8(24, 128);
    clrX = brightness;
    lightIntensity = brightness;
    lightVar = 0;
    colorBuf = ColorFromPalette(palette, clrX, lightIntensity, LINEARBLEND);
}

void SleepLight::run() {
    EVERY_N_SECONDS(30) {
        lightIntensity = lightIntensity <= minBrightness ? minBrightness : lightIntensity - 1;
        lightVar = 2 + (lightIntensity - minBrightness)*(32-5)/(brightness-minBrightness);
        colorBuf.hue = hue;
        colorBuf.sat = sat;
        hue = excludeActiveColors(hue++);
        sat = 24 + (sat+1)%104;
        Log.infoln(F("SleepLight parameters: lightIntensity=%d, lightVariance=%d, hue=%d, sat=%d, color=%r"), lightIntensity, lightVar, hue, sat, colorBuf);
    }
    EVERY_N_SECONDS(5) {
        clrX = clrX > lightIntensity ? lightIntensity : lightIntensity+lightVar;
    }
    EVERY_N_MILLIS(125) {
        //linear interpolation of beat amplitude
        CRGBSet strip(leds, NUM_PIXELS);
        CHSV hsv = colorBuf;
        rblend8(hsv.val, clrX);
        strip = hsv;
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

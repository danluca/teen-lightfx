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
    clrX = secRandom8();
    hue = excludeActiveColors(clrX);
    sat = secRandom8(24, 128);
    lightIntensity = brightness;
    lightVar = 30;
    colorBuf = ColorFromPalette(palette, secRandom8());
    FastLED.setTemperature(ColorTemperature::Candle);
}

void SleepLight::run() {
    EVERY_N_SECONDS(30) {
        lightIntensity = lightIntensity <= minBrightness ? minBrightness : lightIntensity - 1;
        lightVar = 2 + (lightIntensity - minBrightness)*(36-5)/(brightness-minBrightness);
        colorBuf.hue = hue;
        colorBuf.sat = sat;
        //colorBuf.val = lightIntensity;
        hue = excludeActiveColors(++clrX);
        sat = map(lightIntensity, minBrightness, brightness, 20, 96);
        Log.infoln(F("SleepLight parameters: lightIntensity=%d, lightVariance=%d, hue=%d, sat=%d, color=%r"), lightIntensity, lightVar, hue, sat, colorBuf);
    }
    EVERY_N_MILLIS(60) {
        //linear interpolation of beat amplitude
        double dBreath = (exp(sin(millis()/2400.0*PI)) - 0.36787944)*108.0;
        uint8_t breathLum = map(dBreath, 0, 255, lightIntensity, lightIntensity+lightVar);
        colorBuf.val = breathLum;
        CRGB clr = colorBuf;
        if (lightIntensity < (minBrightness+8)) {
            segUp.fadeToBlackBy(3);
            segRight(0, 19).fadeToBlackBy(3);
            segFront(0, 15).fadeLightBy(3);
            segLeft(0, 18).fadeToBlackBy(3);
            segBack(0, 15).fadeToBlackBy(3);

            segRight(20, segRight.size()-1) = clr;
            segFront(16, segFront.size()-1) = clr;
            segLeft(19, segLeft.size()-1) = clr;
            segBack(16, segBack.size()-1) = clr;
        } else {
            segUp = clr;
            segRight = clr;
            segFront = clr;
            segLeft = clr;
            segBack = clr;
        }
        //Log.infoln("dBreath=%F, breathLum=%d current lum=%d, colorBuf=%r, rgb=%r", dBreath, breathLum, lightIntensity, colorBuf, clr);
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

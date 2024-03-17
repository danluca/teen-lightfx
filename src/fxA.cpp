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
SleepLight::SleepLight() : LedEffect(fxa1Desc), state(Fade), refPixel(&segRight[segRight.size()-1]),
        slOffRight(leds, SEG_RIGHT_START, SEG_RIGHT_START + (SEG_RIGHT_END-SEG_RIGHT_START)/2 + 1),
        slOffFront(leds, SEG_FRONT_START, SEG_FRONT_START + (SEG_FRONT_END-SEG_FRONT_START)/2 + 1),
        slOffLeft(leds, SEG_LEFT_START + (SEG_LEFT_END-SEG_LEFT_START)/2 + 1), slOffBack(leds, SEG_BACK_START, SEG_BACK_START+16) {
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
    timer=0;
    state = Fade;
    colorBuf.hue = excludeActiveColors(secRandom8());
    colorBuf.sat = secRandom8(24, 128);
    colorBuf.val = brightness;
    FastLED.setTemperature(ColorTemperature::Tungsten40W);
    fill_solid(leds, NUM_PIXELS, colorBuf);
}

uint8_t flrSub(uint8_t val, uint8_t sub, uint8_t floor) {
    uint8_t res = qsub8(val, sub);
    return res < floor ? floor : res;
}

void SleepLight::run() {
    EVERY_N_SECONDS(20) {
        if (state != Sleep && state != SleepTransition) {
            if (refPixel->getLuma() <= minBrightness)
                state = SleepTransition;
            else {
                colorBuf.hue = excludeActiveColors(colorBuf.hue + random8(2, 19));;
                colorBuf.val = flrSub(toHSV(*refPixel).val, 2, minBrightness);
                colorBuf.sat = map(colorBuf.val, minBrightness, brightness, 20, 96);;
                Log.infoln(F("SleepLight parameters: hue=%X, sat=%X, val=%X, color=%r"), colorBuf.hue, colorBuf.sat, colorBuf.val, colorBuf);
            }
        }
    }
    EVERY_N_MILLIS(125) {
        step();
        FastLED.show();
    }

}

SleepLight::SleepLightState SleepLight::step() {
    switch (state) {
        case Fade:
            if (refPixel->getLuma() > minBrightness)
                ledSet.fadeToBlackBy(1);
            else
                state = SleepTransition;
            break;
        case FadeColorTransition:
            if (lblend(*refPixel, (CRGB)colorBuf, 15))
                state = Fade;
            ledSet = *refPixel;
            break;
        case SleepTransition:
            timer = ++timer%12;
            if (timer == 0) {
                segUp.fadeToBlackBy(1);
                slOffRight.fadeToBlackBy(1);
                slOffFront.fadeToBlackBy(1);
                slOffLeft.fadeToBlackBy(1);
                slOffBack.fadeToBlackBy(1);
                if (segUp[0] == CRGB::Black)
                    state = Sleep;
            }
            break;
        case Sleep:
            break;
    }
    return state;
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

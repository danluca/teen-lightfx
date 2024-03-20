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
SleepLight::SleepLight() : LedEffect(fxa1Desc), state(Fade), refPixel(&segRight[segRight.size()-1]) {
    slOffSegs.push_front(segUp(0, segUp.size()-4));
    slOffSegs.push_front(segRight(5, segRight.size()/2-2));
    slOffSegs.push_front(segRight(segRight.size()/2+2, segRight.size()-6));
    slOffSegs.push_front(segFront(5, segFront.size()/2-2));
    slOffSegs.push_front(segFront(segFront.size()/2+2, segFront.size()-6));
    slOffSegs.push_front(segLeft(5, segLeft.size()/2-2));
    slOffSegs.push_front(segLeft(segLeft.size()/2+2, segLeft.size()-6));
    slOffSegs.push_front(segBack(5, segBack.size()/2-2));
    slOffSegs.push_front(segBack(segBack.size()/2+2, segBack.size()-10));
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
    FastLED.setTemperature(ColorTemperature::Tungsten40W);
    fill_solid(leds, NUM_PIXELS, colorBuf);
    timer=0;
    state = FadeColorTransition;
    colorBuf.hue = excludeActiveColors(secRandom8());
    colorBuf.sat = secRandom8(24, 128);
    colorBuf.val = brightness;
    Log.infoln(F("SleepLight setup: colorBuf=%r, hue=%d, sat=%d, val=%d"), (CRGB)colorBuf, colorBuf.hue, colorBuf.sat, colorBuf.val);
}

/**
 * Subtracts one number from another, preventing the number from dropping below a certain floor value.
 * The sanity check is accomplished via the qsub8 function.
 * @param val value to subtract from
 * @param sub value to subtract
 * @param floor min value not to go under
 * @return val-sub if greater than floor, floor otherwise
 */
uint8_t flrSub(uint8_t val, uint8_t sub, uint8_t floor) {
    uint8_t res = qsub8(val, sub);
    return res < floor ? floor : res;
}

void SleepLight::run() {
    if (state == Fade) {
        EVERY_N_SECONDS(21) {
            colorBuf.val = flrSub(colorBuf.val, 3, minBrightness);
            state = colorBuf.val > minBrightness ? FadeColorTransition : SleepTransition;
            Log.infoln(F("SleepLight parameters: state=%d, colorBuf=%r HSV=(%d,%d,%d), refPixel=%r"), state, (CRGB)colorBuf, colorBuf.hue, colorBuf.sat, colorBuf.val, *refPixel);
        }
        EVERY_N_SECONDS(12) {
            colorBuf.hue = excludeActiveColors(colorBuf.hue + random8(2, 19));
            colorBuf.sat = map(colorBuf.val, minBrightness, brightness, 20, 96);
            state = colorBuf.val > minBrightness ? FadeColorTransition : SleepTransition;
            Log.infoln(F("SleepLight parameters: state=%d, colorBuf=%r HSV=(%d,%d,%d), refPixel=%r"), state, (CRGB)colorBuf, colorBuf.hue, colorBuf.sat, colorBuf.val, *refPixel);
        }
    }
    EVERY_N_MILLIS(125) {
        SleepLightState oldState = step();
        if (!(oldState == state && state == Sleep))
            FastLED.show();
    }

}

SleepLight::SleepLightState SleepLight::step() {
    SleepLightState oldState = state;
    switch (state) {
        case FadeColorTransition:
            if (rblend(*refPixel, (CRGB) colorBuf, 7))
                state = Fade;
            ledSet = *refPixel;
            break;
        case SleepTransition:
            timer = ++timer%12;
            if (timer == 0) {
                for (auto &seg : slOffSegs)
                    seg.fadeToBlackBy(1);
                if (segUp[0] == CRGB::Black)
                    state = Sleep;
            }
            break;
        default:
            break;
    }
    if (oldState != state)
        Log.infoln(F("SleepLight state changed from %d to %d, colorBuf=%r, refPixel=%r"), oldState, state, (CRGB)colorBuf, *refPixel);
    return oldState;
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

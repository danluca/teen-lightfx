//
// Copyright (c) 2023,2024 by Dan Luca. All rights reserved
//
/**
 * Category F of light effects
 *
 */
#include "fxF.h"
#include <vector>
#include <algorithm>

using namespace FxF;
using namespace colTheme;

//~ Effect description strings stored in flash
const char fxf1Desc[] PROGMEM = "FxF1: beat wave";
const char fxf5Desc[] PROGMEM = "FxF5: Fireworks";

void FxF::fxRegister() {
    static FxF1 fxF1;
    static FxF5 fxF5;
}

// FxF1
FxF1::FxF1() : LedEffect(fxf1Desc) {}

void FxF1::setup() {
    LedEffect::setup();
    speed = 60;
    fade = 96;
    hue = random8();
    hueDiff = 8;
}

void FxF1::run() {
    EVERY_N_MILLISECONDS(speed) {
        const uint8_t dotSize = 2;
        tpl.fadeToBlackBy(fade);

        uint16_t w1 = (beatsin16(12, 0, tpl.size()-dotSize-1) + beatsin16(24, 0, tpl.size()-dotSize-1))/2;
        uint16_t w2 = beatsin16(14, 0, tpl.size()-dotSize-1, 0, beat8(10)*128);

        CRGB clr1 = ColorFromPalette(palette, hue, brightness, LINEARBLEND);
        CRGB clr2 = ColorFromPalette(targetPalette, hue, brightness, LINEARBLEND);

        CRGBSet seg1 = tpl(w1, w1+dotSize);
        seg1 = clr1;
        seg1.blur1d(64);
        CRGBSet seg2 = tpl(w2, w2+dotSize);
        seg2 |= clr2;

        replicateSet(tpl, others);
        FastLED.show(stripBrightness);
        hue += hueDiff;
    }
}

bool FxF1::windDown() {
    return transEffect.offSpots();
}

uint8_t FxF1::selectionWeight() const {
    return 12;
}

// FxF5 - algorithm by Carl Rosendahl, adapted from code published at https://www.anirama.com/1000leds/1d-fireworks/
// HEAVY floating point math
FxF5::FxF5() : LedEffect(fxf5Desc) {}

void FxF5::run() {
    EVERY_N_MILLIS_I(fxf5Timer, 1000) {
        flare();

        explode();

        fxf5Timer.setPeriod(random16(1000, 4000));
    }
}

void FxF5::setup() {
    LedEffect::setup();
}

/**
 * Send up a flare
 */
void FxF5::flare() {
    const ushort flareSparksCount = 3;
    float flareStep = flarePos = 0;
    bFade = random8() % 2;
    curPos = random16(tpl.size()*explRangeLow/10, tpl.size()*explRangeHigh/10);
    float flareVel = float(random16(400, 650)) / 1000; // trial and error to get reasonable range to match the 30-80 % range of the strip height we want
    float flBrightness = 255;
    Spark flareSparks[flareSparksCount];

    // initialize launch sparks
    for (auto &spark : flareSparks) {
        spark.pos = 0;
        spark.velocity = (float(random8(180,255)) / 255) * (flareVel / 2);
        // random around 20% of flare velocity
        spark.hue = uint8_t(spark.velocity * 1000);
    }
    // launch
    while ((ushort(flarePos) < curPos) && (flareVel > 0)) {
        tpl = BKG;
        // sparks
        for (auto &spark : flareSparks) {
            spark.pos += spark.velocity;
            spark.limitPos(curPos);
            spark.velocity += gravity;
            spark.hue = capd(qsuba(spark.hue, 1), 64);
            tpl[spark.iPos()] = HeatColor(spark.hue);
            tpl[spark.iPos()] %= 50; // reduce brightness to 50/255
        }

        // flare
        flarePos = easeOutQuad(ushort(flareStep), curPos);
        tpl[ushort(flarePos)] = CHSV(0, 0, ushort(flBrightness));
        replicateSet(tpl, others);
        flareStep += flareVel;
        //flarePos = constrain(flarePos, 0, curPos);
        flareVel += gravity;
        flBrightness *= .985;

        FastLED.show(stripBrightness);
    }
}

/**
 * Explode!
 *
 * Explosion happens where the flare ended.
 * Size is proportional to the height.
 */
void FxF5::explode() const {
    const auto nSparks = ushort(flarePos / 3); // works out to look about right
    Spark sparks[nSparks];
    //map the flare position in its range to a hue
    uint8_t decayHue = constrain(map(ushort(flarePos), tpl.size()*explRangeLow/10, tpl.size()*explRangeHigh/10, 0, 255), 0, 255);
    uint8_t flarePosQdrnt = decayHue/64;

    // initialize sparks
    for (auto &spark : sparks) {
        spark.pos = flarePos;
        spark.velocity = (float(random16(0, 20000)) / 10000.0f) - 1.0f; // from -1 to 1
        spark.hue = random8(flarePosQdrnt*64, 64+flarePosQdrnt*64);   //limit the spark hues in a closer color range based on flare height
        spark.velocity *= flarePos/1.7f/ float(tpl.size()); // proportional to height
    }
    // the original implementation was to designate a known spark starting from a known value and iterate until it goes below a fixed threshold - c2/128
    // since they were all fixed values, the math shows the number of iterations can be precisely determined. The formula is iterCount = log(c2/128/255)/log(degFactor),
    // rounded up to nearest integer. For instance, for original values of c2=50, degFactor=0.99, we're looking at 645 loops. With some experiments, I've landed
    // at c2=30, degFactor=0.987, looping at 535 loops.
    const ushort loopCount = 540;
    float dying_gravity = gravity;
    ushort iter = 0;
    bool activeSparks = true;
    while((iter++ < loopCount) && activeSparks) {
        if (bFade)
            tpl.fadeToBlackBy(9);
        else
            tpl = BKG;
        activeSparks = false;
        for (auto &spark : sparks) {
            if (spark.iPos() == 0)
                continue;   //if this spark has reached bottom, save our breath
            activeSparks = true;
            spark.pos += spark.velocity;
            spark.limitPos(float(tpl.size()-1));
            spark.velocity += dying_gravity;
            //spark.colorFade *= .987f;       //degradation factor degFactor in formula above
            //fade the sparks
//            auto spDist = uint8_t(abs(spark.pos - flarePos) * 255 / flarePos);
            auto spDist = uint8_t(abs(spark.pos - flarePos));
            ushort tplPos = spark.iPos();
            if (bFade) {
                tpl[tplPos] += ColorFromPalette(palette, spark.hue+spDist, 255-2*spDist);
                //tpl.blur1d();
            } else {
                tpl[tplPos] = blend(ColorFromPalette(palette, spark.hue),
                                    CHSV(decayHue, 224, 255-2*spDist),
                                    3*spDist);
            }
        }

        dying_gravity *= 0.985; // as sparks burn out they fall slower
        replicateSet(tpl, others);
        FastLED.show(stripBrightness);
    }
    tpl = BKG;
    replicateSet(tpl, others);
    FastLED.show(stripBrightness);
}

bool FxF5::windDown() {
    return transEffect.offWipe(true);
}

uint8_t FxF5::selectionWeight() const {
    return paletteFactory.getHoliday() == Halloween ? 10 : 64;
}

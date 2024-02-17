//
// Copyright (c) 2023,2024 by Dan Luca. All rights reserved
//
/**
 * Category D of light effects
 *
 */

#include "fxD.h"

using namespace FxD;
using namespace colTheme;

//~ Effect description strings stored in flash
const char fxd3Desc[] PROGMEM = "FxD3: plasma";
const char fxd4Desc[] PROGMEM = "FxD4: rainbow marching";
const char fxd5Desc[] PROGMEM = "FxD5: ripples";

void FxD::fxRegister() {
    static FxD3 fxD3;
    static FxD4 fxD4;
    static FxD5 fxD5;
}

// Fx D3
void FxD3::setup() {
    LedEffect::setup();
    targetPalette = paletteFactory.mainPalette();
    palette = paletteFactory.secondaryPalette();
    monoColor = random8(224);   //colors above this index in the Halloween palette are black
}

void FxD3::run() {
    EVERY_N_MILLISECONDS(50) {                                  // FastLED based non-blocking delay to update/display the sequence.
        plasma();
        FastLED.show(stripBrightness);
    }

    EVERY_N_SECONDS(5) {
        nblendPaletteTowardPalette(palette, targetPalette, maxChanges);
    }

    if (paletteFactory.isHolidayLimitedHue()) {
        EVERY_N_SECONDS(45) {
            monoColor = random8(224);
        }
    } else {
        EVERY_N_SECONDS(30) {
            targetPalette = PaletteFactory::randomPalette(random8());
        }
    }

}

void FxD3::plasma() {
    uint8_t thisPhase = beatsin8(6,-64,64);                           // Setting phase change for a couple of waves.
    uint8_t thatPhase = beatsin8(7,-64,64);

    for (int k=0; k<NUM_PIXELS; k++) {                              // For each of the LED's in the strand, set a localBright based on a wave as follows:
        uint8_t colorIndex = cubicwave8((k*23)+thisPhase)/2 + cos8((k*15)+thatPhase)/2;           // Create a wave and add a phase change and add another wave with its own phase change.. Hey, you can even change the frequencies if you wish.
        uint8_t thisBright = qsuba(colorIndex, beatsin8(7,0,96));              // qsub gives it a bit of 'black' dead space by setting sets a minimum value. If colorIndex < current value of beatsin8(), then bright = 0. Otherwise, bright = colorIndex..
        //plasma becomes slime during Halloween (single color morphing mass)
        uint8_t clr = paletteFactory.isHolidayLimitedHue() ? monoColor : colorIndex;
        leds[k] = ColorFromPalette(palette, clr, thisBright, LINEARBLEND);  // Let's now add the foreground colour.
    }
}

FxD3::FxD3() : LedEffect(fxd3Desc) {}

void FxD3::windDownPrep() {
    transEffect.prepare(SELECTOR_WIPE + random8());
}

uint8_t FxD3::selectionWeight() const {
    return 24;
}

// Fx D4
FxD4::FxD4() : LedEffect(fxd4Desc) {}

void FxD4::setup() {
    LedEffect::setup();
    hue = 0;
    hueDiff = 1;
}

void FxD4::run() {
    static uint8_t secSlot = 0;

    EVERY_N_SECONDS(5) {
        update_params(secSlot);
        secSlot = inc(secSlot, 1, 15);
    }

    EVERY_N_MILLISECONDS(50) {
        rainbow_march();
        FastLED.show(stripBrightness);
    }
}

void FxD4::update_params(uint8_t slot) {
    switch (slot) {
        case 0: rot = 1; hueDiff = 5; break;
        case 2: dirFwd = true; hueDiff = 10; break;
        case 4: rot = 5; dirFwd=false; break;
        case 6: rot = 5; dirFwd = true;hueDiff = 20; break;
        case 9: hueDiff = 30; break;
        case 12: hueDiff = 2;rot = 5; break;
        default: break;
    }
}

void FxD4::rainbow_march() {
    if (dirFwd) hue += rot; else hue-= rot;                                       // I could use signed math, but 'dirFwd' works with other routines.
    if (paletteFactory.isHolidayLimitedHue())
        tpl.fill_gradient_RGB(ColorFromPalette(palette, hue, brightness),
          ColorFromPalette(palette, hue+128, brightness),
          ColorFromPalette(palette, 255-hue, brightness));
    else {
        tpl.fill_rainbow(hue, hueDiff);           // I don't change hueDiff on the fly as it's too fast near the end of the strip.
        tpl.nscale8(brightness);
    }
    replicateSet(tpl, others);
}

bool FxD4::windDown() {
    return transEffect.offSpots();
}

uint8_t FxD4::selectionWeight() const {
    return 18;
}

// Fx D5
FxD5::FxD5() : LedEffect(fxd5Desc) {}

void FxD5::setup() {
    LedEffect::setup();
}

void FxD5::run() {
    EVERY_N_SECONDS(2) {
        nblendPaletteTowardPalette(palette, targetPalette, maxChanges);
    }
    EVERY_N_MILLISECONDS(100) {
        ripples();
        FastLED.show(stripBrightness);
    }
}

void FxD5::ripples() {
    //fadeToBlackBy(leds, NUM_PIXELS, fade);                             // 8 bit, 1 = slow, 255 = fast
    for (auto & r : ripplesData) {
        if (random8() > 224 && !r.Alive()) {
            r.Init(&tpl);
        }
    }

    for (auto & r : ripplesData) {
        if (r.Alive()) {
            r.Fade();
            r.Move();
        }
    }
    replicateSet(tpl, others);
}

void FxD5::windDownPrep() {
    transEffect.prepare(SELECTOR_WIPE + random8());
}

// ripple structure API
void ripple::Move() {
    if (step == 0) {
        (*pSeg)[center] = ColorFromPalette(palette, color, rpBright, LINEARBLEND);
    } else if (step < 12) {
        uint16_t x = (center + step) % pSeg->size();
        x = (center + step) >= pSeg->size() ? (pSeg->size() - x - 1) : x;        // we want the "wave" to bounce back from the end, rather than start from the other end
        (*pSeg)[x] += ColorFromPalette(palette, color + 16, rpBright*2/step, LINEARBLEND);       // Simple wrap from Marc Miller
        x = asub(center, step) % pSeg->size();
        (*pSeg)[x] += ColorFromPalette(palette, color + 16, rpBright*2/step, LINEARBLEND);
    }
    step++;  // Next step.
}

void ripple::Fade() const {
    uint16_t lowEndRipple = qsuba(center, step);
    uint16_t upEndRipple = capu(center + step, pSeg->size()-1);
    (*pSeg)(lowEndRipple, upEndRipple).fadeToBlackBy(rpFade);
}

bool ripple::Alive() const {
    return step < 42;
}

void ripple::Init(CRGBSet *set) {
    pSeg = set;
    center = random8(pSeg->size() / 8, pSeg->size() - pSeg->size() / 8);          // Avoid spawning too close to edge.
    rpBright = random8(192, 255);                                   // upper range of localBright
    color = random8();
    rpFade = random8(25, 80);
    step = 0;
}

uint8_t FxD5::selectionWeight() const {
    return 42;
}

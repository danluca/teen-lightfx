//
// Copyright (c) 2023,2024 by Dan Luca. All rights reserved
//
/**
 * Category E of light effects
 *
 */
#include "fxE.h"

using namespace FxE;
using namespace colTheme;

//~ Effect description strings stored in flash
const char fxe4Desc[] PROGMEM = "FxE4: serendipitous";

void FxE::fxRegister() {
    static FxE4 fxE4;
}

//Fx E4
FxE4::FxE4() : LedEffect(fxe4Desc) {}

void FxE4::setup() {
    LedEffect::setup();
    X = Xorig;
    Y = Yorig;
}

void FxE4::run() {
    EVERY_N_SECONDS(2) {
        nblendPaletteTowardPalette(palette, targetPalette, maxChanges);
    }

    if (!paletteFactory.isHolidayLimitedHue()) {
        EVERY_N_SECONDS(30) {
            targetPalette = PaletteFactory::randomPalette(random8());
        }
    }

    EVERY_N_MILLISECONDS(50) {
        serendipitous();
        FastLED.show(stripBrightness);
    }
}

void FxE4::serendipitous() {
    //  Xn = X-(Y/2); Yn = Y+(Xn/2);
    //  Xn = X-Y/2;   Yn = Y+Xn/2;
    uint16_t Xn = X-(Y/2); uint16_t Yn = Y+(X/2.1); uint16_t Zn = X + Y*2.3;
    //    Xn = X-(Y/3); Yn = Y+(X/1.5);
    //  Xn = X-(2*Y); Yn = Y+(X/1.1);

    X = Xn;
    Y = Yn;

    index=(sin8(X)+cos8(Y))/2;
    CRGB newcolor = ColorFromPalette(palette, index, map(Zn, 0, 65535, dimmed*3, brightness), LINEARBLEND);

    nblend(tpl[map(X, 0, 65535, 0, tpl.size()-1)], newcolor, 224);    // Try and smooth it out a bit. Higher # means less smoothing.
    tpl.fadeToBlackBy(16);                    // 8 bit, 1 = slow, 255 = fast
    replicateSet(tpl, others);
}

void FxE4::windDownPrep() {
    transEffect.prepare(random8());
}

uint8_t FxE4::selectionWeight() const {
    return 36;
}


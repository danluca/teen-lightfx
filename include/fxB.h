//
// Copyright 2023,2024 by Dan Luca. All rights reserved
//
#ifndef TEEN_LIGHTFX_FXB_H
#define TEEN_LIGHTFX_FXB_H

#include "efx_setup.h"

namespace FxB {
    void fadein();

    void rainbow();

    void rainbowWithGlitter();

    void addGlitter(fract8 chanceOfGlitter);

    void fxb_confetti();

    void sinelon();

    void bpm();

    void juggle_short();

    void juggle_long();

    void ease();


    class FxB1 : public LedEffect {
    public:
        FxB1();

        void setup() override;

        void run() override;

        JsonObject & describeConfig(JsonArray &json) const override;

        uint8_t selectionWeight() const override;
    };

    class FxB2 : public LedEffect {
    public:
        FxB2();

        void setup() override;

        void run() override;

        uint8_t selectionWeight() const override;
    };

    class FxB3 : public LedEffect {
    public:
        FxB3();

        void setup() override;

        void run() override;

        JsonObject & describeConfig(JsonArray &json) const override;

        uint8_t selectionWeight() const override;
    };
}

#endif //TEEN_LIGHTFX_FXB_H

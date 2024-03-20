//
// Copyright 2023,2024 by Dan Luca. All rights reserved
//
#ifndef TEEN_LIGHTFX_FXA_H
#define TEEN_LIGHTFX_FXA_H

#include "efx_setup.h"

namespace FxA {

    class SleepLight : public LedEffect {
    public:
        SleepLight();

        void setup() override;

        void run() override;

        void windDownPrep() override;

        uint8_t selectionWeight() const override;

        ~SleepLight() override = default;

    protected:
        enum SleepLightState:uint8_t {Fade, FadeColorTransition, SleepTransition, Sleep} state;
        CHSV colorBuf{};
        uint8_t timer{};
        CRGB* const refPixel;   //reference pixel - a pixel guaranteed to be lit/on at all times for this effect
        std::deque<CRGBSet> slOffSegs;

        SleepLightState step();
    };

    class Quiet : public LedEffect {
    public:
        Quiet();

        void setup() override;

        void run() override;

        void windDownPrep() override;

        uint8_t selectionWeight() const override;

        ~Quiet() override = default;
    };
}
#endif //TEEN_LIGHTFX_FXA_H

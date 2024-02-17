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

        uint8_t selectionWeight() const override;

        ~SleepLight() override = default;

    protected:
        uint8_t lightIntensity{};
        uint8_t color{};
    };

    class Quiet : public LedEffect {
    public:
        Quiet();

        void setup() override;

        void run() override;

        uint8_t selectionWeight() const override;

        ~Quiet() override = default;
    };
}
#endif //TEEN_LIGHTFX_FXA_H

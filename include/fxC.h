//
// Copyright 2023,2024 by Dan Luca. All rights reserved
//
#ifndef TEEN_LIGHTFX_FXC_H
#define TEEN_LIGHTFX_FXC_H

#include "efx_setup.h"

namespace FxC {
    class FxC1 : public LedEffect {
    private:
        CRGBSet setA, setB;

    public:
        FxC1();

        void setup() override;

        void run() override;

        bool windDown() override;

        void animationA();

        void animationB();

        uint8_t selectionWeight() const override;
    };

    class FxC2 : public LedEffect {
    public:
        FxC2();

        //void setup() override;

        void run() override;

        void windDownPrep() override;

        uint8_t selectionWeight() const override;
    };
}
#endif //TEEN_LIGHTFX_FXC_H

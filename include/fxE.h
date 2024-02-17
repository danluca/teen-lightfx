//
// Copyright 2023,2024 by Dan Luca. All rights reserved
//
#ifndef TEEN_LIGHTFX_FXE_H
#define TEEN_LIGHTFX_FXE_H

#include "efx_setup.h"

namespace FxE {
    class FxE4 : public LedEffect {
    public:
        FxE4();

        void setup() override;

        void run() override;

        void windDownPrep() override;

        void serendipitous();

        uint8_t selectionWeight() const override;

    protected:
        uint16_t Xorig = 0x012;
        uint16_t Yorig = 0x015;
        uint16_t X{}, Y{};
        uint8_t index{};
    };
}

#endif //TEEN_LIGHTFX_FXE_H

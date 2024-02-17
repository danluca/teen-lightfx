//
// Copyright 2023,2024 by Dan Luca. All rights reserved
//
#ifndef TEEN_LIGHTFX_FXF_H
#define TEEN_LIGHTFX_FXF_H

#include "efx_setup.h"

namespace FxF {
    class FxF1 : public LedEffect {
    public:
        FxF1();

        void setup() override;

        void run() override;

        bool windDown() override;

        uint8_t selectionWeight() const override;
    };

    struct Spark {
        float pos=0;
        float velocity=0;
        uint8_t hue=0;

        inline uint16_t iPos() const {
            return uint16_t(abs(pos));
        }
        inline float limitPos(float limit) {
            return pos = constrain(pos, 0, limit);
        }
    };

    class FxF5 : public LedEffect {
    public:
        explicit FxF5();

        void setup() override;

        void run() override;

        bool windDown() override;

        uint8_t selectionWeight() const override;

    protected:
        //Spark sparks[NUM_SPARKS]{};
        float flarePos{};
        bool bFade = false;
        const float gravity = -.004;    // m/s/s
        const ushort explRangeLow = 3;  //30%
        const ushort explRangeHigh = 8; //80%

        void flare();
        void explode() const;
    };
}
#endif //TEEN_LIGHTFX_FXF_H

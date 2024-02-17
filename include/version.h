//
// Copyright (c) 2023,2024 by Dan Luca. All rights reserved.
//

#ifndef TEEN_LIGHTFX_VERSION_H
#define TEEN_LIGHTFX_VERSION_H

#define MAIN_VERSION        "1.0.0"

#ifndef GIT_COMMIT

#define GIT_COMMIT          "N/A"
#define GIT_COMMIT_SHORT    "N/A"
#define GIT_BRANCH          "N/A"
#define BUILD_TIME          "N/A"

#endif

#define BUILD_VERSION   MAIN_VERSION "-" GIT_COMMIT_SHORT

#endif //TEEN_LIGHTFX_VERSION_H

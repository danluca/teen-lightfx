//
// Copyright 2023,2024 by Dan Luca. All rights reserved
//
#ifndef TEEN_LIGHTFX_NET_SETUP_H
#define TEEN_LIGHTFX_NET_SETUP_H

#include <WiFiNINA.h>
#include <Arduino_LSM6DSOX.h>
#include "PaletteFactory.h"
#include "util.h"
#include "secrets.h"

extern const CRGB CLR_ALL_OK;
extern const CRGB CLR_SETUP_IN_PROGRESS;
extern const CRGB CLR_SETUP_ERROR;

extern WiFiServer server;

bool wifi_setup();

bool imu_setup();
void server_setup();
void webserver();
void wifi_loop();
void printSuccessfulWifiStatus();
void checkFirmwareVersion();
uint8_t barSignalLevel(int32_t rssi);

#endif //TEEN_LIGHTFX_NET_SETUP_H

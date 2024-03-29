///////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright 2023 (c) Dan Luca
///////////////////////////////////////////////////////////////////////////////////////////////////
// Collection of light strip effects with ability to be configured through Wi-Fi
//
///////////////////////////////////////////////////////////////////////////////////////////////////
#include "efx_setup.h"
#include "log.h"
#include "net_setup.h"
#include "FxSchedule.h"
#include <SchedulerExt.h>

ThreadTasks fxTasks {fx_setup, fx_run};
ThreadTasks micTasks {mic_setup, mic_run};

void adc_setup() {
    //disable ADC
    //hw_clear_bits(&adc_hw->cs, ADC_CS_EN_BITS);
    //enable ADC, including temp sensor
    adc_init();
    adc_set_temp_sensor_enabled(true);
    analogReadResolution(ADC_RESOLUTION);   //get us the higher resolution of the ADC
}

/**
 * Setup LED strip and global data structures - executed once
 */
void setup() {
    delay(1000);    //safety delay
    log_setup();
    adc_setup();
    setupStateLED();

    stateLED(CLR_SETUP_IN_PROGRESS);    //Setup in progress

    fsInit();

    imu_setup();
    secElement_setup();

    Scheduler.startLoop(&fxTasks, 3072);
    Scheduler.startLoop(&micTasks, 1024);

    bool bSetupOk = wifi_setup();
    bSetupOk = bSetupOk && time_setup();
    if (bSetupOk)
        stateLED(CLR_ALL_OK);   //ready for awesome light management
    else
        stateLED(CLR_SETUP_ERROR);

    setupAlarmSchedule();
    Log.infoln(F("System status: %X"), getSysStatus());
}

/**
 * Main loop - runs the WebServer
 */
void loop() {
    wifi_loop();
    alarm_loop();
    yield();
}


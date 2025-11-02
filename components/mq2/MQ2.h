#pragma once

#include "driver/adc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cmath>
#include <stdio.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"


/**
 * @brief MQ2 Gas Sensor class for ESP-IDF
 * 
 * Reads LPG, CO, and Smoke values via analog ADC pin.
 */
class MQ2 {
public:
    MQ2(adc1_channel_t channel);
    void begin();            ///< Calibrate sensor
    float* read(bool print=false);  ///< Read LPG, CO, Smoke
    float readLPG();
    float readCO();
    float readSmoke();
    void handleRecalibration();

private:
    adc1_channel_t _channel; ///< ADC1 channel (ESP32)
    float Ro = 10.0f;        ///< Sensor baseline resistance
    unsigned long lastReadTime = 0;

    float lpg=0, co=0, smoke=0;

    // Calibration & reading
    float MQRead();
    float MQResistanceCalculation(int raw_adc);
    float MQCalibration();

    // Gas conversion
    float MQGetGasPercentage(float rs_ro_ratio, int gas_id);
    int MQGetPercentage(float rs_ro_ratio, float *pcurve);

    // Gas IDs
    static constexpr int GAS_LPG   = 0;
    static constexpr int GAS_CO    = 1;
    static constexpr int GAS_SMOKE = 2;

    // Constants for MQ2 curves (from datasheet)
    static constexpr float LPGCurve[3]   = {2.3, 0.21, -0.47};
    static constexpr float COCurve[3]    = {2.3, 0.72, -0.34};
    static constexpr float SmokeCurve[3] = {2.3, 0.53, -0.44};

    static constexpr int RL_VALUE = 5;  ///< Load resistance in kOhm
    static constexpr int CALIBRATION_SAMPLE_TIMES = 50;
    static constexpr int CALIBRATION_SAMPLE_INTERVAL = 500; // ms
    static constexpr int READ_SAMPLE_TIMES = 5;
    static constexpr int READ_SAMPLE_INTERVAL = 50; // ms

    bool needsRecalibration1min = false;
    bool needsRecalibration5min = false;
	bool save_Ro_to_NVS(float Ro);
};

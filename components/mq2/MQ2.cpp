#include "MQ2.h"
#include "esp_timer.h"

#define NVS_NAMESPACE "mq2"
#define NVS_KEY_RO    "Ro"

MQ2::MQ2(adc1_channel_t channel) : _channel(channel) { }

void MQ2::begin() {

    // Lade Ro aus NVS
    nvs_handle_t handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle) == ESP_OK) {
        size_t sz = sizeof(Ro);
        if (nvs_get_blob(handle, NVS_KEY_RO, &Ro, &sz) == ESP_OK) {
            ESP_LOGI("MQ2", "Loaded Ro from NVS: %.2f kohm", Ro);
        }
        nvs_close(handle);
    }

    esp_timer_handle_t timer1;
    esp_timer_handle_t timer5;

    auto timer_cb = [](void* arg){
        MQ2* sensor = (MQ2*)arg;
        static int count = 0;
        if(count == 0) sensor->needsRecalibration1min = true;
        else if(count == 1) sensor->needsRecalibration5min = true;
        count++;
    };

    esp_timer_create_args_t targs1 = { .callback = timer_cb, .arg = this, .name = "mq2_recalib" };
    esp_timer_create(&targs1, &timer1);
    esp_timer_start_once(timer1, 60 * 1000 * 1000); // 1 min

    esp_timer_create_args_t targs5 = { .callback = timer_cb, .arg = this, .name = "mq2_recalib" };
    esp_timer_create(&targs5, &timer5);
    esp_timer_start_once(timer5, 5 * 60 * 1000 * 1000); // 5 min
}

float* MQ2::read(bool print) {
    lpg   = MQGetGasPercentage(MQRead()/Ro, GAS_LPG);
    co    = MQGetGasPercentage(MQRead()/Ro, GAS_CO);
    smoke = MQGetGasPercentage(MQRead()/Ro, GAS_SMOKE);

    lastReadTime = xTaskGetTickCount() * portTICK_PERIOD_MS;

    static float values[3];
    values[0] = lpg;
    values[1] = co;
    values[2] = smoke;

    if (print) {
        printf("LPG: %.1f ppm, CO: %.1f ppm, SMK: %.1f ppm\n", lpg, co, smoke);
    }

    return values;
}

float MQ2::readLPG() {
    if ((xTaskGetTickCount()*portTICK_PERIOD_MS - lastReadTime) < 10000 && lpg != 0)
        return lpg;
    return lpg = MQGetGasPercentage(MQRead()/Ro, GAS_LPG);
}

float MQ2::readCO() {
    if ((xTaskGetTickCount()*portTICK_PERIOD_MS - lastReadTime) < 10000 && co != 0)
        return co;
    return co = MQGetGasPercentage(MQRead()/Ro, GAS_CO);
}

float MQ2::readSmoke() {
    if ((xTaskGetTickCount()*portTICK_PERIOD_MS - lastReadTime) < 10000 && smoke != 0)
        return smoke;
    return smoke = MQGetGasPercentage(MQRead()/Ro, GAS_SMOKE);
}

float MQ2::MQRead() {
    int rs_sum = 0;
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(_channel, ADC_ATTEN_DB_11);

    for (int i=0; i<READ_SAMPLE_TIMES; i++) {
        int raw = adc1_get_raw(_channel); // 0–4095
        rs_sum += MQResistanceCalculation(raw);
        vTaskDelay(pdMS_TO_TICKS(READ_SAMPLE_INTERVAL));
    }
    return rs_sum / READ_SAMPLE_TIMES;
}

float MQ2::MQResistanceCalculation(int raw_adc) {
    // map 12-bit ADC (0–4095) to voltage (0–3.3V) and calculate resistance
    float voltage = raw_adc * 3.3f / 4095.0f;
    return RL_VALUE * (3.3f - voltage) / voltage; // Rs in kOhm
}

float MQ2::MQCalibration() {
    float val = 0;
    for (int i=0; i<CALIBRATION_SAMPLE_TIMES; i++) {
        int raw = adc1_get_raw(_channel);
        val += MQResistanceCalculation(raw);
        vTaskDelay(pdMS_TO_TICKS(CALIBRATION_SAMPLE_INTERVAL));
    }
    val /= CALIBRATION_SAMPLE_TIMES;
    return val / 9.83f; // RO_CLEAN_AIR_FACTOR
}

float MQ2::MQGetGasPercentage(float rs_ro_ratio, int gas_id) {
    if (gas_id == GAS_LPG)   return MQGetPercentage(rs_ro_ratio, (float*)LPGCurve);
    if (gas_id == GAS_CO)    return MQGetPercentage(rs_ro_ratio, (float*)COCurve);
    if (gas_id == GAS_SMOKE) return MQGetPercentage(rs_ro_ratio, (float*)SmokeCurve);
    return 0;
}

int MQ2::MQGetPercentage(float rs_ro_ratio, float *pcurve) {
    return (int)(pow(10, ((log10(rs_ro_ratio) - pcurve[1])/pcurve[2] + pcurve[0])));
}

bool MQ2::save_Ro_to_NVS(float Ro) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return false;
    err = nvs_set_blob(handle, NVS_KEY_RO, &Ro, sizeof(Ro));
    if (err != ESP_OK) { nvs_close(handle); return false; }
    nvs_commit(handle);
    nvs_close(handle);
    return true;
}

void MQ2::handleRecalibration() {
        if(needsRecalibration1min || needsRecalibration5min) {
        ESP_LOGW("MQ2", "Starting recalibration...");

        Ro = MQCalibration();
        save_Ro_to_NVS(Ro);

        ESP_LOGW("MQ2", "Recalibration done. Ro = %.2f kohm", Ro);

        needsRecalibration1min = false;
        needsRecalibration5min = false;
    }
}

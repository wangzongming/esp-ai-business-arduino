#ifndef AUTO_UPDATE_H
#define AUTO_UPDATE_H

#include <esp-ai.h>
#include "Face.h"
#include <Arduino_JSON.h>
#include "./logging.h"
#include "./ota_manager.h"

#include "./audios/zh/jian_ce_shen_ji.h"
#include "./audios/zh/shen_ji_shi_bai.h"
#include "./audios/zh/zheng_zai_sheng_ji.h"

// 无 face 参数的版本
void auto_update(String device_id, String api_key, String bin_id, String is_official, String domain, String version,
                 ESP_AI &esp_ai, ESPOTAManager &otaManager);
void auto_update(String device_id, String api_key, String bin_id, String is_official, String domain, String version, ESP_AI &esp_ai, ESPOTAManager &otaManager, Face &face);

#endif
#ifndef AUTO_UPDATE_H
#define AUTO_UPDATE_H
  
#include <esp-ai.h>  
#include <Arduino_JSON.h> 
#include "./logging.h"
#include "./ota_manager.h"

#include "./audios/zh/jian_ce_shen_ji.h"
#include "./audios/zh/shen_ji_shi_bai.h"
#include "./audios/zh/zheng_zai_sheng_ji.h"

void auto_update(String device_id, String api_key, String bin_id, String is_official, String domain, String version, ESP_AI &esp_ai, ESPOTAManager &otaManager,
String *bttomText,
                 int *bttomTextPrevLeft,
                 String *topLeftText,
                 String *prevTopLeftText
);

#endif
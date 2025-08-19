#ifndef AUTO_UPDATE_H
#define AUTO_UPDATE_H
  
#include <Arduino_JSON.h>
#include "../USER_CONFIG.h"  
#include "./logging.h"
#include "./ota_manager.h"

#if defined(ENABLE_EEUI)
class Face;
#else
#include "Face.h"
#endif

#include "./audios/zh/jian_ce_shen_ji.h"
#include "./audios/zh/shen_ji_shi_bai.h"
#include "./audios/zh/zheng_zai_sheng_ji.h"

// 无 face 参数的版本
void auto_update(
    const String &device_id,
    const String &api_key,
    const String &bin_id,
    const String &is_official,
    const String &domain,
    const String &version, 
    void (*wait_mp3_player_done)(),
    void (*playBuiltinAudio)(const unsigned char *data, size_t len),
    void (*tts)(const String &text),
    ESPOTAManager &otaManager, 
    void (*SetChatMessage)(const String &text, 
     const String &status // "start" | "error" | "end"
    )
); 
#endif
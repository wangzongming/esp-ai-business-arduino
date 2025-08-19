
#include "auto_update.h"

/**
 * 自动检测 OTA 升级函数
 */
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
    void (*setChatMessage)(const String &text, const String &status))
{

    setChatMessage("系统检查升级中。", "start");
    wait_mp3_player_done();
    playBuiltinAudio(jian_ce_shen_ji_mp3, jian_ce_shen_ji_mp3_len);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    wait_mp3_player_done();

    HTTPClient http;
    String url = domain;
    url += "sdk/query_new_ota?version=" + version;
    url += "&bin_id=" + bin_id;
    url += "&is_official=" + is_official;

    Serial.print("升级地址：");
    Serial.println(url);
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    int httpCode = http.GET();

    if (httpCode > 0)
    {
        String payload = http.getString();
        LOG_D("[HTTP] POST code: %d\n", httpCode);
        JSONVar parse_res = JSON.parse(payload);

        if (JSON.typeof(parse_res) == "undefined" || String(httpCode) != "200")
        {
            LOG_D("[Error HTTP] OTA 自动升级失败: %s", url.c_str());
            wait_mp3_player_done();
            playBuiltinAudio(shen_ji_shi_bai_mp3, shen_ji_shi_bai_mp3_len);
            wait_mp3_player_done();
        }
        else
        {
            String message = (const char *)parse_res["message"];
            if (parse_res.hasOwnProperty("success"))
            {
                bool success = (bool)parse_res["success"];
                if (success == false)
                {
                    LOG_D("[Error HTTP] OTA 自动升级失败: %s", url.c_str());
                    setChatMessage("系统检测升级失败：" + message, "error");
                    wait_mp3_player_done();
                    playBuiltinAudio(shen_ji_shi_bai_mp3, shen_ji_shi_bai_mp3_len);
                    wait_mp3_player_done();
                    tts("系统检测升级失败：" + message);
                }
                else
                {
                    bool latest = (bool)parse_res["data"]["latest"];
                    String bin_url = (const char *)parse_res["data"]["bin_url"];
                    if (latest)
                    { 
                        wait_mp3_player_done();
                        tts("系统已经是最新版本啦！"); 
                        setChatMessage("已经是最新版本", "end"); 
                    }
                    else
                    {
                        setChatMessage("检测到最新系统版本，正在进行升级。", "end");

                        wait_mp3_player_done();
                        playBuiltinAudio(zheng_zai_sheng_ji_mp3, zheng_zai_sheng_ji_mp3_len);

                        vTaskDelay(500 / portTICK_PERIOD_MS);
                        wait_mp3_player_done();
                        vTaskDelay(2000 / portTICK_PERIOD_MS);

                        otaManager.init(device_id);
                        otaManager.update(bin_url);
                    }
                }
            }
            else
            {
                setChatMessage("系统检测升级失败：002", "error");
                wait_mp3_player_done();
                playBuiltinAudio(shen_ji_shi_bai_mp3, shen_ji_shi_bai_mp3_len);
                wait_mp3_player_done();
                LOG_D("[Error HTTP] OTA 自动升级失败: %s", message.c_str());
            }
        }
    }
    else
    {
        setChatMessage("系统检测升级失败：001", "error");
        LOG_D("[Error HTTP] 系统检测升级失败: %s", url.c_str());
    }
    http.end();
}

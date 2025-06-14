
#include "auto_update.h"

/**
 * 自动检测 OTA 升级函数
 */
void auto_update(String device_id, String api_key, String bin_id, String is_official, String domain, String version, ESP_AI &esp_ai,
                 ESPOTAManager &otaManager,
                 String *bttomText,
                 int *bttomTextPrevLeft,
                 String *topLeftText,
                 String *prevTopLeftText)
{
    // face.SetChatMessage("系统检查升级中。");
    *bttomText = "系统检查升级中。";
    bttomTextPrevLeft = 0;
    wait_mp3_player_done();
    play_builtin_audio(jian_ce_shen_ji_mp3, jian_ce_shen_ji_mp3_len);

    HTTPClient http;
    String url = domain;
    url += "sdk/query_new_ota?version=" + version;
    url += "&bin_id=" + bin_id;
    url += "&is_official=" + is_official;

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
            play_builtin_audio(shen_ji_shi_bai_mp3, shen_ji_shi_bai_mp3_len);
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
                    // face.SetChatMessage("系统检测升级失败：" + message);
                    *bttomText = "系统检测升级失败：" + message;
                    bttomTextPrevLeft = 0;
                    wait_mp3_player_done();
                    play_builtin_audio(shen_ji_shi_bai_mp3, shen_ji_shi_bai_mp3_len);
                    wait_mp3_player_done();
                    esp_ai.tts("系统检测升级失败：" + message);
                }
                else
                {
                    bool latest = (bool)parse_res["data"]["latest"];
                    String bin_url = (const char *)parse_res["data"]["bin_url"];
                    if (latest)
                    {
                        wait_mp3_player_done();
                        // face.SetChatMessage("已经是最新版本");
                        *bttomText = "已经是最新版本";
                        bttomTextPrevLeft = 0;
                        vTaskDelay(1000 / portTICK_PERIOD_MS);
                        esp_ai.tts("系统已经是最新版本啦！");
                        vTaskDelay(1000 / portTICK_PERIOD_MS);
                        wait_mp3_player_done();
                    }
                    else
                    {
                        // face.SetChatMessage("检测到最新系统版本，正在进行升级。");
                        *bttomText = "检测到最新系统版本，正在进行升级。";
                        bttomTextPrevLeft = 0;
                        wait_mp3_player_done();
                        play_builtin_audio(zheng_zai_sheng_ji_mp3, zheng_zai_sheng_ji_mp3_len);
                        // test...
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

                // face.SetChatMessage("系统检测升级失败：002");
                *bttomText = "系统检测升级失败：002";
                bttomTextPrevLeft = 0;
                wait_mp3_player_done();
                play_builtin_audio(shen_ji_shi_bai_mp3, shen_ji_shi_bai_mp3_len);
                wait_mp3_player_done();
                LOG_D("[Error HTTP] OTA 自动升级失败: %s", message.c_str());
            }
        }
    }
    else
    {
        // face.SetChatMessage("系统检测升级失败：001");
        *bttomText = "系统检测升级失败：001";
        bttomTextPrevLeft = 0;
        LOG_D("[Error HTTP] 系统检测升级失败: %s", url.c_str());
    }
    http.end();
}

#include <esp-ai.h>
#include <Wire.h>
#include <Battery.h>
#include "main.h"
#include "SimpleTimer.h"
#include <OneButton.h>
#include <Arduino.h>

// ==================版本定义=========================
String _version = "1.30.40";

// ==================OTA 升级定义=========================
// 是否为官方固件， 如果是您自己的固件请改为 "0"
String is_official = "1";

// 固件ID， 请正确填写，否则无法进行 OTA 升级。
// 获取方式： 我的固件 -> 固件ID
String BIN_ID = "0";

// ====================================================

// ==================全局对象===========================
// 屏幕显示对象
Face *face;

// 定时器对象
SimpleTimer timer;
#if defined(IS_XIAO_ZHI_S3_2) || defined(IS_XIAO_ZHI_S3_3) || defined(IS_WU_MING_TFT) || defined(IS_AI_VOX_TFT) || defined(IS_ESP_AI_S3_DOUBLE_OLED)
// 按钮对象
OneButton button_vol_add(VOL_ADD_KEY, true);
OneButton button_vol_sub(VOL_SUB_KEY, true);
#endif
/*** 测试地址 ***/
// 业务服务地址，用于将设备信息传递到用户页面上
// String domain = "http://192.168.3.16:7002/";
// // 业务服务 ws 服务端口ip
// String yw_ws_ip = "192.168.3.16";
// String yw_ws_path = "";
// int yw_ws_port = 7002;
// // ESP-AI 服务配置： { 服务IP， 服务端口, "[可选] 请求参数" }
// ESP_AI_server_config server_config = {"http", "192.168.3.16", 8088};

/*** 线上地址 ***/
String domain = "http://api.espai2.fun/";
String yw_ws_ip = "api.espai2.fun";
String yw_ws_path = "";
int yw_ws_port = 80;
// 这里不配置接口，配网页面也没有 api_key 配置，这会导致使用默认的服务节点：node.espai.fun
ESP_AI_server_config server_config = {"http", "node.espai2.fun", 80};

ESP_AI esp_ai;
WebSocketsClient webSocket_yw;
bool is_yw_connected = false; // 业务服务是否已经连接
// 设备id
String device_id = "";
// 网络状态
String _net_status = "";
// 会话状态
String _session_status = "";

// 设备局域网IP
String _device_ip = "";
// 设备地址
String on_position_nation = "";
String on_position_province = "";
String on_position_city = "";

// 是否启用电量检测
String kwh_enable = "";
// 实例化电源管理对象
Battery battery(3300, 4200, BAT_PIN, 12); // 电池电压范围 3.3v~4.2v
// 上报system的基本信息至服务器的时间间隔
long report_systeminfo_interval = 60 * 1000; // 1分钟上报一次电量

// 情绪灯光灯珠数量
int emotion_led_number = 150;
// 情绪灯光
Adafruit_NeoPixel pixelsEmotion(emotion_led_number, EMOTION_LIGHT_PIN, NEO_GRB + NEO_KHZ800);

// OTA 检测升级过了，用于应付重新连接服务后不要重复去进行提示 OTA 升级
bool ota_ed = false;

// ======================END=======================

// 上报设备电量的函数
void upload_systeminfo()
{
    if (kwh_enable == "0" || _session_status == "说话中")
    {
        return;
    }

// ESP-AI 开发版需要做电量检测
#if defined(IS_ESP_AI_S3_OLED) || defined(IS_ESP_AI_S3_DOUBLE_OLED) || defined(IS_ESP_AI_S3_TFT) || defined(IS_ESP_AI_S3_NO_SCREEN)
    String ext4 = esp_ai.getLocalData("ext4");
    String ext5 = esp_ai.getLocalData("ext5");
    if (ext4 != "" && ext5 != "")
    {
        return;
    }
    int batteryLevel = battery.level(); // 获取电池电量百分比
    LOG_D("上报电量：%d", batteryLevel);

#if !defined(IS_ESP_AI_S3_NO_SCREEN)
    if (battery.getischarge())
    {
        // 说明在充电
        LOG_D("设备充电中...");
        face->SetBatLevel(0xFF);
    }
    else
    {
        face->SetBatLevel(batteryLevel);
    }
#endif

    if (batteryLevel < 10)
    {
        esp_ai.tts("设备电量不足10%啦，请帮我充电哦。");
    }
    else if (batteryLevel < 5)
    {
        esp_ai.tts("设备电量不足5%啦，马上变成一块砖啦！");
    }

    uint32_t free_heap = ESP.getFreeHeap() / 1024;
    uint32_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024;
    // LOG_D("===> 可用内存: %d KB 可用PSRAM: %d KB", free_heap, free_psram);

    JSONVar data_battery;
    data_battery["type"] = "battery";
    data_battery["data"] = batteryLevel;
    data_battery["freememory"] = free_heap;
    data_battery["freepsram"] = free_psram;
    data_battery["device_id"] = device_id;
    String sendData = JSON.stringify(data_battery);
    webSocket_yw.sendTXT(sendData);
#endif
}

float volume = 1;
void up_click(float number)
{
    volume += number;
    if (volume > 1)
    {
        volume = 1;
    }
    esp_ai.setVolume(volume);
    LOG_D("音量加: %d", volume);
#if !defined(IS_ESP_AI_S3_NO_SCREEN)
    face->SetChatMessage("音量：" + String(int(volume * 100)) + "%");
    face->SetVolume(int(volume * 100));
#endif

    // 存储到本地
    esp_ai.setLocalData("ext2", String(volume, 1));

    // 上报服务器
    JSONVar json_data;
    json_data["type"] = "volume";
    json_data["data"] = volume;
    json_data["device_id"] = device_id;
    String sendData = JSON.stringify(json_data);
    webSocket_yw.sendTXT(sendData);
}
void down_click(float number)
{
    volume -= number;
    if (volume < 0)
    {
        volume = 0;
    }
    esp_ai.setVolume(volume);
    LOG_D("音量减: %d", volume);
#if !defined(IS_ESP_AI_S3_NO_SCREEN)
    face->SetChatMessage("音量：" + String(int(volume * 100)) + "%");
    face->SetVolume(int(volume * 100));
#endif

    // 存储到本地
    esp_ai.setLocalData("ext2", String(volume, 1));

    JSONVar json_data;
    json_data["type"] = "volume";
    json_data["data"] = volume;
    json_data["device_id"] = device_id;
    String sendData = JSON.stringify(json_data);
    webSocket_yw.sendTXT(sendData);
}

// ========================= OTA升级逻辑 =========================
ESPOTAManager *otaManager;

// ========================= 错误监听 =========================
void onError(String code, String at_pos, String message)
{
    // some code...
    LOG_D("监听到错误：%s\n", code.c_str());
    String loc_ext7 = esp_ai.getLocalData("ext7");
    if (code == "006" && loc_ext7 != "")
    {
        // 清除板子信息，让板子重启，否则板子会不断重开热点无法操作
        esp_ai.setLocalData("wifi_name", "");
        esp_ai.setLocalData("wifi_pwd", "");
        esp_ai.setLocalData("ext1", "");
        ESP.restart();
    }
    if (message.indexOf("请充值") >= 0)
    {
#if !defined(IS_ESP_AI_S3_NO_SCREEN)
        face->SetChatMessage("额度卡不足，请充值。");
#endif
        play_builtin_audio(yu_e_bu_zu_mp3, yu_e_bu_zu_mp3_len);
        delay(5000);
    }

    //  if (code == "002")
    // {
    //     // 清除 api_key 重启板子
    //     esp_ai.setLocalData("wifi_name", "");
    //     esp_ai.setLocalData("wifi_pwd", "");
    //     esp_ai.setLocalData("ext1", "");
    //     ESP.restart();
    // }
}

// ========================= 指令监听 =========================
void on_command(String command_id, String data)
{
    // LOG_D("\n[]收到指令: %s \n", command_id);
    // LOG_D(data);
    // 控制小灯演示
    // if (command_id == "device_open_001") {
    //   LOG_D("开灯");
    //   // digitalWrite(led_pin, HIGH);
    // }
    // if (command_id == "device_close_001") {
    //   LOG_D("关灯");
    //   // digitalWrite(led_pin, LOW);
    // }
    if (command_id == "add_volume")
    {
        LOG_D("加音量");
        up_click(0.1);
    }
    else if (command_id == "subtract_volume")
    {
        LOG_D("减音量");
        down_click(0.1);
    }
    else if (command_id == "query_battery")
    {
        int batteryLevel = battery.level(); // 获取电池电量百分比
        LOG_D("查询电量");
        LOG_D("设备电量剩余: %d%", batteryLevel);
        esp_ai.stopSession();
        esp_ai.tts("设备电量剩余" + String(batteryLevel) + "%");
    }
    else if (command_id == "on_iat_cb")
    {
#if !defined(IS_ESP_AI_S3_NO_SCREEN)
        face->SetChatMessage(data);
#endif
    }
    else if (command_id == "on_llm_cb")
    {
#if !defined(IS_ESP_AI_S3_NO_SCREEN)
        face->SetChatMessage(data);
#endif
    }
}

// ========================= 会话状态监听 =========================
void on_session_status(String status)
{
    _session_status = status;
    if (otaManager->isUpdating())
    {
        return;
    }

#if !defined(IS_ESP_AI_S3_NO_SCREEN)
    // 处理会话状态
    if (status == "iat_start")
    {
        face->SetEmotion("聆听中");
        face->ShowNotification("聆听中");
    }
    else if (status == "iat_end")
    {
        face->SetEmotion("说话中");
        face->ShowNotification("说话中");
    }
    else if (status == "tts_real_end")
    {
        face->SetEmotion("默认");
        face->ShowNotification("待命中");
    }
    else if (status == "tts_chunk_start")
    {
        // face->DrawStrLeftTop("AI说话中...", 0);
    }
    else
    {
        // face->DrawStrLeftTop("待命中", 2);
    }
#endif
}

// ========================= 网络状态监听 =========================
void on_net_status(String status)
{
    // LOG_D("网络状态: " + status);
    _net_status = status;

    if (status == "0_ing")
    {
#if !defined(IS_ESP_AI_S3_NO_SCREEN)
        face->SetChatMessage("正在连接网络");
#endif
    };
    if (status == "0_ap")
    {
#if !defined(IS_ESP_AI_S3_NO_SCREEN)
        face->ShowNotification("配网模式");
#if defined(BLE_MODEL)
        face->SetChatMessage("请打开小程序配网");
        face->SetChatMessage("请打开小程序配网");
        face->SetChatMessage("请打开小程序配网");
        face->SetChatMessage("请打开小程序配网");
        face->SetChatMessage("请打开小程序配网");
        face->SetChatMessage("请打开小程序配网");
#else
        face->SetChatMessage("连接" + WiFi.softAPSSID() + "热点配网");
#endif
#endif
    };

    if (status == "2")
    {
#if !defined(IS_ESP_AI_S3_NO_SCREEN)
        face->SetChatMessage("正在连接服务");
#endif
    };

    // 已连接服务
    if (status == "3")
    {
#if !defined(IS_ESP_AI_S3_NO_SCREEN)
        face->SetChatMessage("服务连接成功");
#endif
    };
}

/**
 * 电量检测
 */
int batteryDetection()
{
// ESP-AI 开发版需要做电量检测
#if defined(IS_ESP_AI_S3_OLED) || defined(IS_ESP_AI_S3_DOUBLE_OLED) || defined(IS_ESP_AI_S3_TFT) || defined(IS_ESP_AI_S3_NO_SCREEN)
    int batteryLevel = battery.level(); // 获取电池电量百分比
    LOG_D("[Info] -> 电池电量：%d", batteryLevel);
    if (batteryLevel < 10)
    {
        while (true)
        {
            batteryLow();
        }

        return false;
    }
    return batteryLevel;
#else
    return 100;
#endif
}
void battery_detection_timer(int arg)
{
    batteryDetection();
}

/**
 * 电量不足提示
 */
void batteryLow()
{
    if (!battery.getischarge())
    { 
        LOG_D("%s", "电量不足");
        LOG_D("%s", "如果您的开发板没有电压检测模块，请在配网页面或者设备配置页面禁用电量检测功能。");
        play_builtin_audio(mei_dian_le, mei_dian_le_len);
#if !defined(IS_ESP_AI_S3_NO_SCREEN)
        face->SetChatMessage("电量过低，请连接充电器。");
#endif

        pixelsEmotion.fill(pixelsEmotion.Color(255, 69, 0), 0, emotion_led_number);
        pixelsEmotion.show();
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}

bool onBegin()
{
    if (kwh_enable == "0")
    {
        return true;
    }

#if defined(IS_XIAO_ZHI_S3_2) || defined(IS_XIAO_ZHI_S3_3) || defined(IS_WU_MING_TFT) || defined(IS_AI_VOX_TFT) || defined(IS_MA_ZHUANG_TFT)
    return true;
#endif

    batteryDetection();
    return true;
}

void on_ready()
{
    String ext3 = esp_ai.getLocalData("ext3");
    if (ext3 == "")
    {
        esp_ai.setLocalData("ext3", "1");
        esp_ai.awaitPlayerDone();
        esp_ai.stopSession();
        esp_ai.tts("设备激活成功，现在可以和我聊天了哦。");
        esp_ai.awaitPlayerDone();
    }

    // 检测升级
    if (!ota_ed)
    {
        ota_ed = true;
        String loc_api_key = esp_ai.getLocalData("ext1");
#if defined(IS_ESP_AI_S3_NO_SCREEN)
        auto_update(device_id, loc_api_key, BIN_ID, is_official, domain, _version, esp_ai, *otaManager);
#else
        auto_update(device_id, loc_api_key, BIN_ID, is_official, domain, _version, esp_ai, *otaManager, *face);
#endif
    }
}

// ========================= 业务服务的 ws 连接 =========================
void webSocketEvent_ye(WStype_t type, uint8_t *payload, size_t length)
{
    switch (type)
    {
    case WStype_DISCONNECTED:
        LOG_D("业务ws服务已断开");
        is_yw_connected = false;

        // 玳瑁黄
        esp_ai_pixels.setPixelColor(0, esp_ai_pixels.Color(218, 164, 90));

        if (esp_ai_pixels.getBrightness() != 100)
            esp_ai_pixels.setBrightness(10); // 亮度设置
        else
            esp_ai_pixels.setBrightness(10); // 亮度设置
        esp_ai_pixels.show();
        break;
    case WStype_CONNECTED:
    {
        LOG_D("√ 业务ws服务已连接");
        is_yw_connected = true;
        // 上报设备信息
        upload_systeminfo();
        // 指示灯状态应该由设备状态进行控制
        on_net_status(_net_status);
        break;
    }
    case WStype_TEXT:
    {
        JSONVar parseRes = JSON.parse((char *)payload);
        if (JSON.typeof(parseRes) == "undefined")
        {
            return;
        }
        if (parseRes.hasOwnProperty("type"))
        {
            String type = (const char *)parseRes["type"];
            if (type == "ota_update")
            {
                String url = (const char *)parseRes["url"];
                LOG_D("收到ota升级指令");
                // 初始化 OTA
                otaManager->init(device_id);
                otaManager->update(url);
            }
            else if (type == "stop_session")
            {
                LOG_D("业务服务要求停止会话");
                esp_ai.stopSession();
            }
            else if (type == "set_volume")
            {
                double volume = (double)parseRes["volume"];
                LOG_D("接收到音量：%d", volume);
                esp_ai.setVolume(volume);
                // 存储到本地
                esp_ai.setLocalData("ext2", String(volume, 1));
            }
            else if (type == "message")
            {
                String message = (const char *)parseRes["message"];
                LOG_D("收到服务提示：%d", message);
            }
            else if (type == "get_local_data")
            {
                LOG_D("收到 get_local_data 指令!");
                String loc_ext1 = esp_ai.getLocalData("ext1");

                // 上报服务器
                JSONVar local_data = esp_ai.getLocalAllData();

                JSONVar json_data;
                json_data["type"] = "get_local_data_res";
                json_data["data"] = local_data;
                json_data["device_id"] = device_id;
                json_data["api_key"] = loc_ext1;
                String send_data = JSON.stringify(json_data);
                webSocket_yw.sendTXT(send_data);
            }
            else if (type == "set_local_data")
            {
                JSONVar data = parseRes["data"];
                JSONVar keys = data.keys();
                LOG_D("收到 set_local_data 指令：");
                Serial.print(data);
                for (int i = 0; i < keys.length(); i++)
                {
                    String key = keys[i];
                    JSONVar value = data[key];
                    esp_ai.setLocalData(key, String((const char *)value));
                }
                vTaskDelay(pdMS_TO_TICKS(200));
                LOG_D("%s", "即将重启设备更新设置............");
                ESP.restart();
            }
            // else if (type == "re_cache") {
            //   // delay(5000);
            //   esp_ai.reCache();
            // }
        }
        break;
    }
    case WStype_ERROR:
        LOG_D("业务服务 WebSocket 连接错误");
        break;
    }
}

void webScoket_yw_main()
{
    String device_id = esp_ai.getLocalData("device_id");
    String ext1 = esp_ai.getLocalData("ext1");
    String ext2 = esp_ai.getLocalData("ext2");
    String ext3 = esp_ai.getLocalData("ext3");
    String ext4 = esp_ai.getLocalData("ext4");
    String ext5 = esp_ai.getLocalData("ext5");

    if (ext4 != "" && ext5 != "")
    {
        return;
    }

    LOG_D("开始连接业务 ws 服务");
    if (_net_status == "0_ap")
    {
        LOG_D("设备还未配网，忽略业务服务连接。");
        return;
    }
    // ws 服务
    webSocket_yw.begin(
        yw_ws_ip,
        yw_ws_port,
        yw_ws_path + "/connect_espai_node?device_type=hardware&device_id=" + device_id + "&version=" + _version + "&api_key=" + ext1 + "&ext2=" + ext2 + "&ext3=" + ext3 + "&ext4=" + ext4 + "&ext5=" + ext5);

    webSocket_yw.onEvent(webSocketEvent_ye);
    webSocket_yw.setReconnectInterval(3000);
    webSocket_yw.enableHeartbeat(10000, 5000, 0);
}

// ========================= 绑定设备 =========================
String encodeURIComponent(String input)
{
    String encoded = "";
    char hexBuffer[4]; // 用于存储百分比编码后的值

    for (int i = 0; i < input.length(); i++)
    {
        char c = input[i];

        // 检查是否为不需要编码的字符（字母、数字、-、_、.、~）
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
        {
            encoded += c; // 不需要编码的字符直接添加
        }
        else
        {
            // 对需要编码的字符进行百分比编码
            snprintf(hexBuffer, sizeof(hexBuffer), "%%%02X", c);
            encoded += hexBuffer;
        }
    }

    return encoded;
}

HTTPClient on_bind_device_http;
String on_bind_device(JSONVar data)
{
    LOG_D("\n[Info] -> 绑定设备中");
    String device_id = data["device_id"];
    String wifi_name = data["wifi_name"];
    String wifi_pwd = data["wifi_pwd"];
    String ext1 = data["ext1"];
    String ext4 = data["ext4"];
    String ext5 = data["ext5"];

    esp_ai_pixels.setBrightness(10); // 亮度设置
    esp_ai_pixels.show();

    // ext4 存在时是请求的自己的服务器，所以不去请求绑定服务
    // if (ext4.length() > 0 && ext5.length() > 0) {
    if (ext4 != "" && ext5 != "")
    {
#if !defined(IS_ESP_AI_S3_NO_SCREEN)
        face->SetChatMessage("设备激活成功");
#endif
        return "{\"success\":true,\"message\":\"设备激活成功。\"}";
    }

    String loc_api_key = esp_ai.getLocalData("ext1");
    String loc_ext7 = esp_ai.getLocalData("ext7");

    // 标识为临时数据
    String _ble_temp_ = esp_ai.getLocalData("_ble_temp_");

    // 如果 api_key 没有变化，那不应该去请求服务器绑定
    if (loc_api_key == ext1 && _ble_temp_ != "1")
    {
        LOG_D("\n[Info] -> 仅仅改变 wifi 等配置信息");
        return "{\"success\":true,\"message\":\"设备激活成功。\"}";
    }

    // 创建一个HTTP客户端对象
    LOG_D("[Info] ext4: %s", ext4.c_str());
    LOG_D("[Info] ext5: %s", ext5.c_str());
    LOG_D("[Info] api_key: %s", ext1.c_str());
    LOG_D("[Info] device_id: %s", device_id.c_str());
    // 如果已经连接了wifi了的话，这个应该为修改  --这个情况废弃
    String url = domain + "devices/add";
    on_bind_device_http.begin(url);
    on_bind_device_http.addHeader("Content-Type", "application/json");

    JSONVar json_params;
    json_params["version"] = _version;
    json_params["bin_id"] = BIN_ID;
    json_params["device_id"] = device_id;
    json_params["api_key"] = ext1;
    json_params["wifi_ssid"] = wifi_name;
    json_params["wifi_pwd"] = wifi_pwd;
    String send_data = JSON.stringify(json_params);

    // 发送POST请求并获取响应代码
    int httpCode = on_bind_device_http.POST(send_data);

    esp_ai_pixels.setBrightness(100); // 亮度设置
    esp_ai_pixels.show();

    if (httpCode > 0)
    {
        String payload = on_bind_device_http.getString();
        LOG_D("[HTTP] POST code: %d\n", httpCode);
        // LOG_D("[HTTP] POST res: %d\n", payload);
        JSONVar parse_res = JSON.parse(payload);

        if (JSON.typeof(parse_res) == "undefined" || String(httpCode) != "200")
        {
            on_bind_device_http.end();
            LOG_D("[Error HTTP] 请求网址: %s", url.c_str());

#if !defined(IS_ESP_AI_S3_NO_SCREEN)
            face->SetChatMessage("设备激活失败，请重试。");
#endif

            play_builtin_audio(bind_err_mp3, bind_err_mp3_len);
            vTaskDelay(500 / portTICK_PERIOD_MS);
            esp_ai.awaitPlayerDone();
            vTaskDelay(1000 / portTICK_PERIOD_MS);

            // 这个 json 数据中的 message 会在配网页面弹出
            return "{\"success\":false,\"message\":\"设备绑定失败，错误码:" + String(httpCode) + "，重启设备试试呢。\"}";
        }

        if (parse_res.hasOwnProperty("success"))
        {
            bool success = (bool)parse_res["success"];
            String message = (const char *)parse_res["message"];
            String code = (const char *)parse_res["code"];
            if (success == false)
            {
#if !defined(IS_ESP_AI_S3_NO_SCREEN)
                face->SetChatMessage("绑定设备失败：" + message);
#endif
                // 绑定设备失败
                LOG_D("[Error] -> 绑定设备失败，错误信息：%s", message.c_str());

                // esp_ai.tts("绑定设备失败，重启设备试试呢，本次错误原因：" + message);
                on_bind_device_http.end();

                if (message.indexOf("不可以重复绑定") != -1)
                {
                    play_builtin_audio(chong_fu_bang_ding_mp3, chong_fu_bang_ding_mp3_len);
                }
                else
                {
                    play_builtin_audio(bind_err_mp3, bind_err_mp3_len);
                }
                vTaskDelay(100 / portTICK_PERIOD_MS);
                esp_ai.awaitPlayerDone();

                // 这个 json 数据中的 message 会在配网页面弹出
                return "{\"success\":false,\"message\":\"绑定设备失败，错误原因：" + message + "\"}";
            }
            else
            {
#if !defined(IS_ESP_AI_S3_NO_SCREEN)
                face->SetChatMessage("设备激活成功！");
#endif
                // 设备激活成功！
                LOG_D("[Info] -> 设备激活成功！");
                on_bind_device_http.end();
                // 这个 json 数据中的 message 会在配网页面弹出
                return "{\"success\":true,\"message\":\"设备激活成功，即将重启设备。\"}";
            }
        }
        else
        {
#if !defined(IS_ESP_AI_S3_NO_SCREEN)
            face->SetChatMessage("设备激活失败，请求服务失败！");
#endif
            LOG_D("[Error HTTP] 请求网址: %s", url.c_str());
            on_bind_device_http.end();
            return "{\"success\":false,\"message\":\"设备绑定失败，错误码:" + String(httpCode) + "，重启设备试试呢。\"}";
        }
    }
    else
    {
        LOG_D("[Error HTTP] 绑定设备失败: %d", httpCode);
        LOG_D("[Error HTTP] 请求网址: %s", url.c_str());
        on_bind_device_http.end();
        return "{\"success\":false,\"message\":\"请求服务失败，请刷新页面重试。\"}";
    }
}

void on_position(String ip, String nation, String province, String city, String latitude, String longitude)
{

    while (is_yw_connected == false)
    {
        delay(300);
    }

    String device_id = esp_ai.getLocalData("device_id");
    String loc_ext1 = esp_ai.getLocalData("ext1");
    String loc_wifi_name = esp_ai.getLocalData("wifi_name");
    String loc_wifi_pwd = esp_ai.getLocalData("wifi_pwd");

    JSONVar json_params;
    json_params["api_key"] = loc_ext1;
    json_params["version"] = _version;
    json_params["bin_id"] = BIN_ID;
    json_params["device_id"] = device_id;
    json_params["wifi_ssid"] = loc_wifi_name;
    json_params["wifi_pwd"] = loc_wifi_pwd;
    // 本地ip
    json_params["net_ip"] = _device_ip;
    // 设备地址
    if (nation)
    {
        json_params["nation"] = nation;
    }
    if (province)
    {
        json_params["province"] = province;
    }
    if (city)
    {
        json_params["city"] = city;
    }
    if (latitude)
    {
        json_params["latitude"] = latitude;
    }
    if (longitude)
    {
        json_params["longitude"] = longitude;
    }

    JSONVar sendJson;
    sendJson["api_key"] = loc_ext1;
    sendJson["device_id"] = device_id;
    sendJson["type"] = "upload_device_info";
    sendJson["data"] = json_params;

    String send_data = JSON.stringify(sendJson);
    webSocket_yw.sendTXT(send_data);
}

void on_connected_wifi(String device_ip)
{
    _device_ip = device_ip;
}

#if !defined(IS_ESP_AI_S3_NO_SCREEN)
void faceTask(void *arg)
{
    while (true)
    {
        // OTA 升级时更新慢一点
        if (otaManager->isUpdating())
        {
            face->Update();
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
        else
        {
            face->Update();
#if defined(IS_ESP_AI_S3_TFT)
            vTaskDelay(30 / portTICK_PERIOD_MS); // 多增加点时间
#endif
            vTaskDelay(20 / portTICK_PERIOD_MS);
        }
    }
    vTaskDelete(NULL);
}
#endif

long prve_emotion_time = 0;
static String prve_emotion = "无情绪";
void emotion_led_control(String &emotion)
{
    // 情绪灯光
#if defined(IS_ESP_AI_S3_OLED) || defined(IS_ESP_AI_S3_DOUBLE_OLED) || defined(IS_ESP_AI_S3_TFT) || defined(IS_ESP_AI_S3_NO_SCREEN)
    if (prve_emotion != emotion)
    {
        pixelsEmotion.clear();
        if (emotion == "快乐")
        {
            pixelsEmotion.fill(pixelsEmotion.Color(255, 255, 255), 0, emotion_led_number);
        }
        else if (emotion == "伤心")
        {
            pixelsEmotion.fill(pixelsEmotion.Color(255, 255, 0), 0, emotion_led_number);
        }
        else
        {
            // 全部作为无情绪处理：默认米黄色
            pixelsEmotion.fill(pixelsEmotion.Color(250, 138, 0), 0, emotion_led_number);
        }
        pixelsEmotion.show();
        prve_emotion = emotion;
    }
#endif
}

// 情绪检测
void onEmotion(String emotion)
{

    // Serial.print("情绪下发当前情绪：");
    // Serial.println(emotion);

#if !defined(IS_ESP_AI_S3_NO_SCREEN)
    face->SetEmotion(emotion);
#endif

    emotion_led_control(emotion);
}

void report_systeminfo_timer1(int arg)
{
    upload_systeminfo();
}

void websocket_timer2(int arg)
{
    webSocket_yw.loop();
}

/**
 * OTA 升级失败检测，需要按需启动这个定时器 ing...
 */
void update_check_timer3(int arg)
{
    if (otaManager->updateFailed())
    {
        // 10 没有反应就说明升级失败了
        LOG_D("OTA 升级失败");
        // start_update_ed = false;
        String device_id = esp_ai.getLocalData("device_id");
        // 将错误发往业务服务
        JSONVar ota_update_error;
        ota_update_error["type"] = "ota_update_error";
        ota_update_error["device_id"] = device_id;
        String sendData = JSON.stringify(ota_update_error);
        webSocket_yw.sendTXT(sendData);
    }
}

#if defined(IS_XIAO_ZHI_S3_2) || defined(IS_XIAO_ZHI_S3_3) || defined(IS_WU_MING_TFT) || defined(IS_AI_VOX_TFT) || defined(IS_ESP_AI_S3_DOUBLE_OLED)
void attach_vol_add()
{
    up_click(0.1);
}

void attach_vol_sub()
{
    down_click(0.1);
}

void key_timer4(int arg)
{
    button_vol_add.tick();
    button_vol_sub.tick();
}
#endif

void espai_loop_timer5(int arg)
{
    if (otaManager->isUpdating() == false)
    {
        esp_ai.loop();
    }
}

void setup()
{
    Serial.begin(115200);
    delay(500);

#if defined(IS_XIAO_ZHI_S3_2)
    BIN_ID = "756ed3cc63604dc0bb2dcfc24a602a25";
#elif defined(IS_XIAO_ZHI_S3_3)
    BIN_ID = "2c83b29be98d43b0a62572a379d8352b";
#elif defined(IS_ESP_AI_S3_NO_SCREEN)
    BIN_ID = "a8a840de18d4446b8b23ca28d42e2a86";
#elif defined(IS_ESP_AI_S3_TFT)
    BIN_ID = "6f608d802b4c4fa392cf337f93bda630";
#elif defined(IS_WU_MING_TFT)
    BIN_ID = "5666039aeefe4e49a177988c924165f1";
#elif defined(IS_AI_VOX_TFT)
    BIN_ID = "3b45f2bbb79b4940a925d0e4822352f1";
#elif defined(IS_ESP_AI_S3_BASIC)
    BIN_ID = "e9c8377a3e3e468990ec4d983b8eee1e";
#elif defined(IS_ESP_AI_S3_DOUBLE_OLED)
    BIN_ID = "d0392954145448008b2cfed22b2b8a23";
#elif defined(IS_MA_ZHUANG_TFT)
    BIN_ID = "xxx";
#endif

    // 配置ADC电压基准值与衰减倍数
    battery.begin(1250, 4.0);

    String oled_type = "091";
    String oled_sck = esp_ai.getLocalData("oled_sck");
    String oled_sda = esp_ai.getLocalData("oled_sda");

    if (oled_sck == "")
    {
        oled_sck = "38";
    }
    if (oled_sda == "")
    {
        oled_sda = "39";
    }

#if defined(IS_XIAO_ZHI_S3_2)
    face = new Face(4, "091", 42, 41);
#elif defined(IS_XIAO_ZHI_S3_3)
    face = new Face(4, "096", 42, 41);
#elif defined(IS_ESP_AI_S3_OLED)
    face = new Face(4, "096", oled_sck.toInt(), oled_sda.toInt());
#elif defined(IS_ESP_AI_S3_DOUBLE_OLED)
    face = new Face(6, "096_2", oled_sck.toInt(), oled_sda.toInt());
#elif defined(IS_ESP_AI_S3_TFT) || defined(IS_AI_VOX_TFT) || defined(IS_WU_MING_TFT) || defined(IS_MA_ZHUANG_TFT)
    face = new Face(8, "240*240");
#endif

#if defined(IS_ESP_AI_S3_NO_SCREEN)
    otaManager = new ESPOTAManager(&webSocket_yw, &esp_ai, nullptr);
#else
    otaManager = new ESPOTAManager(&webSocket_yw, &esp_ai, face);
#endif

#if !defined(IS_ESP_AI_S3_NO_SCREEN)
    // 更新文字
    face->SetChatMessage("ESP-AI V" + _version);
    xTaskCreate(faceTask, "faceTask", 1024 * 5, NULL, 1, NULL);
#endif

    String ext4 = esp_ai.getLocalData("ext4");
    String ext5 = esp_ai.getLocalData("ext5");
    String ext6 = esp_ai.getLocalData("ext6");
    String ext7 = esp_ai.getLocalData("ext7");
    String ext8 = esp_ai.getLocalData("diyServerParams");
    String volume_enable = esp_ai.getLocalData("volume_enable");
    String volume_pin = esp_ai.getLocalData("volume_pin");
    String vad_first = esp_ai.getLocalData("vad_first");
    String vad_course = esp_ai.getLocalData("vad_course");
    String mic_bck = esp_ai.getLocalData("mic_bck");
    String mic_ws = esp_ai.getLocalData("mic_ws");
    String mic_data = esp_ai.getLocalData("mic_data");
    String speaker_bck = esp_ai.getLocalData("speaker_bck");
    String speaker_ws = esp_ai.getLocalData("speaker_ws");
    String speaker_data = esp_ai.getLocalData("speaker_data");
    String lights_data = esp_ai.getLocalData("lights_data");
    kwh_enable = esp_ai.getLocalData("kwh_enable");
    String _emotion_led_number = esp_ai.getLocalData("emotion_led_number");
    if (_emotion_led_number != "")
    {
        emotion_led_number = _emotion_led_number.toInt();
    }

    if (kwh_enable == "")
    {
        kwh_enable = "1";
    }

    ESP_AI_volume_config volume_config = {volume_pin.toInt(), 4096, 1, volume_enable == "0" ? false : true};
    ESP_AI_lights_config lights_config = {18}; // esp32s3 开发板是 48
    ESP_AI_reset_btn_config reset_btn_config = {};
// 小智AI 引脚不一样
#if defined(IS_XIAO_ZHI_S3_2) || defined(IS_XIAO_ZHI_S3_3) || defined(IS_WU_MING_TFT) || defined(IS_MA_ZHUANG_TFT)
    lights_data = "48";
    speaker_bck = "15";
    speaker_ws = "16";
    speaker_data = "7";
    mic_bck = "5";
    mic_ws = "4";
    mic_data = "6";
    kwh_enable = "0";
    // 按钮冲突，随便设置一个
    reset_btn_config.pin = 46;
    // 音量旋钮冲突，随便设置一个
    volume_config.input_pin = 3;
    volume_config.enable = false;
#endif
    // LOG_D("lights_data: ");
    // LOG_D(lights_data);

    LOG_D("%s", "===============================");
    LOG_D("\n当前设备版本:%s", _version.c_str());
    if (ext4 != "" && ext5 != "")
    {
        LOG_D("你配置了服务IP, 所以将会请求你的服务！如果想要请求开放平台，请打开配网页面更改。");
        LOG_D("服务协议：%s", ext4.c_str());
        LOG_D("服务IP：%s", ext5.c_str());
        LOG_D("服务端口：%s", ext6.c_str());
    }
    LOG_D("%s", "===============================");

    // [必  填] 是否调试模式， 会输出更多信息
    bool debug = true;
    // [必  填] wifi 配置： { wifi 账号， wifi 密码, "热点名字" } 可不设置，连不上wifi时会打开热点：ESP-AI，连接wifi后打开地址： 192.168.4.1 进行配网(控制台会输出地址，或者在ap回调中也能拿到信息)
    ESP_AI_wifi_config wifi_config = {"", "", "", html_str};

#if defined(BLE_MODEL)
    wifi_config.way = "BLE";
#endif

#if defined(IS_ESP_AI_S3_BASIC)
    kwh_enable = "0";
#endif

    // [必  填] 唤醒方案： { 方案, 语音唤醒用的阈值(本方案忽略即可), 引脚唤醒方案(本方案忽略), 发送的字符串 }
    ESP_AI_wake_up_config wake_up_config = {};
    if (ext7 == "pin_high")
    {
        wake_up_config.pin = 10;
        strcpy(wake_up_config.wake_up_scheme, "pin_high"); // 唤醒方案
    }
    else if (ext7 == "pin_low")
    {
        wake_up_config.pin = 10;
        strcpy(wake_up_config.wake_up_scheme, "pin_low"); // 唤醒方案
    }
    else if (ext7 == "boot")
    {
        wake_up_config.pin = 0;
        strcpy(wake_up_config.wake_up_scheme, "pin_low"); // 唤醒方案
    }
    else if (ext7 == "pin_high_listen")
    {
        wake_up_config.pin = 10;
        strcpy(wake_up_config.wake_up_scheme, "pin_high_listen"); // 唤醒方案
    }
    else if (ext7 == "boot_listen")
    {
        wake_up_config.pin = 0;
        strcpy(wake_up_config.wake_up_scheme, "pin_low_listen"); // 唤醒方案
    }
    else if (ext7 == "edge_impulse")
    {
        wake_up_config.threshold = 0.9;
        strcpy(wake_up_config.wake_up_scheme, "edge_impulse"); // 唤醒方案
    }
    else
    {
        // 默认用天问
        strcpy(wake_up_config.wake_up_scheme, "asrpro"); // 唤醒方案
        strcpy(wake_up_config.str, "start");             // 串口和天问asrpro 唤醒时需要配置的字符串，也就是从另一个开发版发送来的字符串
    }

#if defined(IS_AI_VOX_TFT)
    lights_data = "41";
    // AI_VOX 开发板
    ESP_AI_i2s_config_mic i2s_config_mic = {5, 2, 4, 18, I2S_CHANNEL_FMT_RIGHT_LEFT};
    ESP_AI_i2s_config_speaker i2s_config_speaker = {
        .bck_io_num = 13,
        .ws_io_num = 14,
        .data_in_num = 1};

    // 按钮冲突，随便设置一个
    reset_btn_config.pin = 3;
    // 音量旋钮冲突，随便设置一个
    volume_config.input_pin = 18;
    volume_config.enable = false;
#else
    // ESP-AI 可配置化
    ESP_AI_i2s_config_mic i2s_config_mic = {mic_bck.toInt(), mic_ws.toInt(), mic_data.toInt()};
    ESP_AI_i2s_config_speaker i2s_config_speaker = {speaker_bck.toInt(), speaker_ws.toInt(), speaker_data.toInt()};
#endif

    if (lights_data != "")
    {
        lights_config.pin = lights_data.toInt();
    }

    if (ext4 != "" && ext5 != "")
    {
        strcpy(server_config.protocol, ext4.c_str());
        strcpy(server_config.ip, ext5.c_str());
        strcpy(server_config.params, ext8.c_str());
        server_config.port = ext6.toInt();
    }

    // 错误监听, 需要放到 begin 前面，否则可能监听不到一些数据
    esp_ai.onBegin(onBegin);
    esp_ai.onReady(on_ready);

    // 设备局域网IP
    esp_ai.onConnectedWifi(on_connected_wifi);
    // 设备网络状态监听, 需要放到 begin 前面，否则可能监听不到一些数据
    esp_ai.onNetStatus(on_net_status);
    // 上报位置, 需要放到 begin 前面
    esp_ai.onPosition(on_position);
    // 错误监听, 需要放到 begin 前面，否则可能监听不到一些数据
    esp_ai.onError(onError);
    // 绑定设备，蓝牙设备会在 begin 之前
    esp_ai.onBindDevice(on_bind_device);
    esp_ai.begin({debug, wifi_config, server_config, wake_up_config, volume_config, i2s_config_mic, i2s_config_speaker, reset_btn_config, lights_config});
    // 用户指令监听
    esp_ai.onEvent(on_command);
    esp_ai.onSessionStatus(on_session_status);
    // 情绪检测
    esp_ai.onEmotion(onEmotion);

    device_id = esp_ai.getLocalData("device_id");

    // 恢复音量
    String loc_volume = esp_ai.getLocalData("ext2");
    if (loc_volume != "")
    {

#if !defined(IS_ESP_AI_S3_NO_SCREEN)
        face->SetVolume(int(volume * 100));
#endif
        volume = loc_volume.toFloat();
        LOG_D("本地音量：%0.2f", volume);
        esp_ai.setVolume(volume);
    }
    else
    {
#if !defined(IS_ESP_AI_S3_NO_SCREEN)
        face->SetVolume(int(1 * 100));
#endif
    }

    // boot 按钮唤醒方式 esp-ai 库有问题，这begin()后再设置下就好了
    if (ext7 == "boot_listen" || ext7 == "boot")
    {
        pinMode(0, INPUT_PULLUP);
    }

    // 自定义服务器也可以和 ESP-AI 业务服务进行联通，而无限制。
    // if (ext4 != "" && ext5 != "") return;

    // 情绪灯光
    pixelsEmotion.begin();
    pixelsEmotion.setBrightness(100); // 亮度设置

    // 连接业务服务器
    webScoket_yw_main();

    // 定时器初始化
    timer.setInterval(report_systeminfo_interval, report_systeminfo_timer1, 0);
    timer.setInterval(1L, websocket_timer2, 0);
    timer.setInterval(1000L, update_check_timer3, 0);
    timer.setInterval(10L, espai_loop_timer5, 0);
    timer.setInterval(600000L, battery_detection_timer, 0);

#if defined(IS_XIAO_ZHI_S3_2) || defined(IS_XIAO_ZHI_S3_3) || defined(IS_WU_MING_TFT) || defined(IS_AI_VOX_TFT) || defined(IS_ESP_AI_S3_DOUBLE_OLED)
    // 按键初始化
    button_vol_add.attachClick(attach_vol_add); // 加音量按键
    button_vol_sub.attachClick(attach_vol_sub); // 减音量按键
    timer.setInterval(50L, key_timer4, 0);
#endif
}

void loop()
{
    // 定时器
    timer.run();
    // log_fh();
}

long last_log_time = 0;
void log_fh()
{
    if (millis() - last_log_time > 3000)
    {
        last_log_time = millis();
        // Serial.print("===> 可用内存: ");
        // Serial.print(ESP.getFreeHeap() / 1024);
        // Serial.println("KB"); 

        Serial.print("是否在充电中: ");
        Serial.println(battery.getischarge());

    }
}
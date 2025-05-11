// ================= S3 开发板选择 ====================
#define IS_ESP_AI_S3
// ====================================================

#include <esp-ai.h>
#include <HTTPUpdate.h>
#include <Wire.h>
#include <esp_adc_cal.h>
#include <driver/adc.h>

#include <TJpg_Decoder.h>  // JPEG decoder library
#include "SPI.h"
#include <TFT_eSPI.h>  // Hardware-specific library for TFT display
#include "U8g2_for_TFT_eSPI.h"


#include <map>
#include "esp_heap_caps.h"

// 图片缓存结构体
struct ImageCache {
  uint8_t *buffer;
  int len;
};

std::map<String, ImageCache> imageCacheMap;
std::vector<String> imageCacheOrder;
const size_t MAX_CACHE_IMAGES = 10;  // 最大缓存的图片数量，超过后自动清除旧数据


// Declare TFT object
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft); 
U8g2_for_TFT_eSPI *u8g2;

// ====================================================
String _version = "1.0.0";
String _bin_id = "500";
// ====================================================

// GLOBALS
// Face *face;
esp_adc_cal_characteristics_t *adc_chars = NULL;

/*** 线上地址 http ***/
String domain = "http://api.espai.fun/";
String yw_ws_ip = "api.espai.fun";
String yw_ws_path = "";
int yw_ws_port = 80;
// 这里不配置接口，配网页面也没有 api_key 配置，这会导致使用默认的服务节点：node.espai.fun
ESP_AI_server_config server_config = { "http", "node.espai.fun", 80 };

struct EmotionImagePair {
  String emotion;
  String imageURL;
};

// 定义一个情感与图片 URL 的映射数组
EmotionImagePair emotionToImage[] = {
  // 第一个图一定要放默认图片
  { "默认", "http://esp-ai2.oss-cn-beijing.aliyuncs.com/tft_imgs/liuying/default.jpg" },
  { "快乐", "http://esp-ai2.oss-cn-beijing.aliyuncs.com/tft_imgs/liuying/kuai-le.jpg" },
  { "意外", "http://esp-ai2.oss-cn-beijing.aliyuncs.com/tft_imgs/liuying/kuai-le.jpg" },
  { "愤怒", "http://esp-ai2.oss-cn-beijing.aliyuncs.com/tft_imgs/liuying/shen-qi.jpg" },
  { "恐惧", "http://esp-ai2.oss-cn-beijing.aliyuncs.com/tft_imgs/liuying/kong-ju.jpg" },
  { "敬畏", "http://esp-ai2.oss-cn-beijing.aliyuncs.com/tft_imgs/liuying/kong-ju.jpg" },
  { "专注", "http://esp-ai2.oss-cn-beijing.aliyuncs.com/tft_imgs/liuying/si-kao.jpg" },
  { "疑问", "http://esp-ai2.oss-cn-beijing.aliyuncs.com/tft_imgs/liuying/si-kao.jpg" },
  { "伤心", "http://esp-ai2.oss-cn-beijing.aliyuncs.com/tft_imgs/liuying/ku-qi.jpg" },
  { "懊恼", "http://esp-ai2.oss-cn-beijing.aliyuncs.com/tft_imgs/liuying/ku-qi.jpg" }
};



// ====================================================================================
// ====================================================================================

ESP_AI esp_ai;
WebSocketsClient webSocket_yw;
bool is_yw_connected = false;  // 业务服务是否已经连接
String ota_progress = "0%";
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


// 当前情绪状态
String emotion = "无情绪";
String prve_emotion = "无情绪";
long prve_emotion_time = 0;

// 底部绘制的可移动的文字
String bttomText;
// 上一次移动的坐标
int bttomTextPrevLeft;

// 顶部状态图标
String topLeftText;
String prevTopLeftText;

/**
 * 电量，电压检测
 * 使用模块 MH Electronic （直接输出电压）
 * esp32   |   MH Electronic
 *  3.3v   |    +
 *  GND    |    -
 *  8     |    s
 * 
 * 下面代码参数：
 * 聚合锂电池：3.7v  (3.3v~4.2v)
 * 满电状态读取 ADC 值约 1000
 */
// ========================= 电量获取 =========================
/**
* 电量获取
* BatteryMonitor(ADC 输入IO, 满电时 adc 值)
*/
class BatteryMonitor {
public:
  // 构造函数
  BatteryMonitor(int pin, int adcValueAtFull)
    : batteryPin(pin), adcValueAtFull(adcValueAtFull) {}

  // 获取电池电量百分比
  int getBatteryPercentage() {

    // 多次采样以减少噪声
    int adcSum = 0;
    for (int i = 0; i < numSamples; i++) {
      // 读取 ADC 值，注意：S3 -> ADC1_CHANNEL_8。
      uint32_t adc_reading = adc1_get_raw(ADC1_CHANNEL_8);
      // uint32_t adc_reading = adc1_get_raw(ADC1_CHANNEL_7);

      // 使用校准函数将读取的 ADC 值转换为电压值
      uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
      // Serial.printf("ADC Reading: %d, Voltage: %d mV, 电量: %d %\n", adc_reading, voltage, map(voltage, 0, 1024, 0, 100));
      adcSum += map(voltage, 0, 1024, 0, 100);
      delay(5);  // 稍微延迟以获得稳定的读数
    }
    float adcValue = adcSum / numSamples;

    // Serial.print("原始值：");
    // Serial.print(adcValue);
    // Serial.print("\n");

    return adcValue;
  }

private:
  const int batteryPin;
  // const int numSamples = 10;  // 采样次数
  const int numSamples = 1;  // 采样次数

  // 已知参数
  const float VREF = 3.3;    // ESP32的参考电压
  const int ADC_MAX = 4095;  // 12位ADC的最大值

  // 电压模块校准值
  const float measuredVoltageAtFull = 4.2;  // 100% 电量对应的电压（典型值为 4.2V）
  const int adcValueAtFull;                 // 电量 100% 对应的 ADC 值
};

// 创建一个 BatteryMonitor 实例
BatteryMonitor batteryMonitor(8, 1181);  // gooovvv 输入的adc 是 930

// 读取电量的间隔
int readBatteryMonitorTime = 60 * 1000;  // 1分钟上报一次电量
// int readBatteryMonitorTime = 3 * 1000;  // 测试3s上报 test...
long prevReadTime = 0;  // 上一次读取时间
int prevBattery = 0;    // 上一次读取的电量

// 上报设备电量的函数
void uploadBattery() {
  if (kwh_enable == "0") {
    return;
  }

// ESP-AI 开发版需要做电量检测
#if defined(IS_ESP_AI_S3) || defined(IS_ESP_AI_S3_NO_SCREEN)
  String ext4 = esp_ai.getLocalData("ext4");
  String ext5 = esp_ai.getLocalData("ext5");
  if (ext4 != "" && ext5 != "") {
    return;
  }
  int batteryLevel = batteryMonitor.getBatteryPercentage();  // 获取电池电量百分比
  Serial.print("上报电量：");
  Serial.println(batteryLevel);

#if !defined(IS_ESP_AI_S3_NO_SCREEN)
  // face->SetKWH(batteryLevel);
#endif

  if (batteryLevel > prevBattery) {
    // 说明在充电
    Serial.println("设备充电中...");
  } else if ((prevBattery - batteryLevel) >= 3) {
    // 如果每一次的电量都是一样的，可能是没有插电池
    if (batteryLevel < 10) {
      esp_ai.tts("设备电量不足10%啦，请帮我充电哦。");
    }

    if (batteryLevel < 5) {
      esp_ai.tts("设备电量不足5%啦，马上变成一块砖啦！");
    }
  }
  prevBattery = batteryLevel;

  JSONVar data_battery;
  data_battery["type"] = "battery";
  data_battery["data"] = batteryLevel;
  data_battery["device_id"] = device_id;
  String sendData = JSON.stringify(data_battery);
  webSocket_yw.sendTXT(sendData);
#endif
}

// ========================= OTA升级逻辑 =========================
long start_update_time;
bool start_update_ed = false;     // 如果迟迟不拿不到升级进度，说明升级失败了
bool is_update_progress = false;  // 如果迟迟不拿不到升级进度，说明升级失败了
void update_ota(String url) {

#if !defined(IS_ESP_AI_S3_NO_SCREEN)
  // face->DrawStr("设备正在升级。", -30);
#endif
  esp_ai.stopSession();
  delay(1000);
  // esp_ai.tts("设备正在升级，请勿进行操作。");
  // delay(6000);
  // // return;

  // ota 升级
  WiFiClient upDateClient;
  httpUpdate.onStart(update_started);
  httpUpdate.onEnd(update_finished);
  httpUpdate.onProgress(update_progress);
  httpUpdate.onError(update_error);
  long time = millis();
  // String url = downLoadUpdateUrl + version + ".bin";
  Serial.println("ota url:" + url);

  t_httpUpdate_return ret = httpUpdate.update(upDateClient, url);
  Serial.println("===================升级完毕=========================");
  Serial.println(ret);
  switch (ret) {
    case HTTP_UPDATE_FAILED:  //当升级失败
      Serial.println("[update] Update failed.");
      break;
    case HTTP_UPDATE_NO_UPDATES:  //当无升级
      Serial.println("[update] Update no Update.");
      break;
    case HTTP_UPDATE_OK:  //当升级成功
      Serial.println("[update] Update ok.");
      break;
  }

  start_update_time = time;
  // start_update_ed = true;
}

// OTA升级开始回调函数
void update_started() {
  start_update_time = millis();
  start_update_ed = true;
  Serial.println("CALLBACK:  HTTP更新进程启动");
}

// OTA升级完成回调函数
void update_finished() {
  Serial.println("[Info] -> CALLBACK:  HTTP更新进程完成");
}

// 避免发送的进度过于频繁
long prev_send_progress_time = 0;
// OTA升级中回调函数
void update_progress(int cur, int total) {
  is_update_progress = true;
  float percentage = (float)cur / total * 100;
  ota_progress = String(percentage, 2) + "%";
  Serial.print("[Info] -> 更新进度：");
  Serial.print(ota_progress);
  Serial.print("   可用内存: ");
  Serial.print(ESP.getFreeHeap() / 1024);
  Serial.println("KB");


  // 1.5秒钟发送一次
  if (millis() - prev_send_progress_time > 1500) {
    prev_send_progress_time = millis();
    // 将进度发往业务服务
    JSONVar data_ota;
    data_ota["type"] = "ota_progress";
    data_ota["data"] = percentage;
    data_ota["device_id"] = device_id;
    String sendData = JSON.stringify(data_ota);
    webSocket_yw.sendTXT(sendData);

    esp_ai_pixels.setPixelColor(0, esp_ai_pixels.Color(218, 164, 90));
    esp_ai_pixels.setBrightness(50);  // 亮度设置
    esp_ai_pixels.show();
    delay(100);
    esp_ai_pixels.setBrightness(100);  // 亮度设置
    esp_ai_pixels.show();
  }
}

// OTA升级错误回调函数
void update_error(int err) {
  Serial.printf("CALLBACK:  HTTP更新致命错误代码 %d\n", err);
}


// ========================= 业务服务的 ws 连接 =========================
void webSocketEvent_ye(WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.println("业务ws服务已断开\n");
      is_yw_connected = false;
      // 玳瑁黄
      esp_ai_pixels.setPixelColor(0, esp_ai_pixels.Color(218, 164, 90));
      esp_ai_pixels.setBrightness(80);  // 亮度设置
      esp_ai_pixels.show();
      delay(1000);
      // 玳瑁黄
      esp_ai_pixels.setPixelColor(0, esp_ai_pixels.Color(218, 164, 90));
      esp_ai_pixels.setBrightness(10);  // 亮度设置
      esp_ai_pixels.show();
      break;
    case WStype_CONNECTED:
      {
        Serial.println("√ 业务ws服务已连接\n");
        is_yw_connected = true;
        // 上报电量
        uploadBattery();
        // 指示灯状态应该由设备状态进行控制
        on_net_status(_net_status);
        // 上报版本等信息
        // upload_info();
        break;
      }
    case WStype_TEXT:
      {
        JSONVar parseRes = JSON.parse((char *)payload);
        if (JSON.typeof(parseRes) == "undefined") {
          return;
        }
        if (parseRes.hasOwnProperty("type")) {
          String type = (const char *)parseRes["type"];
          if (type == "ota_update") {
            String url = (const char *)parseRes["url"];
            Serial.println("收到ota升级指令");
            update_ota(url);
          } else if (type == "stop_session") {
            Serial.println("业务服务要求停止会话");
            esp_ai.stopSession();
          } else if (type == "set_volume") {
            double volume = (double)parseRes["volume"];
            Serial.print("接收到音量：");
            Serial.println(volume);
            esp_ai.setVolume(volume);
            // 存储到本地
            esp_ai.setLocalData("ext2", String(volume, 1));
          } else if (type == "message") {
            String message = (const char *)parseRes["message"];
            Serial.print("收到服务提示：");
            Serial.println(message);
          } else if (type == "get_local_data") {
            Serial.print("收到 get_local_data 指令：");
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
          } else if (type == "set_local_data") {
            JSONVar data = parseRes["data"];
            JSONVar keys = data.keys();
            Serial.println("收到 set_local_data 指令：");
            Serial.println(data);
            for (int i = 0; i < keys.length(); i++) {
              String key = keys[i];
              JSONVar value = data[key];
              esp_ai.setLocalData(key, String((const char *)value));
            }
            delay(250);
            Serial.println(F("即将重启设备更新设置。"));
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
      Serial.println("业务服务 WebSocket 连接错误");
      break;
  }
}

void webScoket_yw_main() {
  String device_id = esp_ai.getLocalData("device_id");
  String ext1 = esp_ai.getLocalData("ext1");
  String ext2 = esp_ai.getLocalData("ext2");
  String ext3 = esp_ai.getLocalData("ext3");
  String ext4 = esp_ai.getLocalData("ext4");
  String ext5 = esp_ai.getLocalData("ext5");

  if (ext4 != "" && ext5 != "") {
    return;
  }

  Serial.println("开始连接业务 ws 服务\n");
  if (_net_status == "0_ap") {
    Serial.println("设备还未配网，忽略业务服务连接。");
    return;
  }
  // ws 服务
  if (yw_ws_ip == "api.espai.fun" || yw_ws_ip == "api.espai2.fun") {
    // webSocket_yw.beginSSL(
    webSocket_yw.begin(
      yw_ws_ip,
      yw_ws_port,
      yw_ws_path + "/connect_espai_node?device_type=hardware&device_id=" + device_id + "&version=" + _version + "&api_key=" + ext1 + "&ext2=" + ext2 + "&ext3=" + ext3 + "&ext4=" + ext4 + "&ext5=" + ext5);

  } else {
    webSocket_yw.begin(
      yw_ws_ip,
      yw_ws_port,
      yw_ws_path + "/connect_espai_node?device_type=hardware&device_id=" + device_id + "&version=" + _version + "&api_key=" + ext1 + "&ext2=" + ext2 + "&ext3=" + ext3 + "&ext4=" + ext4 + "&ext5=" + ext5);
  }

  webSocket_yw.onEvent(webSocketEvent_ye);
  webSocket_yw.setReconnectInterval(3000);
  webSocket_yw.enableHeartbeat(10000, 5000, 0);
}


// ========================= 错误监听 =========================
void onError(String code, String at_pos, String message) {
  // some code...
  Serial.printf("\n监听到错误：%s\n", code);
  String loc_ext7 = esp_ai.getLocalData("ext7");
  if (code == "006" && loc_ext7 != "") {
    // 清除板子信息，让板子重启，否则板子会不断重开热点无法操作
    // WiFi.mode(WIFI_AP_STA);
    // WiFi.softAP("AI故事机");
    esp_ai.setLocalData("wifi_name", "");
    esp_ai.setLocalData("wifi_pwd", "");
    esp_ai.setLocalData("ext1", "");
    ESP.restart();
  }
}



// ========================= 指令监听 =========================
void on_command(String command_id, String data) {

  // // 爱小明后出现一张图片
  // if (command_id == "love_ming") {
  //   renderImageFromURL("http://esp-ai2.oss-cn-beijing.aliyuncs.com/tft_imgs/yang_mi/da-xiao-240.jpg");
  // }


  if (command_id == "add_volume") {
    Serial.println("加音量");
    up_click(0.1);
  } else if (command_id == "subtract_volume") {
    Serial.println("减音量");
    down_click(0.1);
  } else if (command_id == "query_battery") {
    int batteryLevel = batteryMonitor.getBatteryPercentage();  // 获取电池电量百分比
    Serial.println("查询电量");
    Serial.println("设备电量剩余" + String(batteryLevel) + "%");
    esp_ai.stopSession();
    esp_ai.tts("设备电量剩余" + String(batteryLevel) + "%");
  } else if (command_id == "on_iat_cb") {
    bttomText = data;
    // 重置文字的x位置
    bttomTextPrevLeft = 0;
  } else if (command_id == "on_llm_cb") {
    bttomText = data;
    // 重置文字的x位置
    bttomTextPrevLeft = 0;
  }
}


// ========================= 会话状态监听 =========================
void on_session_status(String status) {
  // Serial.println("会话状态===: " + status);
  _session_status = status;

 // 处理会话状态
  if (status == "iat_start") {
    // face->Behavior.Clear();
    // face->Behavior.SetEmotion(eEmotions::Squint, 1.0);
    // face->DrawStrLeftTop("聆听中", 2);
    topLeftText = "聆听中";
  } else if (status == "iat_end") {
    // face->Behavior.Clear();
    // face->Behavior.SetEmotion(eEmotions::Happy, 1.0);
    // face->Behavior.SetEmotion(eEmotions::Surprised, 1.0);
    // face->DrawStrLeftTop("AI说话中", 2);
    topLeftText = "AI说话中";
  } else if (status == "tts_real_end") {
    // face->Behavior.Clear();
    // face->Behavior.SetEmotion(eEmotions::Normal, 1.0);
    // face->DrawStrLeftTop("待命中", 2);
    topLeftText = "待命中";
    renderImageFromURL(emotionToImage[0].imageURL.c_str(), false);
  } else if (status == "tts_chunk_start") {
  } else {
  }
}

// ========================= 网络状态监听 =========================
void on_net_status(String status) {
  // Serial.println("网络状态: " + status);
  _net_status = status;

  if (status == "0_ing") {
#if !defined(IS_ESP_AI_S3_NO_SCREEN)
    // face->SetWifiStatus(false);
    // face->DrawStr("正在连接网络", -30);
#endif
  };
  if (status == "0_ap") {
#if !defined(IS_ESP_AI_S3_NO_SCREEN)
    // face->DrawStr("连接 ESP-AI 热点配网", -60);
#endif
  };

  if (status == "2") {
#if !defined(IS_ESP_AI_S3_NO_SCREEN)
    // face->SetWifiStatus(true);
    // face->DrawStr("正在连接服务", -34);
#endif
  };

  // 已连接服务
  if (status == "3") {
#if !defined(IS_ESP_AI_S3_NO_SCREEN)
    // face->DrawStrLeftTop("待命中", 2);
#endif

    String ext3 = esp_ai.getLocalData("ext3");
    if (ext3 == "") {
      delay(2000);
      Serial.println("播放提示语");
      esp_ai.setLocalData("ext3", "1");
      esp_ai.tts("设备激活成功，现在可以和我聊天了哦。");
    }
  };
}

bool onBegin() {
  if (kwh_enable == "0") {
    return true;
  }

#if defined(IS_XIAO_ZHI_S3_2) || defined(IS_XIAO_ZHI_S3_3)
  return true;
#endif

// ESP-AI 开发版需要做电量检测
#if defined(IS_ESP_AI_S3) || defined(IS_ESP_AI_S3_NO_SCREEN)
  int batteryLevel = batteryMonitor.getBatteryPercentage();  // 获取电池电量百分比
  Serial.print("[Info] -> 电池电量：");
  Serial.println(batteryLevel);
  if (batteryLevel < 10) {
#if !defined(IS_ESP_AI_S3_NO_SCREEN)
    // face->DrawStr("电量过低，请连接充电器。", -55);
#endif

    while (true) {
      Serial.println(F("电量不足"));
      Serial.println(F("如果您的开发板没有电压检测模块，请在配网页面或者设备配置页面禁用电量检测功能。"));
      esp_ai_dec.write(mei_dian_le, mei_dian_le_len);
      delay(8000);
    }

    return false;
  }
#endif
  return true;
}


float volume = 1;


void up_click(float number) {
  volume += number;
  if (volume > 1) {
    volume = 1;
  }
  esp_ai.setVolume(volume);
  Serial.print("音量加: ");
  Serial.println(volume);
#if !defined(IS_ESP_AI_S3_NO_SCREEN)
  // face->DrawStrTopCenter("音量：" + String(int(volume * 100)) + "%");
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
void down_click(float number) {
  volume -= number;
  if (volume < 0) {
    volume = 0;
  }
  esp_ai.setVolume(volume);
  Serial.print("音量减: ");
  Serial.println(volume);
#if !defined(IS_ESP_AI_S3_NO_SCREEN)
  // face->DrawStrTopCenter("音量：" + String(int(volume * 100)) + "%");
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



// ========================= 绑定设备 =========================
String encodeURIComponent(String input) {
  String encoded = "";
  char hexBuffer[4];  // 用于存储百分比编码后的值

  for (int i = 0; i < input.length(); i++) {
    char c = input[i];

    // 检查是否为不需要编码的字符（字母、数字、-、_、.、~）
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      encoded += c;  // 不需要编码的字符直接添加
    } else {
      // 对需要编码的字符进行百分比编码
      snprintf(hexBuffer, sizeof(hexBuffer), "%%%02X", c);
      encoded += hexBuffer;
    }
  }

  return encoded;
}


HTTPClient on_bind_device_http;
String on_bind_device(JSONVar data) {
  Serial.println("\n[Info] -> 绑定设备中");
  String device_id = data["device_id"];
  String wifi_name = data["wifi_name"];
  String wifi_pwd = data["wifi_pwd"];
  String ext1 = data["ext1"];
  String ext4 = data["ext4"];
  String ext5 = data["ext5"];

  esp_ai_pixels.setBrightness(10);  // 亮度设置
  esp_ai_pixels.show();


  // ext4 存在时是请求的自己的服务器，所以不去请求绑定服务
  // if (ext4.length() > 0 && ext5.length() > 0) {
  if (ext4 != "" && ext5 != "") {
#if !defined(IS_ESP_AI_S3_NO_SCREEN)
    // face->DrawStr("设备激活成功", -30);
#endif
    return "{\"success\":true,\"message\":\"设备激活成功。\"}";
  }


  String loc_api_key = esp_ai.getLocalData("ext1");
  String loc_ext7 = esp_ai.getLocalData("ext7");

  // 如果 api_key 没有变化，那不应该去请求服务器绑定
  if (loc_api_key == ext1) {
    Serial.println("\n[Info] -> 仅仅改变 wifi 等配置信息");
    return "{\"success\":true,\"message\":\"设备激活成功。\"}";
  }

  // 创建一个HTTP客户端对象
  Serial.println("[Info] ext4: " + ext4);
  Serial.println("[Info] ext5: " + ext5);
  Serial.println("[Info] api_key: " + ext1);
  Serial.println("[Info] device_id: " + device_id);
  // Serial.println("[Info] loc_api_key: " + loc_api_key);

  String url = domain + "devices/add";


  on_bind_device_http.begin(url);
  on_bind_device_http.addHeader("Content-Type", "application/json");

  JSONVar json_params;
  json_params["version"] = _version;
  json_params["bin_id"] = _bin_id;
  json_params["device_id"] = device_id;
  json_params["api_key"] = ext1;
  json_params["wifi_ssid"] = wifi_name;
  json_params["wifi_pwd"] = wifi_pwd;
  String send_data = JSON.stringify(json_params);

  // 发送POST请求并获取响应代码
  int httpCode = on_bind_device_http.POST(send_data);

  esp_ai_pixels.setBrightness(100);  // 亮度设置
  esp_ai_pixels.show();

  if (httpCode > 0) {
    String payload = on_bind_device_http.getString();
    Serial.printf("[HTTPS] POST code: %d\n", httpCode);
    // Serial.printf("[HTTPS] POST res: %d\n", payload);
    JSONVar parse_res = JSON.parse(payload);

    if (JSON.typeof(parse_res) == "undefined" || String(httpCode) != "200") {
      on_bind_device_http.end();
      Serial.printf("[Error HTTPS] 请求网址: ");
      Serial.println(url);

#if !defined(IS_ESP_AI_S3_NO_SCREEN)
      // face->DrawStr("设备激活失败，请重试。", -50);
#endif
      // 这个 json 数据中的 message 会在配网页面弹出
      return "{\"success\":false,\"message\":\"设备绑定失败，错误码:" + String(httpCode) + "，重启设备试试呢。\"}";
    }

    if (parse_res.hasOwnProperty("success")) {
      bool success = (bool)parse_res["success"];
      String message = (const char *)parse_res["message"];
      if (success == false) {
#if !defined(IS_ESP_AI_S3_NO_SCREEN)
        // face->DrawStr("绑定设备失败。", -30);
#endif
        // 绑定设备失败
        Serial.println("[Error] -> 绑定设备失败，错误信息：" + message);
        // esp_ai.tts("绑定设备失败，重启设备试试呢，本次错误原因：" + message);
        on_bind_device_http.end();
        // 这个 json 数据中的 message 会在配网页面弹出
        return "{\"success\":false,\"message\":\"绑定设备失败，错误原因：" + message + "\"}";
      } else {
#if !defined(IS_ESP_AI_S3_NO_SCREEN)
        // face->DrawStr("设备激活成功！", -30);
#endif
        // 设备激活成功！
        Serial.println("[Info] -> 设备激活成功！");
        on_bind_device_http.end();
        // 这个 json 数据中的 message 会在配网页面弹出
        return "{\"success\":true,\"message\":\"设备激活成功，即将重启设备。\"}";
      }
    } else {
#if !defined(IS_ESP_AI_S3_NO_SCREEN)
      // face->DrawStr("设备激活失败，请求服务失败！", -50);
#endif
      Serial.printf("[Error HTTPS] 请求网址: ");
      Serial.println(url);
      on_bind_device_http.end();
      return "{\"success\":false,\"message\":\"设备绑定失败，错误码:" + String(httpCode) + "，重启设备试试呢。\"}";
    }
  } else {
    Serial.printf("[Error HTTPS] 绑定设备失败: %s\n", String(httpCode));
    Serial.printf("[Error HTTPS] 请求网址:");
    Serial.println(url);
    on_bind_device_http.end();
    return "{\"success\":false,\"message\":\"请求服务失败，请刷新页面重试。\"}";
  }
}


void on_position(String ip, String nation, String province, String city, String latitude, String longitude) {

  while (is_yw_connected == false) {
    delay(300);
  }

  Serial.println("============ 定位成功 ============");
  Serial.println(ip);
  Serial.println(nation);
  Serial.println(province);
  Serial.println(city);
  Serial.println(latitude);
  Serial.println(longitude);


  String device_id = esp_ai.getLocalData("device_id");
  String loc_ext1 = esp_ai.getLocalData("ext1");
  String loc_wifi_name = esp_ai.getLocalData("wifi_name");
  String loc_wifi_pwd = esp_ai.getLocalData("wifi_pwd");

  JSONVar json_params;
  json_params["api_key"] = loc_ext1;
  json_params["version"] = _version;
  json_params["bin_id"] = _bin_id;
  json_params["device_id"] = device_id;
  json_params["wifi_ssid"] = loc_wifi_name;
  json_params["wifi_pwd"] = loc_wifi_pwd;
  // 本地ip
  json_params["net_ip"] = _device_ip;
  // 设备地址
  if (nation) {
    json_params["nation"] = nation;
  }
  if (province) {
    json_params["province"] = province;
  }
  if (city) {
    json_params["city"] = city;
  }
  if (latitude) {
    json_params["latitude"] = latitude;
  }
  if (longitude) {
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

void on_connected_wifi(String device_ip) {
  _device_ip = device_ip;
}


// ===========================================================
// [可 填] 自定义配网页面
#if defined(IS_ESP_AI_S3) || defined(IS_ESP_AI_S3_NO_SCREEN)
const char html_str[] PROGMEM = R"rawliteral( 
<!DOCTYPE html>
<html lang='en'>

<head>
    <meta charset='UTF-8'>
    <meta name='viewport'
        content='viewport-fit=cover,width=device-width, initial-scale=1, maximum-scale=1, minimum-scale=1, user-scalable=no' />
    <title>ESP-AI 配网</title>
    <style>
        body {
            font-family: 'Helvetica Neue', Helvetica, Arial, sans-serif;
            margin: 0;
            font-size: 16px;
            color: #333;
            position: relative;
            background-color: aliceblue;
        }

        h1,
        h2,
        h3 {
            font-family: 'Intro', 'Rex', 'Cubano', 'Exo', 'Arial', sans-serif;
        }

        #wifi_setting_panel {
            height: 100vh;
            width: 100vw;
            overflow-y: auto;
            overflow-x: hidden;
            padding: 12px 14px;
            box-sizing: border-box;
        }

        select {
            -webkit-appearance: none;
            appearance: none;
            background-color: #fff;
            border: 1px solid #ccc;
            border-radius: 4px;
            padding: 10px;
            color: #333;
            box-sizing: border-box;
        }

        /* 针对iOS的特殊处理 */
        @media screen and (-webkit-min-device-pixel-ratio:0) {
            select {
                padding-right: 30px;
            }
        }

        /* 焦点状态 */
        select:focus {
            outline: none;
            border-color: #40cea5;
        }

        /* 修改边框颜色 */
        #ext4,
        #wakeup-type,
        #ssid {
            border-radius: 3px;
            font-size: 14px;
            padding: 10px 12px;
            box-sizing: border-box;
            border: 1px solid #e9e9e9;
            outline: none;
            /* width: 100%; */
        }

        .inputs-container {
            background-color: #fff;
            border-radius: 8px;
            padding: 12px 0px;
        }

        #title {
            background: aliceblue;
            width: 100%;
            padding: 12px 0px;
            box-sizing: border-box;
            font-size: 14px;
            font-weight: 600;
            font-weight: 700;
            color: #777;
        }

        .input {
            border-radius: 3px;
            font-size: 14px;
            padding: 10px 12px;
            box-sizing: border-box;
            border: 1px solid #e9e9e9;
            outline: none;
            width: calc(100% - 105px);
        }

        .label {
            display: inline-block;
            width: 100px;
            font-size: 14px;
        }

        .input_wrap {
            width: 100%;
            padding: 8px 18px;
            box-sizing: border-box;
        }

        .desc {
            font-size: 12px;
            color: #777;
            padding: 8px 24px;
        }

        #loading {
            position: fixed;
            z-index: 2;
            top: 0px;
            bottom: 0px;
            left: 0px;
            right: 0px;
            background-color: rgba(51, 51, 51, 0.8);
            text-align: center;
            color: #fff;
            font-size: 22px;
            transition: 0.3s;
        }

        #msg {
            position: fixed;
            z-index: 2;
            top: 0px;
            bottom: 0px;
            left: 0px;
            right: 0px;
            background-color: rgba(51, 51, 51, 0.8);
            text-align: center;
            color: #fff;
            font-size: 22px;
            transition: 0.3s;
            display: none;
        }

        #msg_content {
            background-color: #fff;
            width: 300px;
            height: 300px;
            border-radius: 12px;
            box-shadow: 0px 0px 3px #ccc;
            position: absolute;
            left: 0px;
            right: 0px;
            top: 0px;
            bottom: 0px;
            margin: auto;
            color: #333;
            font-size: 14px;
            padding: 24px;
            box-sizing: border-box;
        }

        #msg_content_text {
            height: 220px;
            margin-bottom: 12px;
            overflow: auto;
        }

        #msg_content_btns {
            padding: 6px;
            box-sizing: border-box;
            background-color: #40cea5;
            color: #fff;
            letter-spacing: 2px;
            border-radius: 8px;
        }

        #loading .icon {
            border: 16px solid #f3f3f3;
            border-radius: 50%;
            border-top: 16px solid #09aba2;
            border-bottom: 16px solid #40cea5;
            width: 120px;
            height: 120px;
            margin: auto;
            position: absolute;
            bottom: 0px;
            top: 0px;
            left: 0px;
            right: 0px;
            -webkit-animation: spin 2s linear infinite;
            animation: spin 2s linear infinite;
        }

        .header {
            display: flex;
            align-items: center;
            justify-content: center;
            padding-bottom: 12px;
        }

        @-webkit-keyframes spin {
            0% {
                -webkit-transform: rotate(0deg);
            }

            100% {
                -webkit-transform: rotate(360deg);
            }
        }

        @keyframes spin {
            0% {
                transform: rotate(0deg);
            }

            100% {
                transform: rotate(360deg);
            }
        }
    </style>
</head>

<body>
    <div id='wifi_setting_panel'>
        <div class='block'>
            <div id='title'>
                网络配置
            </div>
            <div class='inputs-container'>
                <div class='input_wrap'>
                    <label class='label'><span style='color:red;'>*</span>wifi账号</label>
                    <select id='ssid' placeholder='请选择 wifi' class='input'>
                        <option value='' id='scan-ing'>网络扫描中...</option>
                    </select>
                </div>
                <div class='input_wrap'>
                    <label class='label'><span style='color:red;'>*</span>wifi密码</label>
                    <input id='password' name='password' type='text' placeholder='请输入WIFI密码' class='input'>
                </div>
            </div>

        </div>

        <div class='block'>
            <div id='title'>
                唤醒/对话方式
            </div>
            <div class='inputs-container'>
                <div class='input_wrap'>
                    <label class='label' style='width:  70px'><span style='color:red;'>*</span>唤醒方式</label>
                    <select id='wakeup-type' name='wakeup-type' placeholder='请选择唤醒/对话方式' value=''
                        style='width:  calc(100% - 80px)'>
                        <option value='asrpro' id='asrpro'>天问唤醒</option>
                        <option value='edge_impulse' id='edge_impulse'>内置语音唤醒（优化中）</option>
                        <option value='pin_high' id='pin_high'>按钮高电平唤醒(三角按钮)</option>
                        <option value='pin_low' id='pin_low'>按钮低电平唤醒(三角按钮)</option>
                        <option value='boot' id='boot'>BOOT按钮唤醒</option>
                        <option value='boot_listen' id='boot_listen'>按住对话(BOOT按钮)</option>
                        <option value='pin_high_listen' id='pin_high_listen'>按住对话(三角按钮)</option>
                    </select>
                </div>
            </div>

        </div>

        <div class='block' style='display: none;'>
            <div id='title'>
                开发板适配
            </div>
            <div class='inputs-container'>
                <div class='input_wrap'>
                    <select id='pcb-type' name='pcb-type' placeholder='请选择唤醒/对话方式' value=''
                        style='width:  calc(100% - 0px)' onchange='pcbChange()'>
                        <option value='esp-ai-s3' id='esp-ai-s3'>ESP-AI S3版本</option>
                        <option value='xiao-zhi-s3' id='xiao-zhi-s3'>小智AI S3版本</option>
                    </select>
                </div>
                <div class='desc'> 一键适配主流的各种开发板或设备 </div>
            </div>

        </div>


        <div class='block'>
            <div id='title'>
                服务配置
            </div>
            <div class='inputs-container'>
                <div class='input_wrap'>
                    <input type='radio' name='server-type' value='esp-ai-server' id='esp-ai-server' checked
                        onchange='useEspAIServer()'>
                    <label for='esp-ai-server'>使用开放平台服务</label>
                    <input type='radio' name='server-type' value='diy-server' id='diy-server' onchange='useDiyServer()'>
                    <label for='diy-server'>使用自定义服务</label>
                </div>
                <div class='input_wrap' style='display: none;' id='ext4_wrap'>
                    <label class='label'><span style='color:red;'>*</span>服务协议</label>
                    <select id='ext4' name='ext4' style='background: #fff;padding-right: 20px;' placeholder='请选择服务协议'
                        value='' class='input'>
                        <option value='http' id='http'>http</option>
                        <option value='https' id='https'>https</option>
                    </select>
                </div>
                <div class='input_wrap' style='display: none;' id='ext5_wrap'>
                    <label class='label'><span style='color:red;'>*</span>服务地址</label>
                    <input id='ext5' name='ext5' type='text' placeholder='服务地址(IP或者域名)...' class='input'>
                </div>
                <div class='input_wrap' style='display: none;' id='ext6_wrap'>
                    <label class='label'><span style='color:red;'>*</span>服务端口</label>
                    <input id='ext6' name='ext6' type='text' placeholder='服务端口' type='number' class='input'>
                </div>
                <div class='input_wrap' style='display: none;' id='ext8_wrap'>
                    <label class='label'><span style='color:red;'>*</span>请求参数</label>
                    <input id='ext8' name='ext8' type='text' placeholder='[可选]服务参数，如：api_key=xxx' type='number'
                        class='input'>
                </div>

                <div class='input_wrap' id='api_key_wrap'>
                    <label class='label'><span style='color:red;'>*</span>api_key</label>
                    <input id='api_key' name='api_key' type='text' placeholder='开放平台秘钥' class='input'>
                </div>

                <div class='desc'> 秘钥位于：开放平台 -> 超体页面 -> 左下角 </div>
            </div>

        </div>

        <div class='block'>
            <div id='title'>
                电量检测配置
                <span style='float: right;font-size: 12px;font-weight: 500;'>高级配置 [选填]</span>
            </div>
            <div class='inputs-container'>
                <div class='input_wrap' id='kwh_enable_wrap'>
                    <label class='label'>启用电量检测</label>
                    <input type='checkbox' id='kwh_enable' checked="true">
                </div> 
                <div class='desc'> ps: 如果你的开发板没有电压检测模块，请关闭这个功能。 </div>
            </div>
        </div>

        <div class='block'>
            <div id='title'>
                音量控制配置
                <span style='float: right;font-size: 12px;font-weight: 500;'>高级配置 [选填]</span>
            </div>
            <div class='inputs-container'>
                <div class='input_wrap' id='volume_enable_wrap'>
                    <label class='label'>启用音量控制</label>
                    <input type='checkbox' id='volume_enable'>
                </div>
                <div class='input_wrap' id='volume_pin_wrap'>
                    <label class='label'>电位器输入引脚</label>
                    <input id='volume_pin' name='volume_pin' placeholder='默认 7' value='7' type='number' class='input'>
                </div>
                <div class='desc'> ps: 使用 10K 电位器，注意：没有插电位器一定不要启用 </div>
            </div>
        </div>

        <div class='block'>
            <div id='title'>
                指示灯配置
                <span style='float: right;font-size: 12px;font-weight: 500;'>高级配置 [选填]</span>
            </div>
            <div class='inputs-container'>
                <div class='input_wrap' id='lights_data_wrap'>
                    <label class='label'>WS2812 引脚</label>
                    <input id='lights_data' name='lights_data' placeholder='默认 18' value='18' type='number'
                        class='input'>
                </div>
                <div class='desc'> 普通 esp32s3 开发板请设置为： 48 </div>
            </div>
        </div>

        <div class='block'>
            <div id='title'>
                OLED屏幕配置
                <span style='float: right;font-size: 12px;font-weight: 500;'>高级配置 [选填]</span>
            </div>
            <div class='inputs-container'>
                <div class='input_wrap' id='oled_type_wrap'>
                    <label class='label'>屏幕类型</label>
                    <select id='oled_type' name='oled_type' style='background: #fff;padding-right: 20px;'
                        placeholder='请选择屏幕' value='096' class='input'>
                        <option value='096' id='096'>0.96寸（方形）</option> 
                    </select>
                </div>
                <div class='input_wrap' id='oled_sck_wrap'>
                    <label class='label'>SCK/SCL引脚</label>
                    <input id='oled_sck' name='oled_sck' placeholder='请输入...' value='38' type='number' class='input'>
                </div>
                <div class='input_wrap' id='oled_sda_wrap'>
                    <label class='label'>SDA引脚</label>
                    <input id='oled_sda' name='oled_sda' placeholder='请输入...' value='39' type='number' class='input'>
                </div>
                <div class='desc'>屏幕正极一定要使用 3.3v 哦 </div>
            </div>
        </div>

        <div class='block'>
            <div id='title'>
                静默时间配置(毫秒)
                <span style='float: right;font-size: 12px;font-weight: 500;'>高级配置 [选填]</span>
            </div>
            <div class='inputs-container'>
                <div class='input_wrap' id='vad_first_wrap'>
                    <label class='label'>首次静默时间</label>
                    <input id='vad_first' name='vad_first' placeholder='建议 5000 (也就是5秒)' value='5000' type='number'
                        class='input'>
                </div>
                <div class='input_wrap' id='vad_course_wrap'>
                    <label class='label'>说话后静默时间</label>
                    <input id='vad_course' name='vad_course' placeholder='建议 500 (也就是0.5秒)' value='500' type='number'
                        class='input'>
                </div>
                <div class='desc'> ps: 按住对话方式设置静默时间无意义 </div>
            </div>
        </div>


        <div class='block'>
            <div id='title'>
                麦克风引脚配置
                <span style='float: right;font-size: 12px;font-weight: 500;'>高级配置 [选填]</span>
            </div>
            <div class='inputs-container'>
                <div class='input_wrap' id='mic_bck_wrap'>
                    <label class='label'>SCK</label>
                    <input id='mic_bck' name='mic_bck' placeholder='请输入...' value='4' type='number' class='input'>
                </div>
                <div class='input_wrap' id='mic_ws_wrap'>
                    <label class='label'>WS</label>
                    <input id='mic_ws' name='mic_ws' placeholder='请输入...' value='5' type='number' class='input'>
                </div>
                <div class='input_wrap' id='mic_data_wrap'>
                    <label class='label'>SD</label>
                    <input id='mic_data' name='mic_data' placeholder='请输入...' value='6' type='number' class='input'>
                </div>
                <div class='desc'> ps: 除非你知道你在做什么，否则不要动这里的参数！ </div>
            </div>
        </div>

        <div class='block'>
            <div id='title'>
                扬声器引脚配置
                <span style='float: right;font-size: 12px;font-weight: 500;'>高级配置 [选填]</span>
            </div>
            <div class='inputs-container'>
                <div class='input_wrap' id='speaker_data_wrap'>
                    <label class='label'>DIN</label>
                    <input id='speaker_data' name='speaker_data' placeholder='请输入...' value='15' type='number'
                        class='input'>
                </div>
                <div class='input_wrap' id='speaker_bck_wrap'>
                    <label class='label'>BCLK</label>
                    <input id='speaker_bck' name='speaker_bck' placeholder='请输入...' value='16' type='number'
                        class='input'>
                </div>
                <div class='input_wrap' id='speaker_ws_wrap'>
                    <label class='label'>LRC</label>
                    <input id='speaker_ws' name='speaker_ws' placeholder='请输入...' value='17' type='number'
                        class='input'>
                </div>
                <div class='desc'> ps: 除非你知道你在做什么，否则不要动这里的参数！ </div>
            </div>
        </div>



        <div
            style='width: 100%;text-align: right; padding: 12px;box-sizing: border-box;border-top: 1px solid aliceblue;position: absolute;bottom: 0px;left: 0px;right: 0px;background-color: #fff;box-shadow: 0px 0px 8px #ccc;'>
            <button id='submit-btn'
                style='width:100% ; box-sizing: border-box; border-radius: 8px; padding: 12px 0px;border: none;color: #fff;background-color: transparent; color:#fff; background: linear-gradient(45deg, #40cea5, #09aba2); box-shadow: 0px 0px 3px #ccc;letter-spacing: 2px;'>保存</button>
        </div>
        <div
            style='width:100%;padding-left: 24px;padding-top: 12px; font-size: 12px;color: #777;text-align: left;line-height: 22px;'>
            密钥获取方式：<br />
            1. 打开开放平台网址 <br />
            2. 创建超体并配置好各项服务 <br />
            3. 找到超体页面左下角 api key 内容，点击复制即可
        </div>

        <div style='font-size: 13px;color:#929292;padding-top: 24px;text-align: center;padding-bottom: 100px;'>
            设备编码：<span id='device_id'> - </span>
        </div>
    </div>

    <div id='msg'>
        <div id='msg_content'>
            <div id='msg_content_text'> </div>
            <div id='msg_content_btns' onclick='closeMsg()'>确定</div>
        </div>
    </div>
    <div id='loading'>
        <div class='icon'></div>
    </div>
</body>

</html>

<script>
    try {
        var domain = '';
        var scanTimer = null;

        var loading = false;
        function myFetch(apiName, params, cb, method = 'GET') {
            var url = domain + apiName;
            var options = {
                method: method,
                mode: 'cors',
                headers: {
                    'Content-Type': 'application/json'
                }
            };
            if (method.toUpperCase() === 'POST' || method.toUpperCase() === 'PUT') {
                options.body = JSON.stringify(params);
            }
            fetch(url, options)
                .then(function (res) { return res.json() })
                .then(function (data) {
                    cb && cb(data);
                });
        };

        function msg(text) {
            document.querySelector('#msg').style.display = 'block';
            document.querySelector('#msg_content_text').innerHTML = text;
        }
        function closeMsg(text) {
            document.querySelector('#msg_content_text').innerHTML = '';
            document.querySelector('#msg').style.display = 'none';
        }

        function get_config() {
            myFetch('/get_config', {}, function (res) {
                console.log('wifi信息：', res);
                document.querySelector('#loading').style.display = 'none';
                if (res.success) {
                    var data = res.data;
                    if (data.wifi_name) {
                        document.querySelector('#ssid').value = data.wifi_name;
                    }
                    if (data.wifi_pwd) {
                        document.querySelector('#password').value = data.wifi_pwd;
                    }
                    if (data.ext1) {
                        document.querySelector('#api_key').value = data.ext1;
                    } else {
                        if (data.ext4 === 'http' && data.ext5) {
                            document.querySelector('#diy-server').click();
                        }
                    }
                    if (data.device_id) {
                        document.querySelector('#device_id').innerHTML = data.device_id;
                    }


                    if (data.ext4) {
                        document.querySelector('#ext4').value = data.ext4;
                    }
                    if (data.ext5) {
                        document.querySelector('#ext5').value = data.ext5;
                    }
                    if (data.ext6) {
                        document.querySelector('#ext6').value = data.ext6;
                    }

                    if (data.ext7) {
                        document.querySelector('#wakeup-type').value = data.ext7;
                    }
                    if (data.diyServerParams) {
                        document.querySelector('#ext8').value = data.diyServerParams;
                    }

                    if (data.volume_enable) {
                        document.querySelector('#volume_enable').checked = data.volume_enable === '1' ? true : false;
                    }

                    if (data.kwh_enable) {
                        document.querySelector('#kwh_enable').checked = data.kwh_enable === '1' ? true : false;
                    }
                    
                    if (data.volume_pin) {
                        document.querySelector('#volume_pin').value = data.volume_pin;
                    }
                    if (data.vad_first) {
                        document.querySelector('#vad_first').value = data.vad_first;
                    }
                    if (data.vad_course) {
                        document.querySelector('#vad_course').value = data.vad_course;
                    }
                    if (data.mic_bck) {
                        document.querySelector('#mic_bck').value = data.mic_bck;
                    }
                    if (data.mic_ws) {
                        document.querySelector('#mic_ws').value = data.mic_ws;
                    }
                    if (data.mic_data) {
                        document.querySelector('#mic_data').value = data.mic_data;
                    }
                    if (data.speaker_bck) {
                        document.querySelector('#speaker_bck').value = data.speaker_bck;
                    }
                    if (data.speaker_ws) {
                        document.querySelector('#speaker_ws').value = data.speaker_ws;
                    }
                    if (data.speaker_data) {
                        document.querySelector('#speaker_data').value = data.speaker_data;
                    }
                    if (data.lights_data) {
                        document.querySelector('#lights_data').value = data.lights_data;
                    }
                    if (data.oled_type) {
                        document.querySelector('#oled_type').value = data.oled_type;
                    }
                    if (data.oled_sck) {
                        document.querySelector('#oled_sck').value = data.oled_sck;
                    }
                    if (data.oled_sda) {
                        document.querySelector('#oled_sda').value = data.oled_sda;
                    }

                } else {
                    msg('请刷新页面重试');
                }
            });
        }

        function scan_ssids() {
            myFetch('/get_ssids', {}, function (res) {
                if (res.success) {
                    if (res.status === 'scaning' || !res.data) {
                        setTimeout(scan_ssids, 1000);
                        return;
                    }
                    var data = res.data || [];
                    if (data.length > 0) {
                        document.querySelector('#scan-ing').innerHTML = '请选择wifi...';
                    };
                    var selectDom = document.getElementById('ssid');
                    var options = selectDom.getElementsByTagName('option') || [];
                    var optionsName = [];
                    for (var i = 0; i < options.length; i++) {
                        optionsName.push(options[i].getAttribute('value'));
                    };
                    data.forEach(function (item) {
                        if (item.ssid && !optionsName.includes(item.ssid)) {
                            var option = document.createElement('option');
                            option.innerText = item.ssid + '     (' + (item.channel <= 14 ? '2.4GHz' : '5GHz') + ' 信道：' + item.channel + ')';
                            option.setAttribute('value', item.ssid);
                            selectDom.appendChild(option);
                        }
                    });
                    if (data.length) {
                        get_config();
                        clearInterval(scan_time);
                    };
                } else {
                    msg('启动网络扫描失败，请刷新页面重试');
                }
            });
        }

        function setWifiInfo() {
            if (loading) {
                return;
            };
            var wifi_name = document.querySelector('#ssid').value;
            var wifi_pwd = document.querySelector('#password').value;
            var api_key = document.querySelector('#api_key').value;
            var ext4 = document.querySelector('#ext4').value || '';
            var ext5 = document.querySelector('#ext5').value || '';
            var ext6 = document.querySelector('#ext6').value;
            var ext8 = document.querySelector('#ext8').value;
            var kwh_enable = document.querySelector('#kwh_enable').checked;
            var volume_enable = document.querySelector('#volume_enable').checked; 
            var volume_pin = document.querySelector('#volume_pin').value;
            var vad_first = document.querySelector('#vad_first').value;
            var vad_course = document.querySelector('#vad_course').value;
            var mic_bck = document.querySelector('#mic_bck').value;
            var mic_ws = document.querySelector('#mic_ws').value;
            var mic_data = document.querySelector('#mic_data').value;
            var speaker_bck = document.querySelector('#speaker_bck').value;
            var speaker_ws = document.querySelector('#speaker_ws').value;
            var speaker_data = document.querySelector('#speaker_data').value;
            var lights_data = document.querySelector('#lights_data').value;
            var oled_type = document.querySelector('#oled_type').value;
            var oled_sck = document.querySelector('#oled_sck').value;
            var oled_sda = document.querySelector('#oled_sda').value;

            var wakeupType = document.querySelector('#wakeup-type').value;
            if (!wifi_name) {
                msg('请输入 WIFI 账号哦~');
                return;
            }
            if (!wifi_pwd) {
                msg('请输入 WIFI 密码哦~');
                return;
            }

            if (document.querySelector('#api_key_wrap').style.display === 'none') {
                if (!ext5) {
                    msg('请输入服务地址哦~');
                    return;
                }
                if (!ext6) {
                    msg('请输入服务端口哦~');
                    return;
                }
            } else {
                if (!api_key) {
                    msg('请输入密钥哦~');
                    return;
                }
            }


            loading = true;
            document.querySelector('#submit-btn').innerHTML = '配网中...';
            clearTimeout(window.reloadTimer);
            window.reloadTimer = setTimeout(function () {
                msg('未知配网状态，即将重启设备');
                setTimeout(function () {
                    window.location.reload();
                }, 2000);
            }, 20000);

            myFetch('/set_config', {
                wifi_name: wifi_name,
                wifi_pwd: wifi_pwd,
                ext1: api_key,
                ext4: ext4,
                ext5: ext5,
                ext6: ext6,
                ext7: wakeupType,
                diyServerParams: ext8, 
                kwh_enable: kwh_enable ? '1' : '0',
                volume_enable: volume_enable ? '1' : '0',
                volume_pin: volume_pin,
                vad_first: vad_first,
                vad_course: vad_course,
                mic_bck: mic_bck,
                mic_ws: mic_ws,
                mic_data: mic_data,
                speaker_bck: speaker_bck,
                speaker_ws: speaker_ws,
                speaker_data: speaker_data,
                lights_data: lights_data,
                oled_type: oled_type,
                oled_sck: oled_sck,
                oled_sda: oled_sda
            }, function (res) {
                clearTimeout(window.reloadTimer);
                loading = false;
                document.querySelector('#submit-btn').innerHTML = '保存';
                if (res.success) {
                    msg(res.message);
                    window.close();
                } else {
                    msg(res.message);
                }
            }, 'POST');

        };


        function useDiyServer() {
            document.querySelector('#api_key_wrap').style.display = 'none';
            document.querySelector('#api_key').value = '';
            document.querySelector('#ext4').value = 'http';
            document.querySelector('#ext5').value = '';
            document.querySelector('#ext6').value = '8088';

            document.querySelector('#ext4_wrap').style.display = 'block';
            document.querySelector('#ext5_wrap').style.display = 'block';
            document.querySelector('#ext6_wrap').style.display = 'block';
            document.querySelector('#ext8_wrap').style.display = 'block';
        }

        function useEspAIServer() {
            document.querySelector('#ext4_wrap').style.display = 'none';
            document.querySelector('#ext5_wrap').style.display = 'none';
            document.querySelector('#ext6_wrap').style.display = 'none';
            document.querySelector('#ext8_wrap').style.display = 'none';
            document.querySelector('#ext4').value = '';
            document.querySelector('#ext5').value = '';
            document.querySelector('#ext6').value = '';
            document.querySelector('#ext8').value = '';


            document.querySelector('#api_key_wrap').style.display = 'block';
        }

        function pcbChange() {
            var pcbType = document.querySelector('#pcb-type').value;
            if (pcbType === 'esp-ai-s3') {
                document.querySelector('#lights_data').value = '18';

                document.querySelector('#oled_sck').value = '38';
                document.querySelector('#oled_sda').value = '39';

                document.querySelector('#mic_bck').value = '4';
                document.querySelector('#mic_ws').value = '5';
                document.querySelector('#mic_data').value = '6';
 
                document.querySelector('#speaker_data').value = '15';
                document.querySelector('#speaker_bck').value = '16';
                document.querySelector('#speaker_ws').value = '17';
            } else if (pcbType === 'xiao-zhi-s3') { 
                document.querySelector('#lights_data').value = '48';

                document.querySelector('#oled_sck').value = '42';
                document.querySelector('#oled_sda').value = '41';

                document.querySelector('#mic_bck').value = '5';
                document.querySelector('#mic_ws').value = '4';
                document.querySelector('#mic_data').value = '6';

                document.querySelector('#speaker_data').value = '7';
                document.querySelector('#speaker_bck').value = '15';
                document.querySelector('#speaker_ws').value = '16';
            }
        }




        window.onload = function () {
            scan_ssids();
            document.querySelector('#submit-btn').addEventListener('click', setWifiInfo);
        }
    } catch (err) {
        alert('页面遇到了错误，请刷新重试：' + err);
        msg('页面遇到了错误，请刷新重试：' + err);
    }

</script>
)rawliteral";
#endif

// 情绪检测
void onEmotion(String _emotion) {
  emotion = _emotion;
  // Serial.print("情绪： ");
  // Serial.println(emotion);
}

void setup() {
  Serial.begin(115200);
  delay(500);


  // 配置 ADC 宽度和通道
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC1_CHANNEL_8, ADC_ATTEN_DB_0);  // 假设使用通道 0
  // 获取校准值
  adc_chars = (esp_adc_cal_characteristics_t *)malloc(sizeof(esp_adc_cal_characteristics_t));
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_11db, ADC_WIDTH_BIT_12, ADC_ATTEN_DB_0, adc_chars);

  String oled_type = esp_ai.getLocalData("oled_type");
  String oled_sck = esp_ai.getLocalData("oled_sck");
  String oled_sda = esp_ai.getLocalData("oled_sda");

  if (oled_sck == "") {
    oled_sck = "38";
  }
  if (oled_sda == "") {
    oled_sda = "39";
  }



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

  if (kwh_enable == "") {
    kwh_enable = "1";
  }


  Serial.println(F("==============================="));
  Serial.println("\n当前设备版本:" + _version);
  if (ext4 != "" && ext5 != "") {
    Serial.println(F("你配置了服务IP, 所以将会请求你的服务！如果想要请求开放平台，请打开配网页面更改。"));
    Serial.print("服务协议：");
    Serial.println(ext4);
    Serial.print("服务IP：");
    Serial.println(ext5);
    Serial.print("服务端口：");
    Serial.println(ext6);
  }
  Serial.println(F("==============================="));

  // [必  填] 是否调试模式， 会输出更多信息
  bool debug = true;
  // [必  填] wifi 配置： { wifi 账号， wifi 密码, "热点名字" } 可不设置，连不上wifi时会打开热点：ESP-AI，连接wifi后打开地址： 192.168.4.1 进行配网(控制台会输出地址，或者在ap回调中也能拿到信息)
  // ESP_AI_wifi_config wifi_config = { "", "", "ESP-AI" };

  // ESP_AI_wifi_config wifi_config = { "", "", "ESP-AI" };
  ESP_AI_wifi_config wifi_config = { "", "", "ESP-AI", html_str };
  // ESP_AI_wifi_config wifi_config = { "oldang", "oldwang520", "ESP-AI" };

  // [必  填] 唤醒方案： { 方案, 语音唤醒用的阈值(本方案忽略即可), 引脚唤醒方案(本方案忽略), 发送的字符串 }
  ESP_AI_wake_up_config wake_up_config = {};
  if (ext7 == "pin_high") {
    wake_up_config.pin = 10;
    strcpy(wake_up_config.wake_up_scheme, "pin_high");  // 唤醒方案
  } else if (ext7 == "pin_low") {
    wake_up_config.pin = 10;
    strcpy(wake_up_config.wake_up_scheme, "pin_low");  // 唤醒方案
  } else if (ext7 == "boot") {
    wake_up_config.pin = 0;
    strcpy(wake_up_config.wake_up_scheme, "pin_low");  // 唤醒方案
  } else if (ext7 == "pin_high_listen") {
    wake_up_config.pin = 10;
    strcpy(wake_up_config.wake_up_scheme, "pin_high_listen");  // 唤醒方案
  } else if (ext7 == "boot_listen") {
    wake_up_config.pin = 0;
    strcpy(wake_up_config.wake_up_scheme, "pin_low_listen");  // 唤醒方案
  } else if (ext7 == "edge_impulse") {
    wake_up_config.threshold = 0.9;
    strcpy(wake_up_config.wake_up_scheme, "edge_impulse");  // 唤醒方案
  } else {
    // 默认用天问
    strcpy(wake_up_config.wake_up_scheme, "asrpro");  // 唤醒方案
    strcpy(wake_up_config.str, "start");              // 串口和天问asrpro 唤醒时需要配置的字符串，也就是从另一个开发版发送来的字符串
  }

  if (vad_first != "") {
    wake_up_config.vad_first = vad_first.toInt();
  }
  if (vad_course != "") {
    wake_up_config.vad_course = vad_course.toInt();
  }

  // ESP_AI_volume_config volume_config = { 7, 4096, 0.8, false };
  ESP_AI_volume_config volume_config = { volume_pin.toInt(), 4096, 0.8, volume_enable == "0" ? false : true };
  ESP_AI_i2s_config_mic i2s_config_mic = { mic_bck.toInt(), mic_ws.toInt(), mic_data.toInt() };
  ESP_AI_i2s_config_speaker i2s_config_speaker = { speaker_bck.toInt(), speaker_ws.toInt(), speaker_data.toInt() };
  ESP_AI_reset_btn_config reset_btn_config = {};
  ESP_AI_lights_config lights_config = { 18 };  // esp32s3 开发板是 48
  if (lights_data != "") {
    lights_config.pin = lights_data.toInt();
  }

  if (ext4 != "" && ext5 != "") {
    strcpy(server_config.protocol, ext4.c_str());
    strcpy(server_config.ip, ext5.c_str());
    strcpy(server_config.params, ext8.c_str());
    server_config.port = ext6.toInt();
  }

  // 错误监听, 需要放到 begin 前面，否则可能监听不到一些数据
  esp_ai.onBegin(onBegin);
  // 设备局域网IP
  esp_ai.onConnectedWifi(on_connected_wifi);
  // 设备网络状态监听, 需要放到 begin 前面，否则可能监听不到一些数据
  esp_ai.onNetStatus(on_net_status);
  // 上报位置, 需要放到 begin 前面
  esp_ai.onPosition(on_position);
  // 错误监听, 需要放到 begin 前面，否则可能监听不到一些数据
  esp_ai.onError(onError);

  esp_ai.begin({ debug, wifi_config, server_config, wake_up_config, volume_config, i2s_config_mic,
                 i2s_config_speaker, reset_btn_config, lights_config });
  // 用户指令监听
  esp_ai.onEvent(on_command);
  esp_ai.onSessionStatus(on_session_status);
  // 绑定设备
  esp_ai.onBindDevice(on_bind_device);
  // 情绪检测
  esp_ai.onEmotion(onEmotion);

  device_id = esp_ai.getLocalData("device_id");
  // 恢复音量
  String loc_volume = esp_ai.getLocalData("ext2");
  if (loc_volume != "") {
    volume = loc_volume.toFloat();
    Serial.print("本地音量：");
    Serial.println(volume);
    esp_ai.setVolume(volume);
  }

  // boot 按钮唤醒方式 esp-ai 库有问题，这begin()后再设置下就好了
  if (ext7 == "boot_listen" || ext7 == "boot") {
    pinMode(0, INPUT_PULLUP);
  }


  if (ext4 != "" && ext5 != "") {
    return;
  }

  // 情绪灯光任务
  // xTaskCreate(emotionTask, "emotionTask", 1024 * 30, NULL, 1, NULL);

  // 渲染文字的任务
  // xTaskCreate(drawStrAniFn, "TaskFunction_t", 1024 * 4, NULL, 1, NULL);

  // 连接业务服务器
  webScoket_yw_main();

  // Initialize TFT screen
  tft.begin(); 
  tft.setTextColor(0xFFFF, TFT_TRANSPARENT);  // 透明背景
  tft.fillScreen(TFT_BLACK);                  // Fill screen with black
  int width_ = tft.width();
  int height_ = tft.height();
  // Serial.println("屏幕宽高: ");
  // Serial.println(width_);
  // Serial.println(height_);
  sprite.createSprite(width_, height_);   
  u8g2 = new U8g2_for_TFT_eSPI();           // create u8g2 procedures
  u8g2->begin(sprite);                      // connect u8g2 procedures to TFT_eSPI
  // u8g2->setBackgroundColor(0x0000);         // 黑色 
  u8g2->setFont(u8g2_font_wqy12_t_gb2312);  // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall  
  u8g2->setDrawColor(0x6D9A);  // 白色

  // sprite2

  // Set up the JPEG decoder with the callback function
  TJpgDec.setCallback(tft_output);
  // The jpeg image can be scaled by a factor of 1, 2, 4, or 8
  TJpgDec.setJpgScale(1);
  // The byte order can be swapped (set true for TFT_eSPI)
  TJpgDec.setSwapBytes(true);


  // 遍历并打印所有情感与图片的映射
  for (int i = 0; i < sizeof(emotionToImage) / sizeof(emotionToImage[0]); i++) {
    // Serial.print("缓存图片: ");
    // Serial.println(emotionToImage[i].imageURL);
    renderImageFromURL(emotionToImage[i].imageURL.c_str(), true);
  }

  // 显示默认图片
  renderImageFromURL(emotionToImage[0].imageURL.c_str(), false);
}



long last_log_time = 0;
void loop() {
  // test...
  // if (millis() - last_log_time > 5000) {
  //   last_log_time = millis();
  //   Serial.print("===> 可用内存: ");
  //   Serial.print(ESP.getFreeHeap() / 1024);
  //   Serial.println("KB");
  // }


  if (start_update_ed == false) {
    esp_ai.loop();
  }
  webSocket_yw.loop();


#if !defined(IS_ESP_AI_S3_NO_SCREEN)
  if (volume < 0.3) {
    // 绘制静音图标
    // face->SetIsSilent(true);
  } else {
    // face->SetIsSilent(false);
  }
#endif
  // if (is_yw_connected) {
  //   face->Update();
  // }


  long cur_time = millis();
  if (is_update_progress == false) {
    if (start_update_ed == true && ((cur_time - start_update_time) > 20000)) {
      // 10 没有反应就说明升级失败了
      Serial.println("OTA 升级失败");
      start_update_ed = false;
      String device_id = esp_ai.getLocalData("device_id");
      // 将错误发往业务服务
      JSONVar ota_update_error;
      ota_update_error["type"] = "ota_update_error";
      ota_update_error["device_id"] = device_id;
      String sendData = JSON.stringify(ota_update_error);
      webSocket_yw.sendTXT(sendData);
    }
  }


  if (start_update_ed == false) {
    // 上报剩余电量
    if ((cur_time - prevReadTime) > readBatteryMonitorTime) {
      prevReadTime = millis();
#if defined(ARDUINO_XIAO_ESP32S3)
      // 先不处理...
#elif defined(ARDUINO_ESP32S3_DEV)
      uploadBattery();
#else
      // 先不处理...
#endif
    }
  }



  // 表情图片
  if (prve_emotion != emotion) {

    if (emotion == "无情绪") {
      if ((millis() - prve_emotion_time) > 6000) {
        prve_emotion_time = millis();
        prve_emotion = emotion;
        renderImageFromURL(emotionToImage[0].imageURL.c_str(), false);
      }
    } else {
      // 避免重复渲染表情
      if ((millis() - prve_emotion_time) > 3000) {
        prve_emotion_time = millis();
        prve_emotion = emotion;

        // 遍历映射数组，查找情绪对应的图片 URL
        for (int i = 0; i < sizeof(emotionToImage) / sizeof(emotionToImage[0]); i++) {
          if (String(emotionToImage[i].emotion) == emotion) {
            renderImageFromURL(emotionToImage[i].imageURL.c_str(), false);
            break;
          }
        }
      }
    }
  }

  // 文字渲染
  drawStrAniFn();
  drawStrTL();
}




// This function will be called during decoding of the jpeg file to render each block to the TFT
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
  if (y >= tft.height()) return 0;  // Prevent drawing beyond screen

  // This function automatically clips the image rendering at the TFT boundaries
  tft.pushImage(x, y, w, h, bitmap);

  // Return 1 to decode the next block
  return 1;
}


void cacheImageToPSRAM(const String &url, uint8_t *buffer, int len) {
  // 如果已存在，先释放旧缓存
  if (imageCacheMap.count(url)) {
    free(imageCacheMap[url].buffer);
    imageCacheMap.erase(url);
    imageCacheOrder.erase(std::remove(imageCacheOrder.begin(), imageCacheOrder.end(), url), imageCacheOrder.end());
  }

  // 超过最大缓存数，释放最旧的缓存
  if (imageCacheOrder.size() >= MAX_CACHE_IMAGES) {
    String oldest = imageCacheOrder.front();
    imageCacheOrder.erase(imageCacheOrder.begin());
    if (imageCacheMap.count(oldest)) {
      free(imageCacheMap[oldest].buffer);
      imageCacheMap.erase(oldest);
    }
  }

  // 加入新缓存
  ImageCache img = { buffer, len };
  imageCacheMap[url] = img;
  imageCacheOrder.push_back(url);
}


// @param image_url   图片地址，建议 http
// @param only_cache  仅仅只是做缓存
void renderImageFromURL(const char *image_url, bool only_cache) {

  int width_ = tft.width();
  int height_ = tft.height();
  tft.fillRect(0, 0, width_, height_, TFT_BLACK);  // 清屏背景
  String url = String(image_url);

  // 缓存命中
  if (imageCacheMap.count(url)) {
    if (only_cache) {
      return;
    }
    ImageCache &img = imageCacheMap[url];
    uint16_t w = 0, h = 0;
    TJpgDec.getJpgSize(&w, &h, img.buffer, img.len);
    TJpgDec.drawJpg(0, 0, img.buffer, img.len);
    return;
  }

  // 网络请求
  HTTPClient http;
  http.begin(image_url);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    int len = http.getSize();

    // 分配 PSRAM 缓冲区
    uint8_t *buffer = (uint8_t *)heap_caps_malloc(len, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!buffer) {
      Serial.println("PSRAM 分配失败！");
      http.end();
      return;
    }

    WiFiClient *stream = http.getStreamPtr();
    int bytesRead = 0;
    while (stream->available() && bytesRead < len) {
      buffer[bytesRead++] = stream->read();
    }

    // 缓存
    cacheImageToPSRAM(url, buffer, len);

    if (only_cache == false) {
      // 渲染
      uint16_t w = 0, h = 0;
      TJpgDec.getJpgSize(&w, &h, buffer, len);
      TJpgDec.drawJpg(0, 0, buffer, len);
      // 不释放 buffer，缓存保留
    }
  } else {
    Serial.printf("HTTP GET 失败：%d\n", httpCode);
  }

  http.end();
}



// 渲染底部文字
int fontHeight = 30;
int spriteWidth = 0;
void drawStrAniFn() {
  if (spriteWidth == 0) {
    spriteWidth = tft.width();
  }
  if (bttomText != "") {
    // int scrollSpeed = map(bttomText.length(), 0, 50, 10, 3);  // 自动调节速度
    if (bttomTextPrevLeft > -3000) {  // 太大就没必要再播放了
      sprite.fillSprite(0x0000);
      // 一开始不滚动，等一会在滚动
      if (bttomTextPrevLeft < -80) {
        u8g2->setCursor(bttomTextPrevLeft + 50, fontHeight - 5);  // start writing at this position
      } else {
        u8g2->setCursor(0, fontHeight - 5);  // start writing at this position
      }
      u8g2->print(bttomText.c_str());
      sprite.pushSprite(0, 210);   
      bttomTextPrevLeft -= 15;
    }
  }
}


// 渲染左上角文字
void drawStrTL() {
  if (topLeftText != "") {
    if (prevTopLeftText != topLeftText) {
      prevTopLeftText = topLeftText;
      sprite.fillSprite(0x0000);
      u8g2->setCursor(0, 30);  // start writing at this position
      u8g2->print(topLeftText.c_str());  
      sprite.pushSprite(0, 0, 0, 0, 100, 50);  
    }
  }
}

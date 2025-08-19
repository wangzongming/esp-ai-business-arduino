#include <esp-ai.h>
#include <Wire.h>
#include <Battery.h>
#include "main.h"
#include "SimpleTimer.h"
#include <OneButton.h>
#include <Arduino.h>

#include "src/ota_manager.h"
#include "src/auto_update.h"

#if defined(ENABLE_EEUI)
#include <vector>
#include <map>
#include "eeui.h"

/**
 * 引入情绪
 * 快乐、伤心、愤怒、意外、专注、发愁、懊恼、困倦、疑问、恐惧、敬畏、肯定、否定
 * 建议gif图尺寸：160px * 160px  (建议不要超过100kb，否则c数组会很大)
 *
 * 打开 https://dev.espai.fun 我的应用/我的素材 将您的 Gif/JPG 专为 C 数组，然后替换u对应表情文件中的数组内容和长度内容e即可
 *
 * 图片颜色不对时，使用下面工具进行处理
 * 图片尺寸处理：https://ezgif.com/optimize/
 * 图片压缩处理：https://ezgif.com/optimize/
 * GIFu图制作：https://ezgif.com/maker
 */
#include "emos/wifi.h"
#include "emos/wx_qrcode.h"
#include "emos/ap_qrcode.h"
#include "emos/error.h"
#include "emos/listen.h"
#include "emos/sleep.h"
#include "emos/music.h"
#include "emos/tts_ing.h"
#include "emos/happy.h"
#include "emos/sad.h"
#include "emos/angry.h"
#include "emos/accident.h"
#include "emos/no.h"

TFT_eSPI emo_tft = TFT_eSPI();
EEUI eeui;

/**
 * 定义一个情感与图片 URL 的映射数组
 */
const EEUIEmotionImagePair emotions[] = {
	// 状态动画
	{"联网中", &wifi_img},
	{"请配网", &wx_qrcode_img}, // 小程序配网时，会自动在屏幕上显示二维码，微信扫码即可跳转配网
	{"请配网", &ap_qrcode_img}, // ap配网时，会自动在屏幕上显示二维码，微信扫码即可跳转配网
	{"发生错误", &error_img},
	{"聆听中", &listen_img},
	{"休息中", &sleep_img},
	{"唱歌中", &music_img},

	// 聊天表情动画(最好是说话的动作+表情)
	{"无情绪", &tts_ing_img}, // 这个表情必须配置
	{"快乐", &happy_img},
	{"伤心", &sad_img},
	{"愤怒", &angry_img},
	{"意外", &accident_img},
	{"否定", &no_img},

	// {"肯定", &kuai_le},
	// {"专注", &kuai_le},
	// {"发愁", &kuai_le},
	// {"懊恼", &kuai_le},
	// {"困倦", &kuai_le},
	// {"疑问", &kuai_le},
	// {"恐惧", &kuai_le},
	// {"敬畏", &kuai_le},
};

#endif

#if defined(ENABLE_OLED) || defined(ENABLE_TFT)
#include "Face.h"

#if defined(ENABLE_OLED)
#if SCREEN_TYPE == 0
#error "请在正确配置 SCREEN_TYPE 屏幕类型: libraries2/esp32-eyes/Common.h 打开文件修改"
#endif
#elif defined(ENABLE_TFT)
#if SCREEN_TYPE == 1
#error "请在正确配置 SCREEN_TYPE 屏幕类型: libraries2/esp32-eyes/Common.h 打开文件修改"
#endif
#endif
#endif

// ==================版本定义=========================
String _version = VERSION;

// ==================OTA 升级定义=========================
// 是否为官方固件， 如果是您自己的固件请改为 "0"
#if defined(IS_BOWKNOT) // 蝴蝶结为社区固件
String is_official = "0";
#else
String is_official = IS_OFFICIAL;
#endif

// 固件ID， 请正确填写，否则无法进行 OTA 升级。
// 获取方式： 我的固件 -> 固件ID
String BIN_ID = DEF_BIN_ID;

// ====================================================

// ==================全局对象===========================
#if defined(ENABLE_OLED) || defined(ENABLE_TFT)
// 屏幕显示对象
Face *face;
#endif

// 定时器对象
SimpleTimer timer;
#if defined(ENABLE_BTN_VOL)
// 按钮对象
OneButton button_vol_add(VOL_ADD_KEY, true);
OneButton button_vol_sub(VOL_SUB_KEY, true);
#endif
/*** 测试地址 ***/
// // 业务服务地址，用于将设备信息传递到用户页面上
// String domain = "http://192.168.3.16:7002/";
// // 业务服务 ws 服务端口ip
// String yw_ws_ip = "192.168.3.16";
// String yw_ws_path = "";
// int yw_ws_port = 7002;
// // ESP-AI 服务配置： { 服务IP， 服务端口, "[可选] 请求参数" }
// ESP_AI_server_config server_config = {"http", "192.168.3.16", 8088};

/*** 线上地址 ***/
String domain = "http://api.espai.fun/";
String yw_ws_ip = "api.espai.fun";
String yw_ws_path = "";
int yw_ws_port = 80;
// 这里不配置接口，配网页面也没有 api_key 配置，这会导致使用默认的服务节点：node.espai.fun
ESP_AI_server_config server_config = {"http", "node.espai.fun", 80};

ESP_AI esp_ai;
WebSocketsClient webSocket_yw;
bool is_yw_connected = false; // 业务服务是否已经连接

// 设备id
String device_id = "";
// 网络状态
String _net_status = "";
// 会话状态
String _session_status = "";
// 待显示LLM的文本
String llm_text = "";
SemaphoreHandle_t llm_text_mutex = xSemaphoreCreateMutex();
int processed_llm_text_index = 0; // 当前处理进度指针

// 设备局域网IP
String _device_ip = "";

// 是否启用电量检测
String kwh_enable = "";
// 是否已经显示过充电特效
bool charging_effected = false;
// 充电中
bool charging = false;
// 上次渲染中的图片
String prev_render_gif_by_name = "";
static bool music_gen_ing = false;
static bool play_music_ing = false;

// 实例化电源管理对象
#if defined(ENABLE_POWER)
Battery battery(3300, 4200, BAT_PIN, 12); // 电池电压范围 3.3v~4.2v
// 上报system的基本信息至服务器的时间间隔
long report_systeminfo_interval = 5 * 60 * 1000; // 5分钟上报一次电量
bool render_battery_ed = false;					 // 渲染过电量了
#endif

#if defined(ENABLE_EMO_LIGHT)
// 情绪灯光灯珠数量
int emotion_led_number = EMOTION_LIGHT_COUNT;
// 情绪灯光
Adafruit_NeoPixel pixelsEmotion(emotion_led_number, EMOTION_LIGHT_PIN, NEO_GRB + NEO_KHZ800);
#endif
// OTA 检测升级过了，用于应付重新连接服务后不要重复去进行提示 OTA 升级
bool ota_ed = false;

// ======================END=======================
#if defined(ENABLE_POWER)
// 上报设备电量的函数
void upload_systeminfo()
{
	if (kwh_enable == "0" || _session_status == "说话中")
	{
		return;
	}

	// ESP-AI 开发版需要做电量检测
	String ext4 = esp_ai.getLocalData("ext4");
	String ext5 = esp_ai.getLocalData("ext5");
	if (ext4 != "" && ext5 != "")
	{
		return;
	}
	int batteryLevel = battery.level(); // 获取电池电量百分比
	if (batteryLevel == 0)
	{
		// 这种情况一般是没有电量检测的设备
		return;
	}
	LOG_D("上报电量：%d", batteryLevel);

#if defined(ENABLE_EEUI)
	eeui.render_battery(batteryLevel); // 渲染电量
	render_battery_ed = true;
#endif

#if defined(ENABLE_OLED) || defined(ENABLE_TFT)

	if (battery.getischarge())
	{
#if !defined(ENABLE_SUPPLY_DETECTION)
		// 说明在充电
		LOG_D("设备充电中...");
		face->SetBatLevel(0xFF);
#endif
	}
	else
	{
		face->SetBatLevel(batteryLevel);
		render_battery_ed = true;
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
}
#endif

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
#if defined(ENABLE_OLED) || defined(ENABLE_TFT)
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
#if defined(ENABLE_OLED) || defined(ENABLE_TFT)
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
void onError(const String &code, const String &at_pos, const String &message)
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

	if (message.indexOf("设备已解绑") >= 0)
	{
		// 清除 api_key 重启板子
		esp_ai.setLocalData("wifi_name", "");
		esp_ai.setLocalData("wifi_pwd", "");
		esp_ai.setLocalData("ext1", "");
		ESP.restart();
	}
}

// ========================= 指令监听 =========================
String asr_text = "";
void on_command(const String &command_id, const String &data)
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
	else if (command_id == "music_gen_ing")
	{
		music_gen_ing = true;
#if defined(ENABLE_EEUI)
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		eeui.render_gif_by_name("无情绪");
		eeui.set_status_text("歌曲创作中", true, "");
#endif
	}
	else if (command_id == "play_music")
	{
		music_gen_ing = false;
		play_music_ing = true;
#if defined(ENABLE_EEUI)
		eeui.render_gif_by_name("唱歌中");
		eeui.set_status_text("唱歌中", true, "");
#endif
	}
#if defined(ENABLE_POWER)
	else if (command_id == "query_battery")
	{
		int batteryLevel = battery.level(); // 获取电池电量百分比
		LOG_D("查询电量");
		LOG_D("设备电量剩余: %d%", batteryLevel);
		esp_ai.stopSession();
		esp_ai.tts("设备电量剩余" + String(batteryLevel) + "%");
	}
#endif
	else if (command_id == "on_iat_cb")
	{
#if defined(ENABLE_OLED) || defined(ENABLE_TFT)
		face->ClearChatMessage();
		face->SetChatMessage(data);
#endif
#if defined(ENABLE_EEUI)
		asr_text = data;
		eeui.set_bottom_scrolling_text(asr_text.c_str());
#endif

		llm_text = "";
		processed_llm_text_index = 0;
	}
	else if (command_id == "on_llm_cb")
	{
#if defined(ENABLE_OLED) || defined(ENABLE_TFT)
		face->SetChatMessage(data);
#endif

#if defined(ENABLE_EEUI)
		if (play_music_ing)
		{
			llm_text = data;
			eeui.set_bottom_scrolling_text(llm_text.c_str());
		}
		else
		{
			if (xSemaphoreTake(llm_text_mutex, pdMS_TO_TICKS(10)) == pdTRUE)
			{
				llm_text += data;
				xSemaphoreGive(llm_text_mutex);
			}
		}

#endif
	}
}

// ========================= 会话状态监听 =========================
void on_session_status(const String &status)
{
	_session_status = status;
	if (otaManager->isUpdating())
	{
		return;
	}

	if (status == "wakeup")
	{
		if (xSemaphoreTake(llm_text_mutex, pdMS_TO_TICKS(10)) == pdTRUE)
		{
			play_music_ing = false;
			music_gen_ing = false;
			llm_text = "";
			processed_llm_text_index = 0;
			xSemaphoreGive(llm_text_mutex);
		}
	}

#if defined(ENABLE_OLED) || defined(ENABLE_TFT)
	// 处理会话状态
	if (status == "iat_start")
	{
		play_music_ing = false;
		music_gen_ing = false;
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

#if defined(ENABLE_EEUI)
	// 处理会话状态
	if (status == "iat_start")
	{
		music_gen_ing = false;
		prev_render_gif_by_name = "聆听中";
		eeui.render_gif_by_name("聆听中");
		eeui.set_status_text("聆听中", true, "");
	}
	else if (status == "iat_end")
	{
		prev_render_gif_by_name = "说话中";
		eeui.render_gif_by_name("说话中");
		eeui.set_status_text("说话中", true, "");
	}
	else if (status == "session_end")
	{
		if (ota_ed && !music_gen_ing)
		{
			prev_render_gif_by_name = "休息中";
			eeui.render_gif_by_name("休息中");
			eeui.set_status_text("休息中", true, "");
		}
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

int getSignalLevel()
{
	int rssi = WiFi.RSSI(); // 获取 RSSI 值（单位 dBm）

	// 映射 RSSI 到 1~3 的等级：
	if (rssi >= -60)
	{
		return 3; // 信号强
	}
	else if (rssi >= -75)
	{
		return 2; // 信号中等
	}
	else
	{
		return 1; // 信号弱
	}
}

// ========================= 网络状态监听 =========================
void on_net_status(const String &status)
{
	// LOG_D("网络状态: " + status);
	_net_status = status;

	if (status == "0_ing")
	{
#if defined(ENABLE_OLED) || defined(ENABLE_TFT)
		face->SetChatMessage("正在连接网络");
#endif
	};
	if (status == "0_ap")
	{

#if defined(ENABLE_EEUI)
#if defined(BLE_MODEL)
		eeui.render_gif_by_name("请配网");
		eeui.set_status_text("微信扫一扫配网", false, "bottom_center");
#else
		eeui.render_gif_by_name("请配网");
		eeui.set_status_text("浏览器扫一扫配网", false, "bottom_center");
#endif
#endif

#if defined(ENABLE_OLED) || defined(ENABLE_TFT)
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

#if defined(ENABLE_EEUI)
		eeui.set_status_text("正在连接服务", false, "bottom_center");
#endif

#if defined(ENABLE_OLED) || defined(ENABLE_TFT)
		face->SetChatMessage("正在连接服务");
#endif
	};

	// 已连接服务
	if (status == "3")
	{
#if defined(ENABLE_OLED) || defined(ENABLE_TFT)
		face->SetChatMessage("服务连接成功");
#endif

#if defined(ENABLE_EEUI)
		eeui.set_status_text("服务连接成功", false, "bottom_center");
#endif
	};
}

#if defined(ENABLE_POWER)
/**
 * 电量检测
 */
int batteryDetection()
{
	// ESP-AI 开发版需要做电量检测
	int batteryLevel = battery.level(); // 获取电池电量百分比
	LOG_D("[Info] -> 电池电量：%d", batteryLevel);
	if (batteryLevel == 0)
	{
		// 一般是不带充电检测的开发板
		return 100;
	}
	if (batteryLevel < 10)
	{
		while (true)
		{
			batteryLow();
		}

		return false;
	}
	return batteryLevel;
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
#if defined(ENABLE_POWER)
		LOG_D("%s", "电量不足");
		LOG_D("%s", "如果您的开发板没有电压检测模块，请在配网页面或者设备配置页面禁用电量检测功能。");
		play_builtin_audio(mei_dian_le, mei_dian_le_len);
#endif
#if defined(ENABLE_OLED) || defined(ENABLE_TFT)
		face->SetChatMessage("电量过低，请连接充电器。");
#endif
#if defined(ENABLE_EMO_LIGHT)
		pixelsEmotion.fill(pixelsEmotion.Color(255, 69, 0), 0, emotion_led_number);
		pixelsEmotion.show();
#endif
		vTaskDelay(10000 / portTICK_PERIOD_MS);
	}
}
#endif

bool onBegin()
{
#if !defined(ENABLE_POWER)
	return true;
#endif
	if (kwh_enable == "0")
		return true;

#if defined(ENABLE_POWER)
	batteryDetection();
#endif
	return true;
}

void on_ready()
{
	// 检测升级
	if (!ota_ed)
	{
#if defined(ENABLE_EEUI)
		eeui.set_status_text("检测升级中", false, "bottom_center");
		eeui.render_loading();
#endif

		auto_update(
			device_id,
			esp_ai.getLocalData("ext1"),
			BIN_ID,
			is_official,
			domain,
			_version,
			[](void)
			{ esp_ai.awaitPlayerDone(); }, // 等待播放器完成的函数
			[](const unsigned char *data, size_t len)
			{ esp_ai.playBuiltinAudio(data, len); }, // 播放音频的函数
			[](const String &text)
			{ esp_ai.tts(text); }, // TTS 函数
			*otaManager, [](const String &text, const String &status)
			{
#if defined(ENABLE_OLED) || defined(ENABLE_TFT)
				face->SetChatMessage(text);
#endif
#if defined(ENABLE_EEUI)
				eeui.set_status_text(text.c_str(), false, "bottom_center");
				if (status == "error")
				{
					prev_render_gif_by_name = "发生错误";
					eeui.render_gif_by_name("发生错误");
				}
				else if (status == "end")
				{
					eeui.hide_loading();

					// 初始化屏幕显示
					prev_render_gif_by_name = "休息中";
					eeui.render_gif_by_name("休息中");

#if defined(ENABLE_POWER)
					eeui.render_battery(battery.level()); // 渲染电量
#endif
					eeui.render_signal(getSignalLevel()); // 渲染信号
					eeui.render_volume(volume);			  // 音量设置

					vTaskDelay(1000 / portTICK_PERIOD_MS);
					eeui.set_status_text("休息中", true, "top_left");
				}
#endif 
					ota_ed = true; });
	}

	if (esp_ai.getLocalData("ext3") == "")
	{
		esp_ai.setLocalData("ext3", "1");
		esp_ai.awaitPlayerDone();
		esp_ai.stopSession();
		esp_ai.tts("设备激活成功，现在可以和我聊天了哦。");
		vTaskDelay(500 / portTICK_PERIOD_MS);
		esp_ai.awaitPlayerDone();
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
		esp_ai_pixels->setPixelColor(0, esp_ai_pixels->Color(218, 164, 90));

		if (esp_ai_pixels->getBrightness() != 100)
			esp_ai_pixels->setBrightness(10); // 亮度设置
		else
			esp_ai_pixels->setBrightness(10); // 亮度设置
		esp_ai_pixels->show();
		break;
	case WStype_CONNECTED:
	{
		LOG_D("√ 业务ws服务已连接");
		is_yw_connected = true;
#if defined(ENABLE_POWER)
		// 上报设备信息
		upload_systeminfo();
#endif
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
		return;

	LOG_D("开始连接业务 ws 服务");
	if (_net_status != "2" && _net_status != "3")
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
void websocket_timer2(int arg)
{
	webSocket_yw.loop();
}

// ========================= 绑定设备 =========================

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

	esp_ai_pixels->setBrightness(10); // 亮度设置
	esp_ai_pixels->show();

	LOG_D("[Info] api_key: %s", ext1.c_str());
	LOG_D("[Info] device_id: %s", device_id.c_str());
	LOG_D("[Info] wifi_name: %s", wifi_name.c_str());
	LOG_D("[Info] wifi_pwd: %s", wifi_pwd.c_str());
	LOG_D("[Info] ext4: %s", ext4.c_str());
	LOG_D("[Info] ext5: %s", ext5.c_str());

	// ext4 存在时是请求的自己的服务器，所以不去请求绑定服务
	if (ext4 != "" && ext5 != "")
	{
#if defined(ENABLE_OLED) || defined(ENABLE_TFT)
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

	esp_ai_pixels->setBrightness(100); // 亮度设置
	esp_ai_pixels->show();

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

#if defined(ENABLE_OLED) || defined(ENABLE_TFT)
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
#if defined(ENABLE_OLED) || defined(ENABLE_TFT)
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
#if defined(ENABLE_OLED) || defined(ENABLE_TFT)
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
#if defined(ENABLE_OLED) || defined(ENABLE_TFT)
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

void on_position(const String &ip, const String &nation, const String &province, const String &city, const String &latitude, const String &longitude)
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

void on_connected_wifi(const String &device_ip)
{
	_device_ip = device_ip;
}

#if defined(ENABLE_OLED) || defined(ENABLE_TFT)
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
#if defined(ENABLE_TFT)
			vTaskDelay(30 / portTICK_PERIOD_MS); // 多增加点时间
#endif
			vTaskDelay(20 / portTICK_PERIOD_MS);
		}
	}
	vTaskDelete(NULL);
}
#endif

#if defined(ENABLE_EMO_LIGHT)
long prve_emotion_time = 0;
static String prve_emotion = "无情绪";
void emotion_led_control(const String &emotion)
{
	// 快乐、伤心、愤怒、意外、专注、发愁、懊恼、困倦、疑问、恐惧、敬畏、肯定、否定
	// 情绪灯光  快乐->蓝色 / 伤心->绿色 / 愤怒->红色 / 恐惧->黄色
	if (prve_emotion != emotion)
	{
		pixelsEmotion.clear();
		if (emotion == "快乐" || emotion == "意外" || emotion == "专注")
		{
			pixelsEmotion.fill(pixelsEmotion.Color(0, 0, 139), 0, emotion_led_number);
		}
		else if (emotion == "伤心" || emotion == "发愁" || emotion == "懊恼" || emotion == "肯定")
		{
			pixelsEmotion.fill(pixelsEmotion.Color(0, 255, 0), 0, emotion_led_number);
		}
		else if (emotion == "愤怒" || emotion == "恐惧" || emotion == "敬畏" || emotion == "否定")
		{
			pixelsEmotion.fill(pixelsEmotion.Color(255, 0, 0), 0, emotion_led_number);
		}
		else
		{
			// 全部作为无情绪处理：默认无颜色
			// pixelsEmotion.fill(pixelsEmotion.Color(250, 138, 0), 0, emotion_led_number);
		}
		pixelsEmotion.show();
		prve_emotion = emotion;
	}
}
#endif

// 情绪检测
void onEmotion(const String &emotion)
{
#if defined(ENABLE_OLED) || defined(ENABLE_TFT)
	face->SetEmotion(emotion);
#endif

#if defined(ENABLE_EEUI)
	if (music_gen_ing)
		return;
	// 查找对应的表情, 需要自动恢复到无情绪
	for (size_t i = 0; i < sizeof(emotions) / sizeof(emotions[0]); ++i)
	{
		if (emotions[i].emotion_name == emotion)
		{
			prev_render_gif_by_name = emotion;
			eeui.render_gif_by_name(emotion.c_str());
			return;
		}
	}
	prev_render_gif_by_name = "无情绪";
	eeui.render_gif_by_name("无情绪");
#endif

#if defined(ENABLE_EMO_LIGHT)
	emotion_led_control(emotion);
#endif
}
#if defined(ENABLE_POWER)
void report_systeminfo_timer1(int arg)
{
	upload_systeminfo();
}
#endif

/**
 * OTA 升级失败检测，需要按需启动这个定时器
 */
void update_check_timer3(int arg)
{
	if (otaManager->updateFailed())
	{
		// 10 没有反应就说明升级失败了
		LOG_D("OTA 升级失败");
		// 将错误发往业务服务
		JSONVar ota_update_error;
		ota_update_error["type"] = "ota_update_error";
		ota_update_error["device_id"] = esp_ai.getLocalData("device_id");
		String sendData = JSON.stringify(ota_update_error);
		webSocket_yw.sendTXT(sendData);
	}
}

#if defined(ENABLE_BTN_VOL)
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

// 供电检测
#if defined(ENABLE_SUPPLY_DETECTION)
void espai_loop_timer_supply_det(int arg)
{
	if (otaManager->isUpdating() == false && _net_status == "3" && ota_ed && render_battery_ed)
	{
		if (digitalRead(SUPPLY_DETECTION_PIN) == 1)
		{
#if defined(ENABLE_OLED) || defined(ENABLE_TFT)
			face->SetBatLevel(0xFF);
#endif
			if (!charging_effected)
			{
				charging = true;
				charging_effected = true;

#if defined(ENABLE_EEUI)
				eeui.recharge(true);
				vTaskDelay(300 / portTICK_PERIOD_MS);
				eeui.render_gif_by_name(prev_render_gif_by_name.c_str());
#endif
			}
		}
		else
		{
			if (charging_effected)
			{
				upload_systeminfo();
				charging_effected = false;
				charging = false;
#if defined(ENABLE_EEUI)
				eeui.recharge(false);
#endif
			}
		}
	}
}
#endif

#if defined(IS_BOWKNOT)
/**
 * 蝴蝶结氛围灯 IO20
 *
 * 蝴蝶结按钮 IO2
 * 唤醒： >3400 < 3500
 * LED： >3800 < 3900
 * +音量： >3400 < 3500
 * -音量： >3400 < 3500
 */
long esp_ai_last_debounce_time = 0;
int esp_ai_debounce_delay = 80;
int esp_ai_prev_state = 0;

void espai_loop_bowknot(int arg)
{
	if (otaManager->isUpdating() == false)
	{
		int reading = analogRead(2);
		// Serial.println(reading);
		long curTime = millis();
		bool is_wakeup = (reading > 2700) && (reading < 3500);

		if (is_wakeup)
		{
			if ((curTime - esp_ai_last_debounce_time) > esp_ai_debounce_delay)
			{
				if (esp_ai_prev_state != 1)
				{
					esp_ai_last_debounce_time = curTime;
					esp_ai.wakeUp("wakeup");
					Serial.println(F("[Info] -> 按下了按钮唤醒"));
					esp_ai_prev_state = 1;
				}
			}
		}
		else
		{
			esp_ai_prev_state = 0;
		}
	}
}

#endif

#if defined(ENABLE_EEUI)

bool is_punctuation(const String &ch)
{
	// 歌曲等句子是用空格分隔的，所以这里也需要判断空格
	static const char *punctuations[] = {"，", "。", "！", "？", "；", "~", ",", ".", "!", "?", " "};
	for (int i = 0; i < sizeof(punctuations) / sizeof(punctuations[0]); i++)
	{
		if (ch == punctuations[i])
			return true;
	}
	return false;
}

int utf8_char_length(char firstByte)
{
	if ((firstByte & 0x80) == 0)
		return 1; // ASCII
	else if ((firstByte & 0xE0) == 0xC0)
		return 2; // 110xxxxx
	else if ((firstByte & 0xF0) == 0xE0)
		return 3; // 1110xxxx
	else if ((firstByte & 0xF8) == 0xF0)
		return 4; // 11110xxx
	return 1;	  // fallback
}

int countTextLength(const char *str)
{
	int count = 0;
	while (*str)
	{
		uint8_t c = *str;
		if ((c & 0x80) == 0x00)
		{
			// 单字节 ASCII（英文）
			str += 1;
		}
		else if ((c & 0xE0) == 0xC0)
		{
			// 两字节字符（如部分日文假名等）
			str += 2;
		}
		else if ((c & 0xF0) == 0xE0)
		{
			// 三字节字符（如中文、日文汉字、韩文等）
			str += 3;
		}
		else if ((c & 0xF8) == 0xF0)
		{
			// 四字节字符（表情、特殊符号等）
			str += 4;
		}
		else
		{
			// 错误编码，跳过
			str += 1;
		}
		count++;
	}
	return count;
}

// 等待时间
int eeui_await_punc_count = 0;
// 上一次渲染的时间
long eeui_prev_time = 0;
// 下一次可以渲染的时间
long eeui_next_render_time = 0;
String eeui_render_llm_text = "";

void espai_loop_eeui_text(int arg)
{
	// 文字渲染
	if (xSemaphoreTake(llm_text_mutex, pdMS_TO_TICKS(10)) == pdTRUE)
	{

		int totalCharCount = llm_text.length();
		if (totalCharCount <= 0 || (processed_llm_text_index >= totalCharCount))
		{
			xSemaphoreGive(llm_text_mutex);
			return;
		}
		if (millis() < eeui_next_render_time)
		{
			xSemaphoreGive(llm_text_mutex);
			return;
		}

		int nextPuncPos = -1;
		int i = processed_llm_text_index;
		while (i < llm_text.length())
		{
			int charByteLen = utf8_char_length(llm_text[i]); // 获取当前字符的 UTF-8 字节长度
			if (i + charByteLen > llm_text.length())
				break; // 防止越界

			String ch = llm_text.substring(i, i + charByteLen);
			if (is_punctuation(ch))
			{
				nextPuncPos = i + charByteLen; // 截到这个标点后为止
				break;
			}
			i += charByteLen;
		}
		if (nextPuncPos != -1 && nextPuncPos > processed_llm_text_index)
		{
			eeui_render_llm_text = llm_text.substring(processed_llm_text_index, nextPuncPos);
			int text_len = countTextLength(eeui_render_llm_text.c_str());
			eeui.set_bottom_scrolling_text(eeui_render_llm_text.c_str());
			processed_llm_text_index = nextPuncPos;
			eeui_await_punc_count = 0;
			eeui_next_render_time = millis() + text_len * 180;
		}
		else
		{
			if (eeui_await_punc_count > 3)
			{
				eeui_render_llm_text = llm_text.substring(processed_llm_text_index, llm_text.length());
				if (eeui_render_llm_text.length())
				{
					int text_len = countTextLength(eeui_render_llm_text.c_str());
					eeui.set_bottom_scrolling_text(eeui_render_llm_text.c_str());
					eeui_next_render_time = 0;
					processed_llm_text_index = nextPuncPos;
				}
			}
			else
			{
				eeui_await_punc_count++;
			}
		}
		xSemaphoreGive(llm_text_mutex);
	}
}

static int prev_signal = 0;
void espai_loop_eeui_status(int arg)
{
	if (ota_ed)
	{
		if (prev_signal != getSignalLevel())
		{
			prev_signal = getSignalLevel();
			eeui.render_signal(prev_signal); // 渲染信号
		}
	}
}

#endif

void on_volume(float volume)
{
	if (ota_ed)
	{
#if defined(ENABLE_EEUI)
		eeui.render_volume(volume); // 音量设置
#endif
		esp_ai.playBuiltinAudio(du_mp3, du_mp3_len);
	}
}

void setup()
{
	Serial.begin(115200);
	delay(500);

#if defined(ENABLE_EEUI)
	// 初始化屏幕
	emo_tft.begin();
	emo_tft.fillScreen(TFT_WHITE);
	emo_tft.setRotation(SCREEN_ROTA); // 设置屏幕方向
	eeui.begin(&emo_tft, emotions, sizeof(emotions) / sizeof(emotions[0]), SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_PAD_LEFT, SCREEN_PAD_RIGHT);

	/** 联网中 **/
	eeui.render_gif_by_name("联网中");
	eeui.set_status_text("网络连接中", false, "bottom_center");
#endif

	if (is_official == "1") // 官方固件才会走这里
	{
#if defined(BLE_MODEL)
		BIN_ID = "58d613ae5578475ca5077434d6aa6981";
#endif

// ===
#if defined(IS_XIAO_ZHI_S3_2)
#if defined(BLE_MODEL)
		BIN_ID = "b4669fa7d29744dda724b60fc83bb50d";
#else
		BIN_ID = "756ed3cc63604dc0bb2dcfc24a602a25";
#endif
// ===
#elif defined(IS_ESP_AI_S3_NO_SCREEN)
#if defined(BLE_MODEL)
		BIN_ID = "d7fc21bae7d44e9dabd138a176d0342e";
#else
		BIN_ID = "a8a840de18d4446b8b23ca28d42e2a86";
#endif
// ===
#elif defined(IS_ESP_AI_S3_TFT)
#if defined(BLE_MODEL)
		BIN_ID = "d0e20198943843f0a91f11d7962219d9";
#else
		BIN_ID = "6f608d802b4c4fa392cf337f93bda630";
#endif
// ===
#elif defined(IS_WU_MING_TFT)
#if defined(BLE_MODEL)
		BIN_ID = "5442916d96294423ab121b4ad1185e5e";
#else
		BIN_ID = "5666039aeefe4e49a177988c924165f1";
#endif
// ===
#elif defined(IS_AI_VOX_TFT)
#if defined(BLE_MODEL)
		BIN_ID = "3512f5201628404dbac10933ef49c4e7";
#else
		BIN_ID = "3b45f2bbb79b4940a925d0e4822352f1";
#endif
// ===
#elif defined(IS_ESP_AI_S3_DOUBLE_OLED)
#if defined(BLE_MODEL)
		BIN_ID = "0f64628e32224ae7992788175ccdff21";
#else
		BIN_ID = "d0392954145448008b2cfed22b2b8a23";
#endif
// ===
#elif defined(IS_ESP_AI_C3)
#if defined(BLE_MODEL)
		BIN_ID = "804cba02d51240bab3a37a6ddc5c945d";
#else
		BIN_ID = "b3c1829e14a74bdbb241dfc779ee0b39";
#endif
// ===
#elif defined(IS_ESP_AI_S3_TFT_EMO)
#if defined(BLE_MODEL)
		BIN_ID = "360691c7b8174bec914167cc27e29cfc";
#else
		BIN_ID = "xxx";
#endif
// ===
#elif defined(IS_WU_MING_TFT_EMO)
#if defined(BLE_MODEL)
		BIN_ID = "9e7f141c15dd496794b83c9fe205c867";
#else
		BIN_ID = "xxx";
#endif
// ===
#elif defined(IS_ESP_AI_S3_TFT_EMO_4G)
#if defined(BLE_MODEL)
		BIN_ID = "b6680eb5797f4c618628d241d028f984";
#else
		BIN_ID = "xxx";
#endif

#endif
	}
	else
	{

#if defined(IS_BOWKNOT)
#if defined(BLE_MODEL)
		BIN_ID = "30cd2bd00adf46eb8d9c53d9bbd349ef";
#else
		BIN_ID = "xxx";
#endif
#endif
	}

#if defined(ENABLE_POWER)
	// 配置ADC电压基准值与衰减倍数
	battery.begin(1250, 4.0);
#endif

#if defined(ENABLE_SUPPLY_DETECTION)
	pinMode(SUPPLY_DETECTION_PIN, INPUT);
#endif

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

#if defined(ENABLE_TFT) || defined(ENABLE_OLED)
#if defined(IS_XIAO_ZHI_S3_2)
	face = new Face(4, "091", 42, 41);
#elif defined(IS_XIAO_ZHI_S3_3)
	face = new Face(4, "096", 42, 41);
#elif defined(IS_ESP_AI_S3_OLED)
	face = new Face(4, "096", oled_sck.toInt(), oled_sda.toInt());
#elif defined(IS_ESP_AI_C3)
	face = new Face(4, "096", 4, 3, 0, 0x06FF, 0x0000, 0x06FF);
#elif defined(IS_ESP_AI_S3_DOUBLE_OLED)
	face = new Face(6, "096_2", oled_sck.toInt(), oled_sda.toInt());
#elif defined(IS_ESP_AI_S3_TFT) || defined(IS_AI_VOX_TFT) || defined(IS_WU_MING_TFT)
	face = new Face(8, "240*240", NULL, NULL, 90);
#endif
#endif

	otaManager = new ESPOTAManager(
		&webSocket_yw,

		// 设置屏幕文字
		[](const char *data)
		{
#if defined(ENABLE_OLED) || defined(ENABLE_TFT)
			face->ShowNotification(String(data));
#endif
		},
		// 仅仅显示屏幕文字, face 库使用
		[](bool only_show_notification)
		{
#if defined(ENABLE_OLED) || defined(ENABLE_TFT)
			face->OnlyShowNotification(true);
#endif
		},

		// 升级进度,
		[](int percent)
		{
#if defined(ENABLE_OLED) || defined(ENABLE_TFT)
			face->ShowNotification("升级进度： " + String(percent));
#endif

#if defined(ENABLE_EEUI)
			eeui.render_ota_percent(percent);
#endif
		},
		[](void)
		{ esp_ai.stopSession(); },
		[](void)
		{ esp_ai.delAllTask(); }, // 等待播放器完成的函数
		[](void)
		{ esp_ai.awaitPlayerDone(); }, // 等待播放器完成的函数
		[](const unsigned char *data, size_t len)
		{ esp_ai.playBuiltinAudio(data, len); }, // 播放音频的函数
		[]()
		{
			// 控制LED显示
			esp_ai_pixels->setPixelColor(0, esp_ai_pixels->Color(218, 164, 90));
			if (100 != esp_ai_pixels->getBrightness())
				esp_ai_pixels->setBrightness(100);
			else
				esp_ai_pixels->setBrightness(50);
			esp_ai_pixels->show();
		});

#if defined(ENABLE_OLED) || defined(ENABLE_TFT)
// 更新文字
#if defined(BLE_MODEL)
	face->SetChatMessage("ESP-AI V" + _version + "  - 蓝牙配网版 ");
#else
	face->SetChatMessage("ESP-AI V" + _version + "  - 热点配网版 ");
#endif
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
#if defined(ENABLE_EMO_LIGHT)
	String _emotion_led_number = esp_ai.getLocalData("emotion_led_number");
	if (_emotion_led_number != "")
	{
		emotion_led_number = _emotion_led_number.toInt();
	}
#endif
	if (kwh_enable == "")
	{
		kwh_enable = "1";
	}

	ESP_AI_volume_config volume_config = {volume_pin.toInt(), 4096, 1, volume_enable == "0" ? false : true};
	ESP_AI_lights_config lights_config = {18}; // esp32s3 开发板是 48
	ESP_AI_reset_btn_config reset_btn_config = {};
// 小智AI 引脚不一样
#if defined(IS_XIAO_ZHI_S3_2) || defined(IS_XIAO_ZHI_S3_3) || defined(IS_WU_MING_TFT) || defined(IS_WU_MING_TFT_EMO)
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
#else
	wifi_config.way = "AP";
#endif

#if !defined(ENABLE_POWER)
	kwh_enable = "0";
#if defined(ENABLE_OLED) || defined(ENABLE_TFT)
	face->SetBatHide(); // 不显示电量
#endif
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
#if defined(IS_ESP_AI_C3) || defined(IS_BOWKNOT)
		wake_up_config.pin = 9;
#else
		wake_up_config.pin = 0;
#endif
		strcpy(wake_up_config.wake_up_scheme, "pin_low"); // 唤醒方案
	}
	else if (ext7 == "pin_high_listen")
	{
		wake_up_config.pin = 10;
		strcpy(wake_up_config.wake_up_scheme, "pin_high_listen"); // 唤醒方案
	}
	else if (ext7 == "boot_listen")
	{
#if defined(IS_ESP_AI_C3) || defined(IS_BOWKNOT)
		wake_up_config.pin = 9;
#else
		wake_up_config.pin = 0;
#endif
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
		strcpy(wake_up_config.str, "start");			 // 串口和天问asrpro 唤醒时需要配置的字符串，也就是从另一个开发版发送来的字符串
	}
	if (lights_data != "")
	{
		lights_config.pin = lights_data.toInt();
	}

	// 一些板子不需要配置引脚，直接写死即可
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
#elif defined(IS_ESP_AI_C3) || defined(IS_BOWKNOT)
	ESP_AI_i2s_config_mic i2s_config_mic = {};
	ESP_AI_i2s_config_speaker i2s_config_speaker = {};
	volume_config.enable = false;
#if defined(IS_BOWKNOT)
	lights_config.pin = 4;
#endif

#else
	// ESP-AI 可配置化
	ESP_AI_i2s_config_mic i2s_config_mic = {mic_bck.toInt(), mic_ws.toInt(), mic_data.toInt()};
	ESP_AI_i2s_config_speaker i2s_config_speaker = {speaker_bck.toInt(), speaker_ws.toInt(), speaker_data.toInt()};
#endif

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
	esp_ai.onVolume(on_volume);

#if defined(IS_ESP_AI_C3)
	esp_ai.begin({debug, wifi_config, server_config, wake_up_config, volume_config});
#else
	esp_ai.begin({debug, wifi_config, server_config, wake_up_config, volume_config, i2s_config_mic, i2s_config_speaker, reset_btn_config, lights_config});
#endif
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

#if defined(ENABLE_OLED) || defined(ENABLE_TFT)
		face->SetVolume(int(volume * 100));
#endif
		volume = loc_volume.toFloat();
		LOG_D("本地音量：%0.2f", volume);
		esp_ai.setVolume(volume);
	}
	else
	{
#if defined(ENABLE_OLED) || defined(ENABLE_TFT)
		face->SetVolume(int(1 * 100));
#endif
	}

#if !defined(IS_ESP_AI_C3)
	// boot 按钮唤醒方式 esp-ai 库有问题，这begin()后再设置下就好了
	if (ext7 == "boot_listen" || ext7 == "boot")
	{
		pinMode(0, INPUT_PULLUP);
	}
#endif

#if defined(ENABLE_EMO_LIGHT)
	// 情绪灯光
	pixelsEmotion.begin();
	pixelsEmotion.setBrightness(100); // 亮度设置
#endif

	// 连接业务服务器
	webScoket_yw_main();
	timer.setInterval(1L, websocket_timer2, 0);

	// 定时器初始化
#if defined(ENABLE_POWER)
	timer.setInterval(report_systeminfo_interval, report_systeminfo_timer1, 0);
	timer.setInterval(600000L, battery_detection_timer, 0);
#endif
	timer.setInterval(1000L, update_check_timer3, 0);
	timer.setInterval(10L, espai_loop_timer5, 0);

	// 蝴蝶结固件按钮
#if defined(IS_BOWKNOT)
	timer.setInterval(1L, espai_loop_bowknot, 0);
#endif

// llm 文字/各种状态 渲染
#if defined(ENABLE_EEUI)
	timer.setInterval(500L, espai_loop_eeui_text, 0);
	timer.setInterval(100L, espai_loop_eeui_status, 0);
#endif

#if defined(ENABLE_SUPPLY_DETECTION)
	timer.setInterval(500L, espai_loop_timer_supply_det, 0);
#endif

#if defined(ENABLE_BTN_VOL)
	// 按键初始化
	button_vol_add.attachClick(attach_vol_add); // 加音量按键
	button_vol_sub.attachClick(attach_vol_sub); // 减音量按键
	timer.setInterval(50L, key_timer4, 0);
#endif
}

long last_log_time = 0;
void log_fh()
{
	if (millis() - last_log_time > 5000)
	{
		last_log_time = millis();
		Serial.print("===> 可用内存: ");
		Serial.print(ESP.getFreeHeap() / 1024);
		Serial.println("KB");
		// Serial.print("是否在充电中: ");
		// Serial.println(battery.getischarge());
	}
}

void loop()
{
	// 定时器
	timer.run();
	// log_fh();
}

// ================= 固件选择 ====================
//  #define IS_ESP_AI_S3_BASIC     // ESP-AI S3 开发板（不带屏幕、不带电池检查，只有基础功能），这个固件需要同时打开 IS_ESP_AI_S3_NO_SCREEN
//  #define IS_ESP_AI_S3_NO_SCREEN // ESP-AI S3 开发板（不带屏幕），这个固件需要同时打开 IS_ESP_AI_S3_BASIC
// #define IS_ESP_AI_S3_OLED // ESP-AI S3 开发板（OLED 屏）
// #define IS_ESP_AI_S3_DOUBLE_OLED // ESP-AI S3 开发板（双OLED 屏, 大白）
// #define IS_XIAO_ZHI_S3_2 // 小智AI S3 二代长条屏开发板
// #define IS_ESP_AI_S3_TFT // ESP-AI S3 开发板（TFT 屏 - 小眼睛表情）
#define IS_ESP_AI_S3_TFT_EMO // ESP-AI S3 开发板（TFT 屏 - EEUI）
// #define IS_ESP_AI_S3_TFT_EMO_4G // ESP-AI S3 开发板（TFT 屏 - EEUI - 4G）启用后需要在ESP-AI库的zi配置中启用。
// #define IS_AI_VOX_TFT  // AI_VOX S3 开发板（TFT 屏）
// #define IS_WU_MING_TFT // 无名科技 S3 开发板（TFT 屏）
// #define IS_WU_MING_TFT_EMO // 无名科技 S3 开发板（TFT 屏 - EEUI）
// #define IS_ESP_AI_C3 // ESP-AI-C3 开发板，支持OLED，兼容小智C3
// #define IS_BOWKNOT // ESP-AI-C3 蝴蝶结

// ================= 是否需要蓝牙配网 ====================
#define BLE_MODEL // 启动后需要注释 ESP-AI 库中的 DISABLE_BLE_NET 宏定义

/*********************************************************/

// ================== 版本定义 =========================
// 版本号必须和发布的一致,否则会自动g升级到发布过的版本
#if defined(IS_BOWKNOT)
#define VERSION "0.0.1"
#else 
#define VERSION "1.40.66" 
#endif

// ================== OTA 升级定义 =========================
// 是否为官方固件， 如果是您自己的固件请改为 "0"
#define IS_OFFICIAL "1"

// 如果是你自己的固件请将固件ID写到下面
#define DEF_BIN_ID "0"
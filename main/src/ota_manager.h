#ifndef ESP_OTA_MANAGER_H
#define ESP_OTA_MANAGER_H

#include "../USER_CONFIG.h"
#include <HTTPUpdate.h>
#include <WebSocketsClient.h>
#include <Arduino_JSON.h> 

#include "./logging.h"
#include "./audios/zh/shen_ji_shi_bai.h"

class ESPOTAManager
{
private:
    HTTPUpdate httpUpdate;
    String deviceId;
    WebSocketsClient *webSocket;
    void (*stopSession)() = nullptr;
    void (*delAllTask)() = nullptr;
    void (*awaitPlayerDone)() = nullptr;
    void (*playBuiltinAudio)(const unsigned char *data, size_t len) = nullptr;
    void (*ledAmi)() = nullptr;

    // Face *face = nullptr;

    // OTA状态变量
    long startUpdateTime;
    bool startUpdateEd = false;
    bool isUpdateProgress = false;
    long prevSendProgressTime;
    String otaProgress;
    bool willUpdate = false;

    // 回调函数
    static void updateStartedCallback();
    static void updateFinishedCallback();
    static void updateProgressCallback(int cur, int total);
    static void updateErrorCallback(int err);

    void (*showNotification)(const char *data) = nullptr;
    void (*onlyShowNotification)(bool only_show_notification) = nullptr;
    void (*onProgress)(int percent) = nullptr;

    // 静态成员变量，用于存储this指针
    static ESPOTAManager *instance;

public:
    ESPOTAManager(WebSocketsClient *webSocket,
                  // Face *face,
                  void (*ShowNotification)(const char *data),
                  void (*OnlyShowNotification)(bool only_show_notification),
                  void (*onProgress)(int percent),
                  void (*stopSessionCb)(),
                  void (*delAllTaskCb)(),
                  void (*awaitPlayerDoneCb)(), void (*playBuiltinAudioCb)(const unsigned char *data, size_t len) = nullptr, void (*ledAmiCb)() = nullptr);

    // 初始化OTA管理器，传入deviceId
    void init(const String &deviceId);

    // 执行OTA升级
    void update(const String &url);

    // 获取OTA状态
    bool isUpdating() const;
    String getProgress() const;
    bool updateFailed() const;
};

#endif // ESP_OTA_MANAGER_H
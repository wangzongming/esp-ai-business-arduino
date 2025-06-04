#ifndef ESP_OTA_MANAGER_H
#define ESP_OTA_MANAGER_H

#include <esp-ai.h>
#include <HTTPUpdate.h>
#include <WebSocketsClient.h>
#include "Face.h"
#include <Arduino_JSON.h>

#include "./logging.h"
#include "./audios/zh/shen_ji_shi_bai.h"

class ESPOTAManager
{
private:
    HTTPUpdate httpUpdate;
    String deviceId;
    WebSocketsClient *webSocket;
    ESP_AI *esp_ai;
    Face *face = nullptr;

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

    // 静态成员变量，用于存储this指针
    static ESPOTAManager *instance; 

public:
    ESPOTAManager(WebSocketsClient *webSocket, ESP_AI *esp_ai, Face *face);

    // 初始化OTA管理器，传入deviceId
    void init(String deviceId);

    // 执行OTA升级
    void update(String url);

    // 获取OTA状态
    bool isUpdating() const;
    String getProgress() const;
    bool updateFailed() const;
};

#endif // ESP_OTA_MANAGER_H
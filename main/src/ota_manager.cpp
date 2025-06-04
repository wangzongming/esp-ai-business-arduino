#include "ota_manager.h"

// 初始化静态成员变量
ESPOTAManager *ESPOTAManager::instance = nullptr;

ESPOTAManager::ESPOTAManager(WebSocketsClient *webSocket, ESP_AI *esp_ai, Face *face)
    : webSocket(webSocket), esp_ai(esp_ai), face(face), startUpdateTime(0),
      startUpdateEd(false), isUpdateProgress(false), prevSendProgressTime(0)
{
    instance = this;
}

void ESPOTAManager::init(String deviceId)
{
    this->deviceId = deviceId;

    // 设置回调函数
    httpUpdate.onStart(updateStartedCallback);
    httpUpdate.onEnd(updateFinishedCallback);
    httpUpdate.onProgress(updateProgressCallback);
    httpUpdate.onError(updateErrorCallback);
}

void ESPOTAManager::update(String url)
{
    esp_ai->stopSession();
    vTaskDelay(500 / portTICK_PERIOD_MS);
    // 重置状态
    startUpdateEd = false;
    isUpdateProgress = false;
    willUpdate = true;

    esp_ai->delAllTask();
// 显示升级通知
#if !defined(IS_ESP_AI_S3_NO_SCREEN)
    face->OnlyShowNotification(true);
    face->ShowNotification("正在升级");
#endif

    // 记录开始时间
    startUpdateTime = millis();
    Serial.print("OTA 地址：");
    Serial.println(url);

    // 执行升级
    WiFiClient updateClient;
    t_httpUpdate_return ret = httpUpdate.update(updateClient, url);

    LOG_D("===================升级完毕=========================");
    switch (ret)
    {
    case HTTP_UPDATE_FAILED:
        LOG_D("[update] Update failed. Error code: %d", httpUpdate.getLastError());
        wait_mp3_player_done();
        play_builtin_audio(shen_ji_shi_bai_mp3, shen_ji_shi_bai_mp3_len);
        wait_mp3_player_done();
        break;
    case HTTP_UPDATE_NO_UPDATES:
        LOG_D("[update] Update no Update.");
        break;
    case HTTP_UPDATE_OK:
        LOG_D("[update] Update ok.");
        break;
    }
}

bool ESPOTAManager::isUpdating() const
{
    if (willUpdate)
    {
        return true;
    }
    return startUpdateEd && !updateFailed();
}

String ESPOTAManager::getProgress() const
{
    return otaProgress;
}

bool ESPOTAManager::updateFailed() const
{
    // 如果长时间没有进度更新，认为升级失败
    if (startUpdateEd && !isUpdateProgress && (millis() - startUpdateTime > 20000))
    {
        return true;
    }
    return false;
}

// 静态回调函数实现
void ESPOTAManager::updateStartedCallback()
{
    if (instance)
    {
        instance->startUpdateTime = millis();
        instance->startUpdateEd = true;
        LOG_D("CALLBACK:  HTTP更新进程启动");
    }
}

void ESPOTAManager::updateFinishedCallback()
{
    if (instance)
    {
        LOG_D("[Info] -> CALLBACK:  HTTP更新进程完成");
    }
}

void ESPOTAManager::updateProgressCallback(int cur, int total)
{
    if (instance)
    {
        instance->isUpdateProgress = true;
        float percentage = (float)cur / total * 100;
        instance->otaProgress = String(percentage, 2) + "%";

        String progress = instance->otaProgress.c_str();
#if !defined(IS_ESP_AI_S3_NO_SCREEN)
        instance->face->ShowNotification("升级进度： " + progress);
#endif

        // 1.5秒钟发送一次进度
        if (millis() - instance->prevSendProgressTime > 1500)
        {
            LOG_D("[Info] -> 更新进度：%s", progress);
            instance->prevSendProgressTime = millis();

            // 将进度发往业务服务
            JSONVar data_ota;
            data_ota["type"] = "ota_progress";
            data_ota["data"] = percentage;
            data_ota["device_id"] = instance->deviceId;
            String sendData = JSON.stringify(data_ota);
            instance->webSocket->sendTXT(sendData);

            // 控制LED显示
            esp_ai_pixels.setPixelColor(0, esp_ai_pixels.Color(218, 164, 90));
            if (100 != esp_ai_pixels.getBrightness())
                esp_ai_pixels.setBrightness(100);
            else
                esp_ai_pixels.setBrightness(50);
            esp_ai_pixels.show();
        }
    }
}

void ESPOTAManager::updateErrorCallback(int err)
{
    if (instance)
    {
        LOG_D("CALLBACK:  HTTP更新致命错误代码 %d\n", err);
    }
}
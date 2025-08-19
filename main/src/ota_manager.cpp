#include "ota_manager.h"

// 初始化静态成员变量
ESPOTAManager *ESPOTAManager::instance = nullptr;

ESPOTAManager::ESPOTAManager(WebSocketsClient *webSocket, 
                             void (*ShowNotification)(const char *data),
                             void (*OnlyShowNotification)(bool only_show_notification),
                             void (*onProgress)(int percent),
                             void (*stopSessionCb)(), 
                             void (*delAllTaskCb)(), 
                             void (*awaitPlayerDoneCb)(), 
                             void (*playBuiltinAudioCb)(const unsigned char *data, size_t len), 
                             void (*ledAmiCb)())
    : webSocket(webSocket),  
      startUpdateTime(0),
      startUpdateEd(false), isUpdateProgress(false), prevSendProgressTime(0)
{
    instance = this;
    this->stopSession = stopSessionCb;
    this->delAllTask = delAllTaskCb;
    this->awaitPlayerDone = awaitPlayerDoneCb;
    this->playBuiltinAudio = playBuiltinAudioCb;
    this->ledAmi = ledAmiCb;

    this->showNotification = ShowNotification;
    this->onlyShowNotification = OnlyShowNotification;
    this->onProgress = onProgress;
}

void ESPOTAManager::init(const String &deviceId)
{
    this->deviceId = deviceId;

    // 设置回调函数
    httpUpdate.onStart(updateStartedCallback);
    httpUpdate.onEnd(updateFinishedCallback);
    httpUpdate.onProgress(updateProgressCallback);
    httpUpdate.onError(updateErrorCallback);
}

void ESPOTAManager::update(const String &url)
{
    this->stopSession();
    vTaskDelay(500 / portTICK_PERIOD_MS);
    // 重置状态
    startUpdateEd = false;
    isUpdateProgress = false;
    willUpdate = true;

    this->delAllTask();
 
    this->showNotification("正在升级");
    this->onlyShowNotification(true);

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
        this->awaitPlayerDone();
        this->playBuiltinAudio(shen_ji_shi_bai_mp3, shen_ji_shi_bai_mp3_len);
        this->awaitPlayerDone();
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
        // #if defined(ENABLE_OLED) || defined(ENABLE_TFT)
        //         instance->face->ShowNotification("升级进度： " + progress);
        // #endif
        instance->onProgress(percentage);

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
            instance->ledAmi();
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
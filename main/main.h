#include "Face.h"

#include "src/logging.h"
#include "src/ota_manager.h"
#include "src/auto_update.h"

/****************************宏定义区*****************************/

// ================= S3 开发板选择 ====================
// #define IS_ESP_AI_S3_NO_SCREEN  // ESP-AI S3 开发板（不带屏幕） Z芯
#define IS_ESP_AI_S3_OLED         // ESP-AI S3 开发板（OLED 屏）
// #define IS_ESP_AI_S3_DOUBLE_OLED         // ESP-AI S3 开发板（双OLED 屏）
// #define IS_ESP_AI_S3_TFT          // ESP-AI S3 开发板（TFT 屏）
// #define IS_AI_VOX_TFT  // AI_VOX S3 开发板（TFT 屏）
// #define IS_WU_MING_TFT // 无名科技 S3 开发板（TFT 屏）
// #define IS_XIAO_ZHI_S3_2 // 小智AI S3 二代长条屏开发板
// #define IS_XIAO_ZHI_S3_3  // 小智AI S3 三代方平屏开发板
// ====================================================

#define EMOTION_LIGHT_PIN 46 // 情绪灯光控制引脚

#if defined(IS_ESP_AI_S3_OLED) || defined(IS_ESP_AI_S3_DOUBLE_OLED) || defined(IS_XIAO_ZHI_S3_2) || defined(IS_XIAO_ZHI_S3_3)
#if SCREEN_TYPE == 0
#error "请在正确配置 SCREEN_TYPE 屏幕类型"
#endif
#elif defined(IS_ESP_AI_S3_TFT) || defined(IS_AI_VOX_TFT)
#if SCREEN_TYPE == 1
#error "请在正确配置 SCREEN_TYPE 屏幕类型"
#endif
#endif

#define BAT_PIN 8 // 电池电压检测引脚

// ================== 调试打印 ==========
// #define LOG_D(fmt, ...)   printf_P(("[%s][%d]:" fmt "\r\n") , __func__, __LINE__, ##__VA_ARGS__)

// ===========================================================
// [可 填] 自定义配网页面
#if defined(IS_ESP_AI_S3_OLED) || defined(IS_ESP_AI_S3_DOUBLE_OLED) || defined(IS_ESP_AI_S3_TFT) || defined(IS_ESP_AI_S3_NO_SCREEN)
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

#if defined(IS_XIAO_ZHI_S3_2) || defined(IS_XIAO_ZHI_S3_3) || defined(IS_WU_MING_TFT) || defined(IS_AI_VOX_TFT)
#define VOL_ADD_KEY 40
#define VOL_SUB_KEY 39
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
                    <select id='wakeup-type' name='wakeup-type' placeholder='请选择唤醒/对话方式' value='boot'
                        style='width:  calc(100% - 80px)'>
                        <option value='boot' id='boot'>BOOT按钮唤醒（唤醒按钮）</option>
                        <option value='asrpro' id='asrpro'>天问唤醒</option> 
                        <option value='boot_listen' id='boot_listen'>按住对话(BOOT按钮)</option> 
                    </select>
                </div>
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
            var mic_bck = "5";
            var mic_ws = "4";
            var mic_data = "6";
            var speaker_bck = "15";
            var speaker_ws = "16";
            var speaker_data = "7"; 

            var lights_data = "48";
            var oled_type = "091";
            var oled_sck = "42";
            var oled_sda = "41";

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

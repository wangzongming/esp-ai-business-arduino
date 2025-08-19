<div align="center"> 
<a name="readme-top"></a>

<div style="background:#fff;border-radius: 12px;width:300px;">
  <img src="https://espai.fun/images/logo.png"/> 
</div> 

<h1>ESP-AI 开放平台 - Arduino 代码</h1>

硬件接入AI最简单、最低成本的方案<br/>The simplest and lowest cost solution for any item to access AI

</div> 


# 简介

本仓库完全基于 `ESP-AI` 开源库进行实现，旨在为开发者和企业提供快速接入 `ESP-AI` 服务的参考代码。      

开发者仅需开通一套[开放平台分站](https://espai.fun/acout/cooperation/#%E6%88%90%E4%B8%BA%E5%88%86%E9%94%80%E5%95%86%E3%80%81%E4%BB%A3%E7%90%86%E5%95%86)或者使用[`ESP-AI OPEN API`](https://espai.fun/dev/dev-open-api/)来开发自己的应用即可快速推出自己的可商业使用的 `AI` 应用。


**总的来说：基于本仓库代码，可以实现各种业务需求。开发者可以删除不需要的功能，或者在固件中写好您的秘钥信息等各种灵活操作...**



<p align="center">
  <img src="https://esp-ai2.oss-cn-beijing.aliyuncs.com/icon/官方固件/TFT屏幕" height="300">
  <img src="https://esp-ai2.oss-cn-beijing.aliyuncs.com/icon/官方固件/feng_main.gif" height="300">
</p>

<p align="center">
  <img src="https://esp-ai2.oss-cn-beijing.aliyuncs.com/icon/官方固件/espai带屏幕版本" height="300">
  <img src="https://esp-ai2.oss-cn-beijing.aliyuncs.com/icon/官方固件/大白"  height="300">
</p>



# 功能

1. [x] 已对接好 `ESP-AI`开放平台服务
2. [x] 支持 `OTA` 升级
3. [x] 支持 `OLED` 屏幕（0.96寸/0.91寸。 动态小表情）
4. [x] 支持 `TFT` 屏幕（1.3寸，240 *240，7789驱动。 可渲染图片/表情） 
5. [x] 微信小程序客户端 
6. [x] 蓝牙配网 

# 适配的主控

1. [x] esp32s3  
2. [x] esp32c3 
 

# 项目结构

```
esp-ai-business-arduion/
├── main/
│   ├── main.ino                    # 主程序-开放平台版本
│   └── voice.hd                    # 天问代码（天问语音唤醒） 
├── web/              
│   ├── index.html                  # 配网页面
│   └── index_xiao_zhi_2.html       # 配网页面（兼容小智硬件）
└── README.md
```

# 环境配置

## Platformio + VsCode (推荐)
1. 下载整个仓库。
```bash
git clone https://gitee.com/xm124/esp-ai-business-arduino
```
2. 解压`libraries2.zip`。
3. 打开`VsCode` 安装 `Platformio` 插件。
4. 点击底部上传或者打包按钮即可。

详情文档见：[Platformio + VsCode](https://espai.fun/guide/client-dev/#_1-3%E3%80%81arduino-%E5%BC%80%E5%8F%91%E6%9D%BF%E7%8E%AF%E5%A2%83%E5%AE%89%E8%A3%85)

## Arduino IDE
1. 先看一遍文档： 
[ Arduino IDE 文档](https://espai.fun/guide/client-dev/#%E4%B8%80%E3%80%81arduino-ide-%E7%8E%AF%E5%A2%83)

2. 下载依赖库 `libraries` 中所有依赖文件，然后解压到 `C:\Users\[用户名]\Documents\Arduino\libraries` (注意 用户名自己改成你电脑用户名，并且删除中括号。)

**注意**
如果你用 platformIO，需要注意自行将： `main\platformio.ini` 中的 `lib_dir` 改为 `../libraries2`


# 固件打包

参见文章： [Arduino 导出bin文件并且使用ESP32烧录工具进行烧录（esp32生产环境批量烧录）
](https://juejin.cn/post/7436363573348696118)


# 将固件发布到 ESP-AI 开放平台或者发布到您的分站

该功能正在积极开发中，预计在 2025/6 月份或更早上线，敬请期待~




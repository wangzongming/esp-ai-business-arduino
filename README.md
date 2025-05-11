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



# 功能

1. [x] 已对接好 `ESP-AI`开放平台服务
2. [x] 支持 `OTA` 升级
3. [x] 支持 `OLED` 屏幕（0.96寸/0.91寸。 动态小表情）
4. [x] 支持 `TFT` 屏幕（1.3寸，240 *240，7789驱动。 可根据情绪渲染不同图片） 
5. [ ] 微信小程序客户端 （很快推出）
6. [ ] 蓝牙配网（很快推出）


# 项目结构

```
esp-ai-business-arduion/
├── main/
│   ├── main.ino                    # 主程序-开放平台版本
│   └── voice.hd                    # 天问代码（天问语音唤醒）
├── main-img/             
│   ├── main-img.ino                # 主程序-TFT图片版
│   └── voice.hd                    # 天问代码（天问语音唤醒）
├── web/              
│   ├── index.html                  # 配网页面
│   └── index_xiao_zhi_2.html       # 配网页面（兼容小智硬件）
└── README.md
```

# 环境配置

1. 先看一遍文档： 
[ Arduino IDE 文档](https://espai.fun/guide/client-dev/#%E4%B8%80%E3%80%81arduino-ide-%E7%8E%AF%E5%A2%83)

2. 下载依赖库 `libraries2.zip`，然后解压到 `C:\Users\[用户名]\Documents\Arduino\libraries` (注意 用户名自己改成你电脑用户名，并且删除中括号。)



# 彩屏图片更改

打开代码文件 `main-img\main-img.ino` 修改下面代码即可：

```c++
// 定义一个情感与图片 URL 的映射数组
EmotionImagePair emotionToImage[] = {
  // 第一个图一定要放默认图片
  // 不同的表情可以使用同一张图片
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
```

**注意：**  
1. 图片必须使用 `http`, 不要使用 `https`。
2. 图片尺寸必须为 `240 * 240`像素
3. 图片大小必须在 `10kb` 以内
 


# 1.54 寸 TFT 屏幕接线（8pin）
 
| ESP-AI(v1/v2/mini) | TFT |
| --------------- | --- |
| 3.3v            | vcc |
| GND             | GND |
| 1               | SCK |
| 2               | SDA |
| 48              | CS  |
| 47              | DC  |
| 45              | RST |
| 13              | BL  |

 

# 1.3 寸 TFT 屏幕接线（7pin）

需要自行打开 `libraries\TFT_eSPI\User_Setup.h` 将下面的两行代码注释。
``` c++
// #define TFT_RST   45  // Reset pin (could connect to NodeMCU RST, see next line)
// #define TFT_BL    13  // LED back-light (only for ST7789 with backlight control pin)
```

| ESP-AI(v1/v2/mini) | TFT |
| --------------- | --- |
| 3.3v            | vcc |
| GND             | GND |
| 1               | SCK |
| 2               | SDA |
| 48              | CS  |
| 47              | DC  | 



# 固件打包

参见文章： [Arduino 导出bin文件并且使用ESP32烧录工具进行烧录（esp32生产环境批量烧录）
](https://juejin.cn/post/7436363573348696118)


# 将固件发布到 ESP-AI 开放平台或者发布到您的分站

该功能正在积极开发中，预计在 2025/6 月份或更早上线，敬请期待~




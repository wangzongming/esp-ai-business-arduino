


## 前言
...


---
## 项目结构
...


---
## vscode 中运行

1. 安装 `platformio` 插件
2. 点击底部上传按钮即可


注意：`项目路径不可以出现中文或者嵌套的很深，否则编译可能会报错`

---

## Window WSL 中运行(ubuntu22)

### 1. 在 `WSL` 中安装 `PlatformIO`
```base
pip install platformio
```

### 2. 安装 `usbipd-win`

打开 管理员 `PowerShell`，执行以下命令：

```powershell
winget install --interactive --exact dorssel.usbipd-win
```
或者手动下载安装： https://github.com/dorssel/usbipd-win/releases

### 3. 将 `USB` 设备连接到 `WSL`
**步骤 1：列出可用 USB 设备**
```powershell
usbipd wsl list
```
输出示例：
```
plaintext
BUSID  VID:PID    DEVICE                                                        STATE
1-3    10c4:ea60  CP2102N USB to UART Bridge Controller [COM3]               Not attached
1-4    046d:c077  Logitech USB Input Device                                     Not attached
```

**步骤 2：将设备连接到 WSL**


先共享设备
```cmd
usbipd bind --busid 2-4
```
然后将设备连接 `wsl`
```powershell
# 2-4 和 ubuntu-20.04 替换为实际值
usbipd attach --busid 2-4 --wsl ubuntu-20.04
```

**步骤 3：在 WSL 中验证设备**

列出 WSL 中的 USB 设备
```sh
lsusb
```

查看串口设备
```sh
ls /dev/ttyUSB*
```


### 4. 执行编译`pio run`

这一步只是测试编译代码是否有问题


### 5. 烧录代码
```sh
pio run -t upload --upload-port /dev/ttyUSB0
```

### 6. 串口监听
```sh
pio device monitor --baud 115200
```
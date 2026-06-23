# CarKey V5 - ESP32-S3 车辆远程控制系统

## 项目概述
基于 ESP32-S3 的汽车遥控钥匙系统，支持 NFC刷卡解锁、蓝牙接近自动解锁、Web远程控制、OTA固件升级。适用于马自达6 2006款（及类似车型）。

## 硬件平台
- 主控：ESP32-S3 N16R8（16MB Flash / 8MB PSRAM）
- OLED：SH1106 128x64 I2C（SDA=GPIO8, SCL=GPIO21, 地址0x3C）
- NFC：PN532 SPI（SCK=12, MISO=13, MOSI=11, SS=10）
- 灯光：WS2812 16灯 RGB环（GPIO48）+ 蜂鸣器（GPIO47）
- 继电器：引擎启动/熄火控制（GPIO5）
- 传感器：手刹GPIO41、空档GPIO42、电瓶电压ADC GPIO4

## 编译环境
- PlatformIO + Arduino框架
- 分区表：双OTA分区（app0 6MB + app1 5.875MB）+ SPIFFS 4MB
- 依赖库：Adafruit PN532, Adafruit NeoPixel, Adafruit SH110X, Adafruit GFX, ArduinoJson

## 核心模块架构

### main.cpp
启动序列：延迟3s → Serial → StatusLight → 配置加载 → OTA检查 → 手刹/空档引脚 → 电瓶电压 → 继电器 → BLE → OLED显示 → WiFi/Web → NFC → FreeRTOS任务 → 休眠管理器

主循环每500ms调用sleepManager.update()，空闲时自动进入Light Sleep省电

### Config.cpp/h
- 系统配置管理（WiFi SSID/密码、蓝牙名称、启动密码、安全开关）
- NVS持久化存储（carkey_sys命名空间）
- 全局日志缓冲 sysLog() → Serial + webLogBuffer
- 构建时间戳 kBuildStamp

### BLEManager.cpp/h
- BLE扫描检测授权手机（扫描周期：快扫3s/慢扫3s，间隔3-5s）
- 配对模式：ESP32作为BLE Server，手机连接后保存设备名称
- 认证方式：仅名称匹配（isPairedDevice），适配手机MAC随机化
- 授权窗口：检测到手机后180秒内视为有效

### NFCManager.cpp/h
- PN532读卡，3次重试初始化
- RFID卡片UID读取，AuthManager验证白名单
- 刷卡成功 → 解锁/上锁车门（VEHICLE_CMD_LOCK_TOGGLE）

### RelayManager.cpp/h
- 引擎启动控制（GPIO5继电器）
- 启动条件检查：空档+手刹+密码验证（可选）
- 远程启动：Web或MQTT命令触发
- 低电压保护：<11.8V禁止启动

### StateMachine.cpp/h
- 车辆状态机：锁定/解锁/引擎运转/等待认证
- 安全带校验：启动后60秒内需NFC二次认证，否则自动熄火

### WebManager.cpp/h
- HTTP服务（端口80），SPIFFS文件服务
- 内嵌Web页面（EmbeddedIndexPage.h），单页应用
- API端点：
  /api/status - 车辆状态（电压、档位、手刹、BLE、NFC、引擎）
  /api/start_engine?pwd=xxx - 远程启动
  /api/stop_engine?pwd=xxx - 远程熄火
  /api/update_config - 配置保存
  /api/logs - 实时日志
  /api/ota - 固件上传
  /api/ota/download - URL下载固件
  /api/ota/status - OTA状态（含版本号）
  /api/ota/rollback - 固件回滚
  /api/system/info - 内存/Flash/PSRAM使用
  /api/reboot - 重启
- WiFi事件处理（连接/断开均有日志）
- 配置锁定：NFC刷卡解锁后才可修改设置

### DisplayManager.cpp/h
- SH1106 OLED 128x64驱动
- 初始化含完整寄存器序列（电荷泵使能等）
- 根据StatusLight状态显示对应文字

### StatusLight.cpp/h
- WS2812 16灯RGB环状态指示
- 状态动画：呼吸/闪烁/追逐/旋转
- 蜂鸣器：成功/失败提示音

### SleepManager.cpp/h
- Light Sleep低功耗管理（~0.8mA休眠电流）
- 引擎运转/BLE授权/NFC解锁时保持唤醒
- 空闲15秒后进入休眠，RTC定时器唤醒
- 休眠前WiFi进入modem-sleep省电模式

### OTAManager.cpp/h
- 双分区OTA升级（ESP32原生OTA API）
- 上传方式：文件上传 / URL下载
- 固件校验：启动后标记有效，失败自动回滚
- 版本管理：构建时间戳记录到NVS

### TaskManager.cpp/h
- FreeRTOS多任务调度（7个任务）
  - VehicleTask（优先级4）：车辆指令队列处理
  - NfcTask（优先级5）：NFC轮询读卡
  - WebTask（优先级2）：HTTP请求处理
  - BleTask（优先级1）：BLE扫描循环
  - SensorTask（优先级3）：电压/传感器更新
  - WatchdogTask（优先级1）：任务心跳监控，超时恢复/重启

## 数据流
手机蓝牙接近 → BLE扫描检测名称匹配 → 180s授权窗口
  └→ Web远程启动 → /api/start_engine → 密码验证 → 继电器吸合
NFC刷卡 → UID验证 → 解锁车门 / 配置解锁

## Web页面功能
- 仪表盘：档位、手刹、蓝牙、NFC、电瓶电压
- 引擎控制：START/STOP按钮（需密码）
- BLE配对：开始/停止/清除
- 系统设置：WiFi、蓝牙名称、启动密码、安全开关
- OTA升级：文件上传（含进度条）+ URL下载 + 回滚
- 实时日志面板（2秒轮询服务端日志）

## 当前优化的休眠参数
kIdleTimeoutMs = 15000（15秒空闲后休眠）
kSleepDurationMs = 30000（每30秒唤醒检查）

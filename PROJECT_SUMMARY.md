# CarKey V5 - ESP32-S3 车辆远程控制系统

## 版本历史

### V1.1 (2026-06-23) — 车辆状态检测系统
- **新增** VehicleStatusManager 模块，统一管理所有车辆状态检测
- **新增** 发动机运行检测：电瓶电压滞回判断（<13.2V停止 / 13.2~13.5V保持 / >13.5V运行）
- **新增** ACC状态检测：GPIO32输入（高电平=ON）
- **新增** 手刹状态检测：迁移至GPIO34输入（低电平=拉起）
- **新增** 驾驶员车门检测：GPIO35输入（低电平=开门）
- **新增** 启动结果确认：继电器执行后等待3秒，通过电压验证发动机实际启动状态
- **新增** 电瓶健康分级：>=12.5V GOOD / 12.2~12.5V LOW / <12.2V CRITICAL
- **新增** 统一API `/api/status` 扩展字段：engineRunning, batteryVoltage, batteryHealth, acc, handBrake, driverDoorOpen
- **新增** Web仪表盘状态卡片：ENGINE / BATTERY / ACC / PARK BRAKE / DOOR
- **新增** 日志类型：[ENGINE] [BATTERY] [ACC] [BRAKE] [DOOR]
- **修改** RelayManager 不再持有 engineRunning，状态检测统一由 VehicleStatusManager 管理
- **修改** /api/start_engine 改为同步验证流程（继电器→等待3秒→电压确认→返回结果）
- **修改** 手刹引脚从GPIO32迁移至GPIO34，GPIO32改为ACC检测
- **修改** WebManager `/api/system/info` JSON字符串转义修复

### V5 (初始版本)
- 基于ESP32-S3的汽车遥控钥匙系统
- NFC刷卡解锁、BLE接近自动解锁、Web远程控制、OTA固件升级

---



## 项目概述
基于 ESP32-S3 的汽车遥控钥匙系统，支持 NFC刷卡解锁、蓝牙接近自动解锁、Web远程控制、OTA固件升级。适用于马自达6 2006款（及类似车型）。

## 硬件平台
- 主控：ESP32-S3 N16R8（16MB Flash / 8MB PSRAM）
- OLED：SH1106 128x64 I2C（SDA=GPIO8, SCL=GPIO21, 地址0x3C）
- NFC：PN532 SPI（SCK=12, MISO=13, MOSI=11, SS=10）
- 灯光：WS2812 16灯 RGB环（GPIO48）+ 蜂鸣器（GPIO47）
- 继电器：引擎启动/熄火控制（GPIO5）
- 传感器：手刹GPIO34(V1.1从GPIO32迁移)、空档GPIO33、ACC GPIO32(V1.1新增)、车门GPIO35(V1.1新增)、电瓶电压ADC GPIO4/36

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
- 引擎启动控制（GPIO5继电器）、点火(IGN GPIO25)、启动马达(START GPIO26)、喇叭(HORN GPIO27)
- 启动条件检查：空档+手刹+低电压保护(<11.8V禁止启动)
- 远程启动：Web直接调用或MQTT命令触发
- **V1.1**: 不再持有engineRunning状态，仅执行继电器序列；状态检测交由VehicleStatusManager


### VehicleStatus.cpp/h (V1.1新增)
- 车辆状态统一管理模块，负责所有传感器状态采集、缓存、变化日志
- **发动机检测**: 电瓶电压滞回判断，<13.2V停止 / 13.2~13.5V保持原状态 / >13.5V运行
- **ACC状态**: GPIO32数字输入（HIGH=ON, LOW=OFF）
- **手刹状态**: GPIO34数字输入（LOW=拉起, HIGH=释放）
- **车门状态**: GPIO35数字输入（LOW=开门, HIGH=关门），第一版仅驾驶员车门
- **电池健康**: 电压分级 GOOD(>=12.5V) / LOW(12.2~12.5V) / CRITICAL(<12.2V)
- **启动确认**: verifyEngineStart() — 继电器序列后等待3秒，通过电压检测确认启动结果
- **状态变化日志**: 发动机启停、电池电压变化(>0.2V)、电池健康变化、ACC/手刹/车门状态变化均记录到日志
- **API输出**: toJson() 方法输出统一JSON格式供Web API使用

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
  - SensorTask（优先级3）：电瓶电压/继电器验证/VehicleStatus/StatusLight/Display更新
  - WatchdogTask（优先级1）：任务心跳监控，超时恢复/重启

## 数据流
手机蓝牙接近 → BLE扫描检测名称匹配 → 180s授权窗口
  └→ Web远程启动 → /api/start_engine → 密码验证 → 继电器序列(IGN→START) → 等待3秒 → VehicleStatus电压确认 → 返回启动结果
NFC刷卡 → UID验证 → 解锁车门 / 配置解锁

V1.1 状态数据流:
GPIO32(ACC) → VehicleStatusManager::updateGpioStates() → 状态缓存 + 变化日志
GPIO34(手刹) → VehicleStatusManager::updateGpioStates() → 状态缓存 + 变化日志
GPIO35(车门) → VehicleStatusManager::updateGpioStates() → 状态缓存 + 变化日志
ADC电压    → BatteryVoltage::update() → VehicleStatusManager::updateEngineState() → 滞回判断 + 健康分级
                  ↓
         /api/status (2秒轮询) → Web仪表盘状态卡片实时更新

## Web页面功能
- 仪表盘：引擎状态、电瓶电压/健康、ACC、手刹、车门、档位、蓝牙、NFC
- 引擎控制：START/STOP按钮（需密码）
- BLE配对：开始/停止/清除
- 系统设置：WiFi、蓝牙名称、启动密码、安全开关
- OTA升级：文件上传（含进度条）+ URL下载 + 回滚
- 实时日志面板（2秒轮询服务端日志）

## 当前优化的休眠参数
kIdleTimeoutMs = 15000（15秒空闲后休眠）
kSleepDurationMs = 30000（每30秒唤醒检查）

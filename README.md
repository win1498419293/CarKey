# CarKey V5 — ESP32-S3 车辆远程控制系统

基于 ESP32-S3 的智能汽车遥控钥匙系统，支持 **NFC 刷卡解锁**、**BLE 蓝牙接近自动解锁**、**Web 远程控制**、**4G 蜂窝远程操控**、**MQTT 云平台通信**、**OTA 固件升级**。适用于马自达 6 2006 款及类似车型。

## 已实现功能

### 核心控制
- **NFC 刷卡解锁** — PN532 读取 RFID 卡片 UID，白名单验证，刷卡解锁/上锁车门
- **BLE 蓝牙接近解锁** — BLE 扫描检测授权手机设备名称，180 秒授权窗口自动解锁
- **引擎远程启动/熄火** — 通过 Web 页面或 MQTT 远程指令触发继电器序列（IGN→START），启动后电压确认
- **安全认证** — Web 启动需密码验证，MQTT 指令需 HMAC-SHA256 签名校验

### 车辆状态检测
- **发动机运行检测** — 电瓶电压滞回判断（<13.2V 停止 / 13.2~13.5V 保持 / >13.5V 运行）
- **ACC 状态检测** — GPIO32 数字输入
- **手刹状态检测** — GPIO34 数字输入
- **驾驶员车门检测** — GPIO35 数字输入
- **电瓶健康分级** — GOOD(>=12.5V) / LOW(12.2~12.5V) / CRITICAL(<12.2V)
- **启动结果确认** — 继电器执行后等待 3 秒，通过电压验证发动机实际启动状态

### 网络与远程通信
- **WiFi Web 控制** — 内嵌单页 Web 应用，实时仪表盘 + 引擎控制 + 系统设置
- **4G 蜂窝网络** — Air780EP 模块通过 AT 指令实现 MQTT 远程通信，信号质量监测
- **MQTT 云平台** — WiFi 和 4G 双通道 MQTT，状态上报 + 指令接收，自动重连与心跳
- **NetworkManager 网络抽象** — 统一 Client 接口，WiFi / 4G 自动切换，上层代码无感知

### Web 仪表盘
- 引擎状态、电瓶电压/健康、ACC、手刹、车门实时显示
- 引擎 START/STOP 控制按钮
- BLE 配对管理
- 系统设置（WiFi、蓝牙名称、启动密码、安全开关）
- OTA 固件升级（文件上传 + URL 下载 + 回滚）
- 实时日志面板

### 系统能力
- **OTA 双分区固件升级** — 失败自动回滚，版本管理
- **FreeRTOS 多任务调度** — 7 个任务（车辆指令、NFC 轮询、Web 服务、BLE 扫描、传感器、看门狗）
- **Light Sleep 低功耗** — 空闲 15 秒自动休眠，RTC 定时唤醒
- **WS2812 RGB 灯光状态指示** + 蜂鸣器音效
- **SH1106 OLED 显示屏** — 状态信息显示

## 硬件平台

| 组件 | 型号/引脚 |
|------|----------|
| 主控 | ESP32-S3 N16R8（16MB Flash / 8MB PSRAM） |
| OLED | SH1106 128x64 I2C（SDA=GPIO8, SCL=GPIO21） |
| NFC | PN532 SPI（SCK=12, MISO=13, MOSI=11, SS=10） |
| 灯光 | WS2812 16 灯 RGB 环（GPIO48）+ 蜂鸣器（GPIO47） |
| 继电器 | 引擎（GPIO5）、点火 IGN（GPIO25）、启动马达 START（GPIO26）、喇叭 HORN（GPIO27） |
| 传感器 | 手刹 GPIO34、空档 GPIO33、ACC GPIO32、车门 GPIO35、电瓶电压 ADC GPIO4/36 |
| 4G 模块 | Air780EP（Serial2: GPIO16 RX / GPIO17 TX，PWRKEY=GPIO4） |

## 编译环境

- **PlatformIO** + Arduino 框架
- 分区表：双 OTA 分区（app0 6MB + app1 5.875MB）+ SPIFFS 4MB
- 依赖库：Adafruit PN532、Adafruit NeoPixel、Adafruit SH110X、Adafruit GFX、ArduinoJson、PubSubClient

## 项目结构

```
CarKey_V5/
├── src/                    # 源代码
│   ├── main.cpp            # 主入口，初始化与主循环
│   ├── BLEManager.cpp      # BLE 扫描与配对
│   ├── NFCManager.cpp      # NFC 读卡与验证
│   ├── RelayManager.cpp    # 继电器控制
│   ├── VehicleStatus.cpp   # 车辆状态检测
│   ├── WebManager.cpp      # HTTP 服务与 API
│   ├── CellularManager.cpp # 4G AT 指令引擎
│   ├── MqttManager.cpp     # MQTT 客户端
│   ├── NetworkManager.cpp  # 网络抽象层
│   ├── StatusJsonBuilder.cpp # 统一状态 JSON 构建
│   ├── OTAManager.cpp      # OTA 固件升级
│   ├── StatusLight.cpp     # RGB 灯光 + 蜂鸣器
│   ├── DisplayManager.cpp  # OLED 显示
│   ├── SleepManager.cpp    # 低功耗管理
│   ├── TaskManager.cpp     # FreeRTOS 任务调度
│   ├── StateMachine.cpp    # 车辆状态机
│   ├── Config.cpp          # 配置管理
│   ├── Metrics.cpp         # 运行指标统计
│   ├── AuthManager.cpp     # 认证管理
│   └── Logger.cpp          # 日志
├── include/                # 头文件
├── data/                   # SPIFFS Web 前端资源
├── docs/                   # 文档
├── examples/               # 示例（Air780EP 测试项目）
├── minimal_test/           # 最小化测试项目
├── test/                   # 单元测试
└── platformio.ini          # PlatformIO 配置
```

## 最近主要变更（2026-07）

### 4G 蜂窝网络 + MQTT 远程控制
- 启用 `ENABLE_CELLULAR` 功能开关
- **CellularClient** — 将 Air780EP AT 指令封装为 Arduino `Client` 接口，透明桥接上层应用
- **CellularManager** 大幅重构（+1088 行），新增 CSQ 信号质量检测、网络注册状态机、GNSS 定位
- **MqttManager** — 基于 PubSubClient 的 WiFi MQTT 客户端，支持状态/位置上报、指令接收、HMAC-SHA256 签名验证
- **NetworkManager** — 网络抽象层，WiFi / 4G 双通道自动切换，优先级 WiFi 优先、4G 回退

### 统一状态上报
- **StatusJsonBuilder** — 统一 JSON 构建器，三种模式（API/WebSocket、4G MQTT、WiFi MQTT），一处修改全局生效
- 4G MQTT 模式包含 GNSS 定位信息，WiFi MQTT 模式精简字段

### Web UI 优化
- 重构内嵌 Web 页面（`EmbeddedIndexPage.h`），重写电瓶电压显示曲线
- WebManager 增加广播机制，支持多客户端实时同步

### 运行指标增强
- 新增 MQTT 心跳 RTT 统计
- 新增 4G 信号质量（RSSI/dBm）记录
- 新增 GPS 定位成功率统计
- 新增指令验证成功率统计

### 配置增强
- `platformio.ini` 增加 4G 编译选项、PubSubClient 依赖
- `Config.h` 新增 `TEST_4G_ONLY` 纯 4G 测试模式
- 分区表更新以适配 16MB Flash

### Air780EP 调试文档
- 新增 `docs/Air780EP_Debug_Log.md` — Air780EP AT 通信调试记录
- 新增 `docs/4G_Status_Dataflow_Trace.docx` — 4G 状态数据流追踪
- 新增 `examples/Air780EP_Test` — Air780EP 独立测试项目

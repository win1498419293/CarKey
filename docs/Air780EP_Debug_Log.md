# Air780EP 4G Module Debug Log

## Problem: ESP32 cannot communicate with Air780EP (AT command no response)

### Symptoms
- ESP32 Serial2(GPIO16/17) sends AT command, Air780EP no reply
- All baud rates (9600~921600) scan timeout
- Luatools (USB direct) can communicate normally
- Receives irregular `RDY` characters (module rebooting repeatedly)

### Investigation Process

#### 1. Hardware Connection Check ✅
- GPIO16 -> Air780EP TXD (ESP32 receive)
- GPIO17 -> Air780EP RXD (ESP32 send)
- GND common ground
- Independent USB power (one USB for ESP32, one for Air780EP)

#### 2. PWRKEY Power-on ✅
- Air780EP needs PWRKEY pin pulled low for 1.5~2 seconds to power on
- Controlled via ESP32 GPIO4
- Added `pulsePwrkey()` function in test project
- PWRKEY low for 2s, then wait 3s for module boot

#### 3. Baud Rate Detection ✅ <- KEY FINDING
- Air780EP actually works at **19200 baud** (not default 115200)
- Module firmware may have been configured to 19200
- Baud rate scanning function detected 19200 successfully

### Solution
1. **CellularManager baud rate**: `Serial2.begin(115200,...)` -> `Serial2.begin(19200,...)`
2. **Add PWRKEY control**: GPIO4 output 2s pulse to power on
3. **Add baud rate auto-detection** (test project): scan 9600~921600

### Verification Results
Air780EP at 19200 baud:
- ✅ AT / ATE0 / ATI normal
- ✅ +CPIN: READY (SIM ready)
- ✅ +CSQ: 23 (signal -67dBm, excellent)
- ✅ +CEREG: 0,1 (network registered)
- ✅ +CGATT: 1 (GPRS attached)
- ✅ +CGPADDR: 10.57.9.251 (IP obtained)
- ✅ MCONFIG + MIPSTART + MCONNECT (MQTT connected)
- ⚠️ MPUB JSON quote escaping fixed

### Prevention (next time)
1. **Check PWRKEY first**: module does not auto-power-on, needs PWRKEY pulse
2. **Use Luatools to verify module**: if Luatools connects, module and SIM are fine
3. **Baud rate scan**: don't assume 115200, scan actual baud rate
4. **TX/RX wiring**: ESP32 RX(GPIO16) -> module TX, ESP32 TX(GPIO17) -> module RX
5. **Common GND required**: must connect GND when using separate power supplies


## 2026-07-13 主工程 CellularManager 修复

### 修复的问题

**问题1: PWRKEY 时序不足**
- 原来: pulsePwrkey() 只 hold 1.5s，没有模块启动等待，没有 boot message flush
- 修复: pulsePwrkey() hold 2s → 等待 3s 模块启动 → flush boot 消息
- 参考: 测试工程中 PWRKEY 2s + 等待 3s 的时序

**问题2: updateModemInit() 和 update() 的 _connStep 状态机冲突**
- 原来: updateModemInit() 使用独立的静态 initStep 状态机，
  但 update() 中的 power-on 超时判断会在 POWER_WAIT 超时后
  把 _connStep 覆盖为 IDLE，导致下次 updateModemInit() 无法识别状态
- 修复: 移除 update() 中的 power-on 超时判断，
  全部由 updateModemInit() 基于 _connStep 统一管理

**问题3: updateConnection() 二次读取 Serial2**
- 原来: updateConnection() 也直接从 Serial2 读取数据，
  与 update() 的 drain 循环形成竞争
- 修复: updateConnection() 只检查超时，_connReply 由 update() 的 drain 循环喂入

**问题4: readATLine(100) 在 19200 下超时风险**
- 修复: 增大到 200ms

**问题5: Config.h 中 ENABLE_CELLULAR 重复定义**
- 修复: 添加#ifndef/#endif 保护

### 当前状态
- esp32s3_n16r8 编译通过
- 请上传固件测试


## 2026-07-13 关键发现：波特率 115200，供电问题

### 测试工程验证通过
- Air780EP 实际工作在 **115200 波特率**（不是 19200）
- 模块自动开机，不需要 PWRKEY 脉冲
- 之前反复 ^boot.rom/RDY 重启的原因是 **供电不足**
- 两台电脑 USB 口同时供电时，电流不够导致模块欠压重启
- 用充电头单独给扩展板供电可以解决问题
- DNS 解析有问题（CME ERROR: 3），MIPSTART 必须用 IP 地址直连

### 主工程修改
1. Serial2.begin() 从 19200 改为 115200
2. 移除 PWRKEY 引脚初始化（pinMode/digitalWrite）
3. pulsePwrkey() 简化为只 flush boot 消息
4. updateModemInit() 移除波特率扫描逻辑

### MQTT 完整测试流程（已验证通过）
AT+MIPSTART="35.172.255.228",1883  → CONNECT OK
AT+MCONNECT=1,120                 → CONNACK OK
AT+MSUB="test/topic",0            → SUBACK
AT+MPUB="test/topic",0,0,"msg"    → OK

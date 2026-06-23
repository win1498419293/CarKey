================================================================
NimBLEDevice 迁移方案 — 逐 chunk 实施
================================================================

## 当前 Bluedroid API  vs  NimBLE 等价 API

### 1. 头文件 & 全局替换
┌─────────────────────────────────────────────────────────┐
│ 旧: #include <BLEDevice.h>                              │
│     #include <BLEAdvertisedDevice.h>                    │
│     #include <BLEScan.h>                                │
│     #include <BLEUtils.h>                               │
│     #include <BLEServer.h>                              │
│     #include <esp_gap_ble_api.h>                        │
│                                                         │
│ 新: #include <NimBLEDevice.h>  ← 一个头文件涵盖所有      │
└─────────────────────────────────────────────────────────┘

### 2. 类名前缀替换 (全文件 BLE → NimBLE)
┌──────────────┬──────────────────────┬──────┐
│ 旧           │ 新                   │ 难度 │
├──────────────┼──────────────────────┼──────┤
│ BLEDevice    │ NimBLEDevice         │ ★    │
│ BLEScan      │ NimBLEScan           │ ★    │
│ BLEServer    │ NimBLEServer         │ ★    │
│ BLEService   │ NimBLEService        │ ★    │
│ BLECharacteristic │ NimBLECharacteristic │★│
│ BLEAdvertising│ NimBLEAdvertising   │ ★    │
│ BLEScanResults│ NimBLEScanResults   │ ★    │
│ BLEAdvertisedDeviceCallbacks │ NimBLEAdvertisedDeviceCallbacks│★★│
│ BLEServerCallbacks │ NimBLEServerCallbacks │ ★★    │
└──────────────┴──────────────────────┴──────┘

### 3. API 差异 — 仅 3 处需要改逻辑，其余纯重命名
┌──────────────────────────────────────────────────────────┐
│ Chunk A: 广告回调 (唯一需要改逻辑的地方)                  │
│                                                          │
│  旧: void onResult(BLEAdvertisedDevice d) override       │
│       d.getAddress().toString().c_str()  // 值传递        │
│       d.getName().c_str()                                │
│       d.getRSSI()                                        │
│                                                          │
│  新: void onResult(NimBLEAdvertisedDevice* d) override   │
│       d->getAddress().toString().c_str() // 指针传递       │
│       d->getName().c_str()                               │
│       d->getRSSI()                                       │
│                                                          │
│  影响: 4 处 . → ->                                      │
├──────────────────────────────────────────────────────────┤
│ Chunk B: 配对服务回调                                     │
│                                                          │
│  旧: void onConnect(BLEServer* s,                        │
│          esp_ble_gatts_cb_param_t* param) override       │
│      String mac = bdaToString(param->connect.remote_bda) │
│                                                          │
│  新: void onConnect(NimBLEServer* s,                     │
│          NimBLEConnInfo& connInfo) override              │
│      String mac = connInfo.getAddress().toString().c_str()│
│                                                          │
│  影响: 1 处，且可删除 bdaToString() 工具函数              │
├──────────────────────────────────────────────────────────┤
│ Chunk C: BLEDevice::setPower()                           │
│                                                          │
│  旧: BLEDevice::setPower(ESP_PWR_LVL_N12)                │
│  新: NimBLEDevice::setPower(ESP_PWR_LVL_N12)             │
│       (完全兼容，仅前缀变化)                              │
└──────────────────────────────────────────────────────────┘

### 4. 无需改动的部分
  - scan->start(duration, callback, is_continue)  ← 签名相同
  - scan->setInterval()/setWindow()/setActiveScan()  ← 相同
  - scan->stop()/clearResults()  ← 相同
  - scan->setAdvertisedDeviceCallbacks(cb, true/false)  ← 相同
  - server->createService(uuid)  ← 相同
  - service->createCharacteristic(uuid, props)  ← 相同
  - char->setValue(str)  ← 相同
  - service->start()  ← 相同
  - advertising->start()/stop()  ← 相同
  - advertising->addServiceUUID/setScanResponse  ← 相同
  - advertising->setMinInterval/setMaxInterval  ← 相同
  - BLECharacteristic::PROPERTY_*  ← 常量名相同
  - scan complete callback: 签名相同

### 5. platformio.ini 改动
  删除/替换:
    当前项目未显式依赖 BLEDevice（它是 framework-arduino 自带）
  
  添加:
    lib_deps =
        h2zero/NimBLE-Arduino@^2.2.0

### 6. 实施步骤 (每个 chunk 可独立提交/验证)

  S1: 添加 NimBLE-Arduino 库依赖，注释掉旧 BLE 代码
  S2: 全局替换 BLE→NimBLE 类名前缀
  S3: 修 MyAdvertisedDeviceCallbacks::onResult (指针→成员)
  S4: 修 PairServerCallbacks::onConnect (connInfo)
  S5: 删除 bdaToString() 和 esp_gap_ble_api.h 引用
  S6: platformio.ini lib_deps 清理 & 编译验证
  S7: 烧录测试 BLE 扫描 + 配对功能

### 7. 风险点
  ┌───────────────────────────────────────────────────┐
  │ ⚠ NimBLE setPower() 的 ESP_PWR_LVL_N12 可能无效   │
  │    NimBLE 默认使用自身功率管理，建议:              │
  │    → 先不加 setPower()，测连接距离后再调           │
  │                                                   │
  │ ⚠ setActiveScan(false) 在 NimBLE 中行为略有不同    │
  │   → 不影响 CarKey 项目（本来就用被动扫描）          │
  │                                                   │
  │ ⚠ NVS 中 "carkey_bt" namespace 格式不变            │
  │   → MAC 字符串格式一致，无需迁移配对数据            │
  └───────────────────────────────────────────────────┘

#pragma once

#include <Arduino.h>

enum class VehicleCommandType : uint8_t {
    VEHICLE_CMD_START_FROM_WEB = 0,
    VEHICLE_CMD_START_FROM_MQTT,
    VEHICLE_CMD_LOCK_TOGGLE_FROM_NFC,
    AUTH_NFC_FAIL
};

struct VehicleCommand {
    VehicleCommandType type;
};

class TaskManager {
public:
    static void begin();
    static bool sendVehicleCommand(VehicleCommandType type, uint32_t timeoutMs = 0);
};

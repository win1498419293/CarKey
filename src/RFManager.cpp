#include "RFManager.h"

RFMode RFManager::currentMode = RFMode::RF_IDLE;
bool RFManager::nfcRequested = false;
int8_t RFManager::savedWifiTxPower = 78;
wifi_ps_type_t RFManager::savedWifiPsMode = WIFI_PS_NONE;
bool RFManager::savedWifiPsModeValid = false;

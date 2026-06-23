#include "AuthManager.h"
#include "Config.h"
#include <string.h>

#define UID_LENGTH 4
#define WHITELIST_SIZE 5

// NFC auth: see Config.cpp authorizedCards list
static uint8_t whitelist[WHITELIST_SIZE][UID_LENGTH] = {
  {0x57, 0x27, 0xD5, 0x41},
  {0x12, 0x16, 0x23, 0x07},
  {0x3C, 0xD9, 0x03, 0x07},
  {0x1B, 0xA9, 0x7E, 0x11},
  {0x37, 0x19, 0xED, 0x70}
};

bool AuthManager::verify(uint8_t* uid, uint8_t uidLen) {
    if (uidLen != UID_LENGTH) {
        return false;  // Need at least 4 byte UID
    }

    for (int i = 0; i < WHITELIST_SIZE; i++) {
        if (memcmp(uid, whitelist[i], UID_LENGTH) == 0) {
            return true;
        }
    }

    return false;
}

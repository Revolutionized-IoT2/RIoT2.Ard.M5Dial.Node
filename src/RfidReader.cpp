#include "RfidReader.h"

#include <M5Dial.h>

void RfidReader::setActive(bool active) {
    _active = active;
    if (_active && !_initialized) {
        Serial.println("[RFID] Configuration includes an RFID-consuming view, enabling on-device reader");
        M5Dial.Rfid.begin();
        _initialized = true;
    }
}

String RfidReader::poll() {
    if (!_active || !M5Dial.Rfid.PICC_IsNewCardPresent() || !M5Dial.Rfid.PICC_ReadCardSerial()) {
        return String();
    }

    String uid;
    for (byte i = 0; i < M5Dial.Rfid.uid.size; i++) {
        if (M5Dial.Rfid.uid.uidByte[i] < 0x10) {
            uid += "0";
        }
        uid += String(M5Dial.Rfid.uid.uidByte[i], HEX);
    }
    uid.toUpperCase();
    M5Dial.Rfid.PICC_HaltA();

    unsigned long now = millis();
    if (uid == _lastUid && (now - _lastReadMs) < kRepeatSuppressMs) {
        return String();
    }
    _lastUid = uid;
    _lastReadMs = now;
    Serial.printf("[RFID] Tag read: %s\n", uid.c_str());
    return uid;
}

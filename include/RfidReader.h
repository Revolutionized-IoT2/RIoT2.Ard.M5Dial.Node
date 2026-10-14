#pragma once

#include <Arduino.h>

// The M5Dial's built-in MFRC522 reader (M5Dial.Rfid). It is powered up only once a configuration
// has a view that consumes tag reads (RFIDView), and never powered down again.
class RfidReader {
public:
    // Repeat reads of the same tag within this window are ignored, so a card resting on the
    // reader doesn't keep re-triggering RFIDView.
    static constexpr unsigned long kRepeatSuppressMs = 3000;

    // After each configuration: whether a view wants tag reads.
    void setActive(bool active);

    // Once per loop(): the UID (upper-case hex) of a newly read tag, or "".
    String poll();

private:
    bool _initialized = false;
    bool _active = false;
    String _lastUid;
    unsigned long _lastReadMs = 0;
};

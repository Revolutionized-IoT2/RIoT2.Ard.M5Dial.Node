#pragma once

#include <Arduino.h>
#include <M5GFX.h>

#include <memory>
#include <vector>

#include <riot2/BleTypes.h>
#include <riot2/Command.h>
#include <riot2/DeviceConfiguration.h>

#include "ClockView.h"
#include "IView.h"

// Drives the on-device home carousel built from the orchestrator's
// deviceConfigurations. Styled after a vertically scrolling "belt"/coverflow
// list (M5Dial-UserDemo's app_more_menu): every view gets one row (icon +
// name + view-type subtitle), stacked vertically and scrolled smoothly as
// the dial turns, with the focused row centered at full size/brightness and
// rows further away shrinking, dimming, and bowing rightward - see
// renderCarousel() in ViewManager.cpp for the math. There's no separate
// page-position indicator (e.g. a dot strip); the list itself communicates
// position. Alert/notification-style views (IView::isAlert()) are excluded
// from this menu entirely - they're transient popups only ever shown when
// their inbound command arrives (see onCommand()), not something you dial
// or tap your way to.
// From the carousel, a view can be entered (becomes "focused" and fills the
// whole screen) three ways:
//   - rotate the encoder to scroll to an entry, then touch the content or
//     press the physical button to enter the current one; or
//   - touch the content directly, which selects and enters it immediately.
// Touching the top band moves to the previous entry; touching the bottom
// band moves to the next entry.
// While a view is focused, the encoder is forwarded to it only if it claims
// the encoder via isInteracting() (e.g. an "adjust value" submode), and
// touch is forwarded to it directly - views never need the physical button.
// The physical button is the "confirm" gesture: on the carousel it enters
// the currently highlighted entry (like touching the content); while a view
// is focused it instead returns to the home carousel via exitToCarousel().
class ViewManager {
public:
    enum class Mode { Carousel, Focused };

    // (Re)builds the carousel from a fresh NodeConfiguration. Safe to call
    // again for re-configuration pushes: replaces the whole carousel without
    // requiring a reboot. Always returns to the carousel (never leaves a
    // stale view focused).
    void rebuild(const NodeConfiguration& nodeConfiguration);

    void onReport(IView::ReportCallback callback) { _reportCallback = callback; }

    bool hasViews() const { return !_entries.empty(); }
    Mode mode() const { return _mode; }
    bool isFocused() const { return _mode == Mode::Focused && activeView() != nullptr; }

    // True if the current NodeConfiguration includes at least one view with
    // IView::consumesRfidEvents() (e.g. RFIDView). main.cpp uses this to
    // decide whether to power up/poll the node's on-device RFID reader at
    // all - it stays off when no configured view actually consumes tag
    // reads, rather than being enabled unconditionally at boot.
    bool hasRfidConsumer() const;

    // True if the current NodeConfiguration includes at least one view with
    // IView::consumesBleEvents() (e.g. BLEView). main.cpp uses this to
    // decide whether to power up the node's on-device BLE radio at all - it
    // stays off when no configured view actually consumes BLE scan events,
    // rather than being enabled unconditionally at boot (see BleScanner.h).
    bool hasBleConsumer() const;

    // Leaves the currently focused view and returns to the home carousel,
    // highlighting the view that was just exited. No-op if already showing
    // the carousel.
    void exitToCarousel();

    // Input routing, call from loop().
    void onEncoderChange(int delta);
    void onButtonPress();
    void onTouch(int x, int y);

    // Routes an inbound command to whichever view owns a commandTemplate
    // matching commandId (regardless of which view is currently focused).
    // Returns true if this caused an isAlert() view to take over the
    // display (e.g. AlertView/NotificationView) - callers use this to know
    // whether to wake the display from sleep/dim, since such a takeover
    // should always be visible rather than left on a blank/sleeping screen.
    bool onCommand(const String& commandId, const Command& command);

    // Routes a tag read from the node's on-device RFID reader to every
    // entry with IView::consumesRfidEvents() (regardless of which view is
    // currently focused), taking over the display for it just like an
    // isAlert() view does for an inbound command (see onCommand()). Returns
    // true if a takeover happened, so callers know whether to wake the
    // display from sleep/dim.
    bool notifyRfidTagRead(const String& value);

    // Routes BLE scan events from BleScanner to every entry with
    // IView::consumesBleEvents() (regardless of which view is currently
    // focused). Unlike notifyRfidTagRead(), these never take over the
    // display - BLEView is a normal carousel entry, not an alert/popup.
    void notifyBleDeviceDiscovered(const BleDeviceInfo& device);
    void notifyBleDeviceLost(const String& address);
    void notifyBleAdvertisement(const BleAdvertisement& advertisement);

    void render(M5Canvas& canvas);

private:
    struct Entry {
        DeviceConfiguration config;
        std::unique_ptr<IView> view;
    };

    // After kIdleTimeoutMs with no input, render() auto-shows a ClockView
    // idle screen; any subsequent input dismisses it (without being
    // forwarded to the underlying view) and resumes the carousel.
    static constexpr unsigned long kIdleTimeoutMs = 30000;

    // Carousel layout, all in display coordinates (M5Dial's round display is
    // a fixed 240x240, center (120,120)). Styled after the "belt"/coverflow
    // list from M5Dial-UserDemo's app_more_menu: entries are laid out in a
    // vertical scrolling list, one row per entry, smoothly scrolling as the
    // encoder turns (see _scrollPosition). The focused row sits on
    // kCenterY at full size/brightness; rows above/below shrink, dim, and
    // bow rightward (kArcAmplitude) the further they are from center - see
    // renderCarousel() in ViewManager.cpp for the math. There's no separate
    // page-position indicator (e.g. dot strip) any more; the list itself
    // communicates position.
    static constexpr int kCenterY = 120;          // focused row's vertical center
    static constexpr int kItemSpacingY = 54;      // vertical distance between adjacent rows
    static constexpr int kIconColumnX = 34;       // icon center x for a row with no arc shift
    static constexpr int kIconRadiusFocused = 19;
    static constexpr int kIconRadiusMin = 10;
    static constexpr int kArcAmplitude = 46;       // max rightward shift for the furthest visible rows
    static constexpr int kArcRange = 130;          // pixel offset from center at which the arc shift maxes out
    static constexpr int kMaxTextWidth = 160;
    static constexpr float kScrollEase = 0.35f;    // 0..1 smoothing factor applied to _scrollPosition each frame

    // Touch is split into three horizontal bands rather than precise
    // per-element hit circles - easier to hit accurately on a small round
    // touch panel: top band = previous entry, bottom band = next entry,
    // everything in between = enter the current entry.
    static constexpr int kPrevBandBottomY = 50;
    static constexpr int kNextBandTopY = 172;

    std::vector<Entry> _entries;
    size_t _activeIndex = 0;

    // Indices into _entries for entries that appear in the navigable home
    // carousel - alert/notification-style entries (IView::isAlert()) are
    // excluded here since they're popups triggered only by an inbound
    // command (see onCommand()), never menu items you dial/tap to. Rebuilt
    // alongside _entries in rebuild(). _menuPosition is the highlighted
    // position *within this list* (not an _entries index); moveHighlight()
    // and renderCarousel() operate on it, and enterFocused() is always
    // called with _menuIndices[_menuPosition] to resolve it to a real
    // _entries index.
    std::vector<size_t> _menuIndices;
    size_t _menuPosition = 0;

    Mode _mode = Mode::Carousel;
    IView::ReportCallback _reportCallback;
    ClockView _idleView;
    unsigned long _lastInputMs = 0;
    bool _idle = false;

    // Continuous (fractional) version of _activeIndex used purely for the
    // carousel's scroll animation - eases toward _activeIndex every
    // renderCarousel() call, taking the shortest wraparound path so looping
    // past the first/last entry animates smoothly instead of sweeping
    // across the whole list.
    float _scrollPosition = 0.0f;

    IView* activeView() const;
    void moveHighlight(int direction);
    void enterFocused(size_t index);

    void renderCarousel(M5Canvas& canvas);
};


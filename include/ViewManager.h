#pragma once

#include <Arduino.h>
#include <M5GFX.h>

#include <memory>
#include <vector>

#include "ClockView.h"
#include "Command.h"
#include "DeviceConfiguration.h"
#include "IView.h"

// Drives the on-device home carousel built from the orchestrator's
// deviceConfigurations. Styled after compact round-display "smart button"
// menu UIs (small colored icon + left-aligned title/subtitle, dimmed peeks
// of the previous/next entries' icons above/below the current one, and a
// vertical dot strip along the right edge showing page position with the
// current entry as a diamond marker). Only ONE entry is shown at a time,
// which scales to many views far better than a ring of icons (which gets
// crowded/overlapping past a handful of entries).
// From the carousel, a view can be entered (becomes "focused" and fills the
// whole screen) two ways:
//   - rotate the encoder to page through entries, then touch the content to
//     enter the current one; or
//   - touch the content directly, which selects and enters it immediately.
// Touching the top band moves to the previous entry; touching the bottom
// band moves to the next entry.
// While a view is focused, the encoder is forwarded to it only if it claims
// the encoder via isInteracting() (e.g. an "adjust value" submode), and
// touch is forwarded to it directly - views never need the physical button.
// The physical button is reserved exclusively as the "go back to the home
// carousel" gesture: a press while a view is focused always calls
// exitToCarousel() (it's a no-op while already showing the carousel).
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
    void onCommand(const String& commandId, const Command& command);

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
    // a fixed 240x240, center (120,120)). Chosen so every element stays
    // within the round panel's visible circular area - see renderCarousel()
    // in ViewManager.cpp for the math.
    static constexpr int kIconCenterX = 78;   // small colored icon glyph, upper-left of the text
    static constexpr int kIconCenterY = 100;
    static constexpr int kIconRadius = 17;
    static constexpr int kTextX = 48;  // left edge of the title/subtitle text block
    static constexpr int kTitleY = 134;
    static constexpr int kSubtitleY = 156;
    static constexpr int kNextPeekCenterX = 78;  // dimmed peek of the *next* entry's icon
    static constexpr int kNextPeekCenterY = 194;
    static constexpr int kNextPeekRadius = 12;
    static constexpr int kPrevPeekCenterX = 78;  // dimmed peek of the *previous* entry's icon
    static constexpr int kPrevPeekCenterY = 32;
    static constexpr int kPrevPeekRadius = 12;
    static constexpr int kDotsX = 210;  // vertical page-position dot strip, right edge
    static constexpr int kDotsCenterY = 128;
    static constexpr int kDotRadius = 3;
    static constexpr int kDotSpacing = 18;
    static constexpr size_t kMaxVisibleDots = 5;

    // Touch is split into three horizontal bands rather than precise
    // per-element hit circles - easier to hit accurately on a small round
    // touch panel: top band = previous entry, bottom band = next entry,
    // everything in between = enter the current entry.
    static constexpr int kPrevBandBottomY = 50;
    static constexpr int kNextBandTopY = 172;

    std::vector<Entry> _entries;
    size_t _activeIndex = 0;
    Mode _mode = Mode::Carousel;
    IView::ReportCallback _reportCallback;
    ClockView _idleView;
    unsigned long _lastInputMs = 0;
    bool _idle = false;

    IView* activeView() const;
    void moveHighlight(int direction);
    void enterFocused(size_t index);

    void renderCarousel(M5Canvas& canvas);
};


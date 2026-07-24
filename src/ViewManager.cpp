#include "ViewManager.h"

#include "Icons.h"
#include "ViewColors.h"
#include "ViewFactory.h"

constexpr unsigned long ViewManager::kIdleTimeoutMs;
constexpr int ViewManager::kIconCenterX;
constexpr int ViewManager::kIconCenterY;
constexpr int ViewManager::kIconRadius;
constexpr int ViewManager::kTextX;
constexpr int ViewManager::kTitleY;
constexpr int ViewManager::kSubtitleY;
constexpr int ViewManager::kNextPeekCenterX;
constexpr int ViewManager::kNextPeekCenterY;
constexpr int ViewManager::kNextPeekRadius;
constexpr int ViewManager::kPrevPeekCenterX;
constexpr int ViewManager::kPrevPeekCenterY;
constexpr int ViewManager::kPrevPeekRadius;
constexpr int ViewManager::kDotsX;
constexpr int ViewManager::kDotsCenterY;
constexpr int ViewManager::kDotRadius;
constexpr int ViewManager::kDotSpacing;
constexpr size_t ViewManager::kMaxVisibleDots;
constexpr int ViewManager::kPrevBandBottomY;
constexpr int ViewManager::kNextBandTopY;

namespace {
// Icon drawn inside a view's icon bubble, chosen per view type from a real
// PNG icon asset (Assets/icons/*.png, embedded as byte arrays in Icons.cpp)
// instead of a hand-drawn vector glyph. Each icon PNG already bakes in its
// view's Primary color as a filled circle background with a white glyph on
// top, so `icon`/`iconLen` alone fully determine what's drawn - `r`/`g`/`b`
// (the Primary color, see ViewColors.h) is only used for the title text
// color and, for unstyled fallback entries (no icon asset registered), a
// plain colored dot.
struct ViewStyle {
    uint8_t r, g, b;
    const uint8_t* icon;
    size_t iconLen;
};

// One distinct, vividly-saturated color + icon per known view type, keyed by
// classFullName - deliberately avoids near-black/near-gray colors (which can
// read as "no icon"/background on the panel).
const ViewStyle& styleForClassFullName(const String& classFullName, size_t fallbackIndex) {
    static const ViewStyle kButton = {ViewColors::Button.r, ViewColors::Button.g, ViewColors::Button.b,
                                       Icons::kButtonPng, Icons::kButtonPngLen};
    static const ViewStyle kColorScheme = {ViewColors::ColorScheme.r, ViewColors::ColorScheme.g,
                                            ViewColors::ColorScheme.b, Icons::kColorSchemePng,
                                            Icons::kColorSchemePngLen};
    static const ViewStyle kValue = {ViewColors::Value.r, ViewColors::Value.g, ViewColors::Value.b, Icons::kValuePng,
                                      Icons::kValuePngLen};
    static const ViewStyle kPercentage = {ViewColors::Percentage.r, ViewColors::Percentage.g,
                                           ViewColors::Percentage.b, Icons::kPercentagePng,
                                           Icons::kPercentagePngLen};
    static const ViewStyle kToggle = {ViewColors::Toggle.r, ViewColors::Toggle.g, ViewColors::Toggle.b,
                                       Icons::kTogglePng, Icons::kTogglePngLen};
    static const ViewStyle kSlider = {ViewColors::Slider.r, ViewColors::Slider.g, ViewColors::Slider.b,
                                       Icons::kSliderPng, Icons::kSliderPngLen};
    static const ViewStyle kScene = {ViewColors::Scene.r, ViewColors::Scene.g, ViewColors::Scene.b, Icons::kScenePng,
                                      Icons::kScenePngLen};
    static const ViewStyle kClock = {ViewColors::Clock.r, ViewColors::Clock.g, ViewColors::Clock.b, Icons::kClockPng,
                                      Icons::kClockPngLen};
    static const ViewStyle kAlert = {ViewColors::Alert.r, ViewColors::Alert.g, ViewColors::Alert.b, Icons::kAlertPng,
                                      Icons::kAlertPngLen};
    static const ViewStyle kNotification = {ViewColors::Notification.r, ViewColors::Notification.g,
                                             ViewColors::Notification.b, Icons::kNotificationPng,
                                             Icons::kNotificationPngLen};
    static const ViewStyle kTimer = {ViewColors::Timer.r, ViewColors::Timer.g, ViewColors::Timer.b, Icons::kTimerPng,
                                      Icons::kTimerPngLen};

    // Vivid fallback palette (no black/gray) for any classFullName not
    // explicitly styled above, cycled by the entry's position. No icon
    // asset applies, so a plain colored dot is drawn instead (see
    // drawViewIcon()).
    static const ViewStyle kFallback[] = {
        {40, 110, 240, nullptr, 0}, {40, 180, 90, nullptr, 0},  {235, 140, 20, nullptr, 0},
        {190, 60, 220, nullptr, 0}, {20, 175, 175, nullptr, 0}, {220, 60, 95, nullptr, 0},
    };
    constexpr size_t kFallbackCount = sizeof(kFallback) / sizeof(kFallback[0]);

    if (classFullName == "RIoT2.Ard.M5Dial.Node.ButtonView") return kButton;
    if (classFullName == "RIoT2.Ard.M5Dial.Node.ColorSchemeView") return kColorScheme;
    if (classFullName == "RIoT2.Ard.M5Dial.Node.ValueView") return kValue;
    if (classFullName == "RIoT2.Ard.M5Dial.Node.PercentageView") return kPercentage;
    if (classFullName == "RIoT2.Ard.M5Dial.Node.ToggleView") return kToggle;
    if (classFullName == "RIoT2.Ard.M5Dial.Node.SliderView") return kSlider;
    if (classFullName == "RIoT2.Ard.M5Dial.Node.SceneSelectorView") return kScene;
    if (classFullName == "RIoT2.Ard.M5Dial.Node.ClockView") return kClock;
    if (classFullName == "RIoT2.Ard.M5Dial.Node.AlertView") return kAlert;
    if (classFullName == "RIoT2.Ard.M5Dial.Node.NotificationView") return kNotification;
    if (classFullName == "RIoT2.Ard.M5Dial.Node.TimerView") return kTimer;
    return kFallback[fallbackIndex % kFallbackCount];
}

// One-line fallback description per view type, used as the card's subtitle
// when the device configuration doesn't supply its own `subHeader`
// deviceParameter.
String defaultSubtitleForClassFullName(const String& classFullName) {
    if (classFullName == "RIoT2.Ard.M5Dial.Node.ButtonView") return "Buttons";
    if (classFullName == "RIoT2.Ard.M5Dial.Node.ColorSchemeView") return "Color picker";
    if (classFullName == "RIoT2.Ard.M5Dial.Node.ValueView") return "Values";
    if (classFullName == "RIoT2.Ard.M5Dial.Node.PercentageView") return "Percentage";
    if (classFullName == "RIoT2.Ard.M5Dial.Node.ToggleView") return "Switch";
    if (classFullName == "RIoT2.Ard.M5Dial.Node.SliderView") return "Slider";
    if (classFullName == "RIoT2.Ard.M5Dial.Node.SceneSelectorView") return "Scenes";
    if (classFullName == "RIoT2.Ard.M5Dial.Node.ClockView") return "Clock";
    if (classFullName == "RIoT2.Ard.M5Dial.Node.AlertView") return "Alert";
    if (classFullName == "RIoT2.Ard.M5Dial.Node.NotificationView") return "Notification";
    if (classFullName == "RIoT2.Ard.M5Dial.Node.TimerView") return "Timer";
    return "Tap to open";
}

// Native pixel size (square) of the icon PNGs in Assets/icons/.
constexpr int kIconNativeSize = 42;

// Draws a view's icon centered at (cx, cy), scaled so it roughly fills a
// circle of radius `r`. When an icon PNG is available (`style.icon` set),
// it's drawn at full brightness - it already bakes in the view's Primary
// color as its own circle background, so no separate background fill is
// needed, and its baked-in pixel colors can't be dimmed without decoding
// it. `dim` (0-1) is therefore only applied to the plain-dot fallback used
// by unstyled entries, muting its color for the carousel's dimmed prev/next
// peeks (which rely on `r` alone being smaller than the focused icon).
void drawViewIcon(M5Canvas& canvas, const ViewStyle& style, int cx, int cy, int r, float dim = 1.0f) {
    if (style.icon != nullptr) {
        // NOTE: intentionally NOT using datum=middle_center here. M5GFX's
        // image datum centering falls back to the *whole canvas* size
        // (240x240) whenever maxWidth/maxHeight are left at 0, so
        // "center within box" ends up centering the small ~34px icon
        // inside a 240px box and offsetting it ~100px right/down instead
        // of leaving it at (cx, cy) - drawing icons far from their
        // intended spot. Computing the top-left corner ourselves and using
        // the default top_left datum sidesteps that box-centering math
        // entirely.
        float scale = (r * 2.0f) / kIconNativeSize;
        int scaledSize = static_cast<int>(kIconNativeSize * scale);
        int drawX = cx - scaledSize / 2;
        int drawY = cy - scaledSize / 2;
        canvas.drawPng(style.icon, style.iconLen, drawX, drawY, 0, 0, 0, 0, scale, scale);
        return;
    }

    uint16_t fillColor = canvas.color565(static_cast<uint8_t>(style.r * dim), static_cast<uint8_t>(style.g * dim),
                                          static_cast<uint8_t>(style.b * dim));
    canvas.fillCircle(cx, cy, r, fillColor);
    canvas.fillCircle(cx, cy, r * 0.3f, BLACK);
}

// Draws `text` left-aligned starting at (x, y), starting at `startSize` and
// shrinking down to size 1 (and, as a last resort, truncating with an
// ellipsis) so it never grows wider than `maxWidth` and spills over
// neighboring elements (used for the carousel's title/subtitle text block).
void drawFittedLeftText(M5Canvas& canvas, const String& text, int x, int y, int maxWidth, uint16_t color,
                        int startSize = 2) {
    canvas.setTextColor(color);
    canvas.setTextDatum(middle_left);

    canvas.setTextSize(startSize);
    if (canvas.textWidth(text) <= maxWidth) {
        canvas.drawString(text, x, y);
        return;
    }

    if (startSize > 1) {
        canvas.setTextSize(1);
        if (canvas.textWidth(text) <= maxWidth) {
            canvas.drawString(text, x, y);
            return;
        }
    }

    String truncated = text;
    while (truncated.length() > 1 && canvas.textWidth(truncated + "..") > maxWidth) {
        truncated.remove(truncated.length() - 1);
    }
    canvas.drawString(truncated + "..", x, y);
}

// Small filled diamond (rotated square) used to mark the current entry in
// the vertical page-position dot strip, distinguishing it from the plain
// round dots for the other entries.
void drawDiamond(M5Canvas& canvas, int cx, int cy, int r, uint16_t color) {
    canvas.fillTriangle(cx, cy - r, cx - r, cy, cx + r, cy, color);
    canvas.fillTriangle(cx - r, cy, cx + r, cy, cx, cy + r, color);
}
}  // namespace

void ViewManager::rebuild(const NodeConfiguration& nodeConfiguration) {
    if (IView* current = activeView()) {
        current->onExit();
    }

    _entries.clear();
    _activeIndex = 0;
    _mode = Mode::Carousel;

    for (const auto& deviceConfig : nodeConfiguration.deviceConfigurations) {
        auto view = ViewFactory::instance().create(deviceConfig.classFullName);
        if (!view) {
            Serial.printf("[ViewManager] No view registered for classFullName=%s (name=%s), skipping\n",
                          deviceConfig.classFullName.c_str(), deviceConfig.name.c_str());
            continue;
        }

        view->setReportCallback(_reportCallback);
        view->begin(deviceConfig);

        Entry entry;
        entry.config = deviceConfig;
        entry.view = std::move(view);
        _entries.push_back(std::move(entry));
    }

    Serial.printf("[ViewManager] Rebuilt carousel with %u view(s)\n", static_cast<unsigned>(_entries.size()));

    _idleView.begin(DeviceConfiguration{});
    _lastInputMs = millis();
    _idle = false;

    // Always land on the home carousel after a (re)build - never leaves a
    // stale view focused, since the entry that used to be focused may no
    // longer exist post-reconfiguration.
}

IView* ViewManager::activeView() const {
    if (_entries.empty() || _activeIndex >= _entries.size()) {
        return nullptr;
    }
    return _entries[_activeIndex].view.get();
}

void ViewManager::moveHighlight(int direction) {
    if (_entries.size() < 2) {
        return;
    }

    size_t count = _entries.size();
    _activeIndex = (_activeIndex + static_cast<size_t>(direction > 0 ? 1 : count - 1)) % count;
}

void ViewManager::enterFocused(size_t index) {
    if (index >= _entries.size()) {
        return;
    }

    if (IView* current = activeView()) {
        if (_mode == Mode::Focused) {
            current->onExit();
        }
    }

    _activeIndex = index;
    _mode = Mode::Focused;

    if (IView* view = activeView()) {
        view->onEnter();
    }
}

void ViewManager::exitToCarousel() {
    if (_mode != Mode::Focused) {
        return;
    }

    if (IView* current = activeView()) {
        current->onExit();
    }
    _mode = Mode::Carousel;
    _lastInputMs = millis();
}

void ViewManager::onEncoderChange(int delta) {
    _lastInputMs = millis();
    if (_idle) {
        _idle = false;  // first input after idle just wakes the screen
        return;
    }

    if (delta == 0 || _entries.empty()) {
        return;
    }

    if (_mode == Mode::Carousel) {
        moveHighlight(delta);
        return;
    }

    IView* view = activeView();
    if (view && view->isInteracting()) {
        view->onEncoderChange(delta);
    }
}

void ViewManager::onButtonPress() {
    _lastInputMs = millis();
    if (_idle) {
        _idle = false;
        return;
    }

    if (_entries.empty()) {
        return;
    }

    // The physical button is reserved exclusively for "go back to the home
    // carousel" - views never see button presses (they're driven by touch
    // and the bezel instead). Pressing it while already on the carousel is
    // a no-op, since there's nowhere further back to go.
    if (_mode == Mode::Focused) {
        exitToCarousel();
    }
}

void ViewManager::onTouch(int x, int y) {
    _lastInputMs = millis();
    if (_idle) {
        _idle = false;
        return;
    }

    if (_entries.empty()) {
        return;
    }

    if (_mode == Mode::Carousel) {
        // Three horizontal touch bands: top = previous entry, bottom = next
        // entry, everything else (the content itself) = enter the current
        // entry. Easier to hit reliably on a small round touch panel than
        // precise per-element hit circles.
        if (y < kPrevBandBottomY) {
            moveHighlight(-1);
        } else if (y > kNextBandTopY) {
            moveHighlight(1);
        } else {
            enterFocused(_activeIndex);
        }
        return;
    }

    // The unified header drawn on top of every focused view is a pure
    // visual identity cue, not a touch target - going back to the carousel
    // is button-only (see class comment). Touch always goes straight to the
    // focused view.
    if (IView* view = activeView()) {
        view->onTouch(x, y);
    }
}


void ViewManager::onCommand(const String& commandId, const Command& command) {
    for (size_t i = 0; i < _entries.size(); ++i) {
        Entry& entry = _entries[i];
        for (const auto& cmdTemplate : entry.config.commandTemplates) {
            if (cmdTemplate.id == commandId) {
                entry.view->onCommand(command);

                // Alert-like views (AlertView/NotificationView) interrupt
                // whatever is currently shown as soon as their command
                // arrives, instead of waiting for the user to dial/tap their
                // way to them.
                if (entry.view->isAlert() && !(_mode == Mode::Focused && _activeIndex == i)) {
                    enterFocused(i);
                    _idle = false;
                    _lastInputMs = millis();
                }
                return;
            }
        }
    }
    Serial.printf("[ViewManager] No view owns commandTemplate id=%s\n", commandId.c_str());
}

void ViewManager::renderCarousel(M5Canvas& canvas) {
    canvas.fillScreen(BLACK);

    size_t count = _entries.size();
    if (count == 0) {
        return;
    }

    uint16_t dimGray = canvas.color565(90, 90, 90);

    // Current entry: icon glyph knocked out (in black) of a filled circle in
    // the view's assigned color, beside a left-aligned title/subtitle block.
    const auto& activeEntry = _entries[_activeIndex];
    const ViewStyle& style = styleForClassFullName(activeEntry.config.classFullName, _activeIndex);
    uint16_t styleColor = canvas.color565(style.r, style.g, style.b);

    drawViewIcon(canvas, style, kIconCenterX, kIconCenterY, kIconRadius);

    int maxTextWidth = kDotsX - kTextX - 14;
    drawFittedLeftText(canvas, activeEntry.config.name, kTextX, kTitleY, maxTextWidth, styleColor);

    String subtitle = findParameter(activeEntry.config.deviceParameters, "subHeader", "");
    if (subtitle.length() == 0) {
        subtitle = defaultSubtitleForClassFullName(activeEntry.config.classFullName);
    }
    if (subtitle.length() > 0) {
        drawFittedLeftText(canvas, subtitle, kTextX, kSubtitleY, maxTextWidth, dimGray, 1);
    }

    // Dimmed peek of the *next* entry's icon below the current content, a
    // quieter hint than the previous design's full peek bubble.
    if (count > 1) {
        size_t nextIndex = (_activeIndex + 1) % count;
        const ViewStyle& nextStyle = styleForClassFullName(_entries[nextIndex].config.classFullName, nextIndex);
        drawViewIcon(canvas, nextStyle, kNextPeekCenterX, kNextPeekCenterY, kNextPeekRadius, 1.0f / 3.0f);
    }

    // Mirrors the next-entry peek above the current content, so turning the
    // bezel counter-clockwise (toward the previous entry) has the same
    // preview hint as turning it clockwise.
    if (count > 1) {
        size_t prevIndex = (_activeIndex + count - 1) % count;
        const ViewStyle& prevStyle = styleForClassFullName(_entries[prevIndex].config.classFullName, prevIndex);
        drawViewIcon(canvas, prevStyle, kPrevPeekCenterX, kPrevPeekCenterY, kPrevPeekRadius, 1.0f / 3.0f);
    }

    // Vertical page-position dot strip along the right edge: a sliding
    // window centered on the current entry so this stays legible/scales to
    // many views, instead of one dot per entry (which would overflow the
    // strip's height once there are more than a handful of entries). The
    // current entry is drawn as a diamond instead of a plain dot.
    size_t visible = count < kMaxVisibleDots ? count : kMaxVisibleDots;
    size_t windowStart = 0;
    if (count > visible) {
        size_t half = visible / 2;
        if (_activeIndex > half) {
            windowStart = _activeIndex - half;
        }
        if (windowStart + visible > count) {
            windowStart = count - visible;
        }
    }

    int totalHeight = static_cast<int>((visible - 1) * kDotSpacing);
    int startY = kDotsCenterY - totalHeight / 2;
    for (size_t i = 0; i < visible; ++i) {
        size_t entryIndex = windowStart + i;
        int dotY = startY + static_cast<int>(i) * kDotSpacing;
        if (entryIndex == _activeIndex) {
            drawDiamond(canvas, kDotsX, dotY, kDotRadius + 2, WHITE);
        } else {
            canvas.fillCircle(kDotsX, dotY, kDotRadius, dimGray);
        }
    }
}

void ViewManager::render(M5Canvas& canvas) {
    if (_entries.empty()) {
        canvas.fillScreen(BLACK);
        canvas.setTextColor(WHITE);
        canvas.setTextDatum(middle_center);
        canvas.setTextSize(2);
        canvas.drawString("No views", canvas.width() / 2, canvas.height() / 2);
        return;
    }

    if (!_idle && millis() - _lastInputMs >= kIdleTimeoutMs) {
        _idle = true;
    }

    if (_idle) {
        _idleView.render(canvas);
        return;
    }

    if (_mode == Mode::Focused) {
        if (IView* view = activeView()) {
            // Lets a transient alert/notification ask to return to the
            // carousel on its own (acknowledged via tap, or auto-dismissed
            // after a timeout) without needing the physical button.
            if (view->wantsExit()) {
                exitToCarousel();
            }
        }
    }

    if (_mode == Mode::Carousel) {
        renderCarousel(canvas);
        return;
    }

    if (IView* view = activeView()) {
        view->render(canvas);
    }
}

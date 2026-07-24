#include "ViewManager.h"

#include <cmath>

#include "Icons.h"
#include "ViewColors.h"
#include "ViewFactory.h"

constexpr unsigned long ViewManager::kIdleTimeoutMs;
constexpr int ViewManager::kCenterY;
constexpr int ViewManager::kItemSpacingY;
constexpr int ViewManager::kIconColumnX;
constexpr int ViewManager::kIconRadiusFocused;
constexpr int ViewManager::kIconRadiusMin;
constexpr int ViewManager::kArcAmplitude;
constexpr int ViewManager::kArcRange;
constexpr int ViewManager::kMaxTextWidth;
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
// stepping down one size at a time until it fits within `maxWidth` (and, as
// a last resort, truncating with an ellipsis at size 1) so it never spills
// over neighboring elements (used for the carousel's title/subtitle text
// block). Stepping down one size at a time (rather than jumping straight to
// size 1) matters now that the focused row's title uses a larger
// `startSize` - a merely-long name should drop to a still-readable
// intermediate size instead of shrinking all the way down.
void drawFittedLeftText(M5Canvas& canvas, const String& text, int x, int y, int maxWidth, uint16_t color,
                        int startSize = 2) {
    canvas.setTextColor(color);
    canvas.setTextDatum(middle_left);

    for (int size = startSize; size >= 1; --size) {
        canvas.setTextSize(size);
        if (canvas.textWidth(text) <= maxWidth) {
            canvas.drawString(text, x, y);
            return;
        }
    }

    canvas.setTextSize(1);
    String truncated = text;
    while (truncated.length() > 1 && canvas.textWidth(truncated + "..") > maxWidth) {
        truncated.remove(truncated.length() - 1);
    }
    canvas.drawString(truncated + "..", x, y);
}

// Lays out the focused row's title, preferring the largest single-line size
// (from 4 down to 2) that fits within maxWidth. If the name is too long to
// fit on one line even at the smallest "big" size, it's wrapped onto a
// second line (split at a space) instead of shrinking further, so long
// names still render large rather than collapsing down to a tiny single
// line - only pathologically long/unsplittable names fall back to a small
// ellipsis-truncated single line. Returns 1 or 2 lines via outLines and the
// chosen font size via outSize.
void layoutFocusedTitle(M5Canvas& canvas, const String& text, int maxWidth, std::vector<String>& outLines,
                         int& outSize) {
    for (int size = 4; size >= 2; --size) {
        canvas.setTextSize(size);
        if (canvas.textWidth(text) <= maxWidth) {
            outLines.assign(1, text);
            outSize = size;
            return;
        }
    }

    for (int size = 3; size >= 2; --size) {
        canvas.setTextSize(size);
        int bestSplit = -1;
        for (int sp = 0; sp < static_cast<int>(text.length()); ++sp) {
            if (text[sp] != ' ') continue;
            String line1 = text.substring(0, sp);
            String line2 = text.substring(sp + 1);
            if (canvas.textWidth(line1) <= maxWidth && canvas.textWidth(line2) <= maxWidth) {
                bestSplit = sp;
            }
        }
        if (bestSplit >= 0) {
            outLines = {text.substring(0, bestSplit), text.substring(bestSplit + 1)};
            outSize = size;
            return;
        }
    }

    canvas.setTextSize(1);
    String truncated = text;
    while (truncated.length() > 1 && canvas.textWidth(truncated + "..") > maxWidth) {
        truncated.remove(truncated.length() - 1);
    }
    outLines.assign(1, truncated + "..");
    outSize = 1;
}

}  // namespace

void ViewManager::rebuild(const NodeConfiguration& nodeConfiguration) {
    if (IView* current = activeView()) {
        current->onExit();
    }

    _entries.clear();
    _activeIndex = 0;
    _mode = Mode::Carousel;
    _scrollPosition = 0.0f;

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

    // Alert/notification-style entries are popups triggered only by an
    // inbound command (see onCommand()'s isAlert() handling) - they never
    // appear as items in the navigable home carousel.
    _menuIndices.clear();
    for (size_t i = 0; i < _entries.size(); ++i) {
        if (!_entries[i].view->isAlert()) {
            _menuIndices.push_back(i);
        }
    }
    _menuPosition = 0;

    Serial.printf("[ViewManager] Rebuilt carousel with %u view(s), %u in menu\n",
                  static_cast<unsigned>(_entries.size()), static_cast<unsigned>(_menuIndices.size()));

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
    if (_menuIndices.size() < 2) {
        return;
    }

    size_t count = _menuIndices.size();
    _menuPosition = (_menuPosition + static_cast<size_t>(direction > 0 ? 1 : count - 1)) % count;
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

    // The physical button is the "confirm" gesture: on the carousel it
    // enters the currently highlighted entry (same as touching the
    // content); while a View is focused it instead returns to the home
    // carousel - views never see button presses themselves (they're driven
    // by touch and the bezel instead).
    if (_mode == Mode::Focused) {
        exitToCarousel();
    } else if (!_menuIndices.empty()) {
        enterFocused(_menuIndices[_menuPosition]);
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
        } else if (!_menuIndices.empty()) {
            enterFocused(_menuIndices[_menuPosition]);
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

    size_t count = _menuIndices.size();
    if (count == 0) {
        return;
    }

    float countF = static_cast<float>(count);

    // Ease _scrollPosition toward _menuPosition, taking the shortest
    // wraparound path (e.g. going from the last entry to entry 0 animates
    // forward by one step instead of sweeping backward across the whole
    // list) - see the class comment on _scrollPosition.
    float diff = static_cast<float>(_menuPosition) - _scrollPosition;
    diff -= countF * roundf(diff / countF);
    _scrollPosition += diff * kScrollEase;
    _scrollPosition = fmodf(_scrollPosition, countF);
    if (_scrollPosition < 0) {
        _scrollPosition += countF;
    }

    // Belt/coverflow-style vertical list (styled after M5Dial-UserDemo's
    // app_more_menu): every entry gets a row that scrolls smoothly as the
    // encoder turns. The focused row sits at kCenterY at full size and
    // brightness; rows further away shrink, dim, and bow to the right
    // (kArcAmplitude), fading toward the round panel's edge instead of
    // being hard-clipped.
    for (size_t i = 0; i < count; ++i) {
        float raw = static_cast<float>(i) - _scrollPosition;
        raw -= countF * roundf(raw / countF);

        int rowY = kCenterY + static_cast<int>(roundf(raw * kItemSpacingY));
        if (rowY < -kIconRadiusFocused * 2 || rowY > canvas.height() + kIconRadiusFocused * 2) {
            continue;  // fully outside the panel, nothing to draw
        }

        float t = (raw * kItemSpacingY) / static_cast<float>(kArcRange);
        if (t > 1.0f) t = 1.0f;
        if (t < -1.0f) t = -1.0f;
        float absT = fabsf(t);

        int arcShift = static_cast<int>(kArcAmplitude * (1.0f - cosf(absT * (PI / 2.0f))));
        float brightness = 1.0f - 0.7f * absT;
        if (brightness < 0.2f) brightness = 0.2f;
        bool focused = absT < 0.12f;
        int iconRadius = static_cast<int>(kIconRadiusFocused - (kIconRadiusFocused - kIconRadiusMin) * absT);

        const Entry& entry = _entries[_menuIndices[i]];
        const ViewStyle& style = styleForClassFullName(entry.config.classFullName, _menuIndices[i]);

        int iconX = kIconColumnX + arcShift;
        int textX = iconX + iconRadius + 12;
        int maxTextWidth = kMaxTextWidth - arcShift;
        if (maxTextWidth < 40) maxTextWidth = 40;

        drawViewIcon(canvas, style, iconX, rowY, iconRadius, brightness);

        uint8_t gray = static_cast<uint8_t>(255 * brightness);
        uint16_t titleColor = canvas.color565(gray, gray, gray);

        if (focused) {
            // The focused row is alone (no neighboring rows fighting for the
            // same horizontal band), so let its title use the full width
            // remaining to the panel's edge - rather than the same
            // arc-shrunk `maxTextWidth` budget shared rows use - and wrap
            // onto a second line rather than shrinking when a name is too
            // long for one line, so it's always rendered as big as the
            // screen allows.
            int maxWidthFocused = canvas.width() - textX - 6;

            std::vector<String> titleLines;
            int titleSize;
            layoutFocusedTitle(canvas, entry.config.name, maxWidthFocused, titleLines, titleSize);

            canvas.setTextDatum(middle_left);
            canvas.setTextColor(titleColor);
            canvas.setTextSize(titleSize);

            int titleBaseY = rowY - 13;
            int subtitleY = rowY + 17;
            if (titleLines.size() == 1) {
                canvas.drawString(titleLines[0], textX, titleBaseY);
            } else {
                int lineHeight = 8 * titleSize;
                int half = lineHeight / 2 + 2;
                canvas.drawString(titleLines[0], textX, titleBaseY - half);
                canvas.drawString(titleLines[1], textX, titleBaseY + half);
                subtitleY = titleBaseY + half + lineHeight / 2 + 10;
            }

            String subtitle = findParameter(entry.config.deviceParameters, "subHeader", "");
            if (subtitle.length() == 0) {
                subtitle = defaultSubtitleForClassFullName(entry.config.classFullName);
            }
            if (subtitle.length() > 0) {
                uint16_t subtitleColor = canvas.color565(90, 90, 90);
                drawFittedLeftText(canvas, subtitle, textX, subtitleY, maxWidthFocused, subtitleColor, 1);
            }
        } else {
            // Unfocused rows always render at the same small, fixed size
            // (no per-row growing/shrinking) so the whole scrolled list
            // reads as a uniform, tiny "list" contrasted against the single
            // large focused row.
            drawFittedLeftText(canvas, entry.config.name, textX, rowY, maxTextWidth, titleColor, 1);
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

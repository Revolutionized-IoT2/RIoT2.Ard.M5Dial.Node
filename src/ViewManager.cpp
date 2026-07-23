#include "ViewManager.h"

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
// Simple vector glyph drawn inside a view's icon bubble, chosen per view
// type so each is recognizable at a glance instead of a two-letter label.
// Each named entry (other than Generic) corresponds to the Google Material
// Symbols icon (https://fonts.google.com/icons) assigned to that view type:
// power_settings_new, colors, monitoring, percent, toggle_on, sliders, scene,
// schedule.
enum class IconGlyph { Generic, Power, Colors, Monitoring, Percent, ToggleOn, Sliders, Scene, Schedule };

struct ViewStyle {
    uint8_t r, g, b;
    IconGlyph icon;
};

// One distinct, vividly-saturated color + icon per known view type, keyed by
// classFullName - deliberately avoids near-black/near-gray colors (which can
// read as "no icon"/background on the panel).
const ViewStyle& styleForClassFullName(const String& classFullName, size_t fallbackIndex) {
    static const ViewStyle kButton = {ViewColors::Button.r, ViewColors::Button.g, ViewColors::Button.b,
                                       IconGlyph::Power};
    static const ViewStyle kColorScheme = {ViewColors::ColorScheme.r, ViewColors::ColorScheme.g,
                                            ViewColors::ColorScheme.b, IconGlyph::Colors};
    static const ViewStyle kValue = {ViewColors::Value.r, ViewColors::Value.g, ViewColors::Value.b,
                                      IconGlyph::Monitoring};
    static const ViewStyle kPercentage = {ViewColors::Percentage.r, ViewColors::Percentage.g,
                                           ViewColors::Percentage.b, IconGlyph::Percent};
    static const ViewStyle kToggle = {ViewColors::Toggle.r, ViewColors::Toggle.g, ViewColors::Toggle.b,
                                       IconGlyph::ToggleOn};
    static const ViewStyle kSlider = {ViewColors::Slider.r, ViewColors::Slider.g, ViewColors::Slider.b,
                                       IconGlyph::Sliders};
    static const ViewStyle kScene = {ViewColors::Scene.r, ViewColors::Scene.g, ViewColors::Scene.b, IconGlyph::Scene};
    static const ViewStyle kClock = {ViewColors::Clock.r, ViewColors::Clock.g, ViewColors::Clock.b,
                                      IconGlyph::Schedule};

    // Vivid fallback palette (no black/gray) for any classFullName not
    // explicitly styled above, cycled by the entry's position.
    static const ViewStyle kFallback[] = {
        {40, 110, 240, IconGlyph::Generic},   {40, 180, 90, IconGlyph::Generic},
        {235, 140, 20, IconGlyph::Generic},   {190, 60, 220, IconGlyph::Generic},
        {20, 175, 175, IconGlyph::Generic},   {220, 60, 95, IconGlyph::Generic},
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
    return "Tap to open";
}

// Draws a small recognizable glyph for `icon` inside a circle of radius `r`
// centered at (cx, cy), in `ink` color. Each glyph approximates the Google
// Material Symbols icon (https://fonts.google.com/icons) named in its case.
void drawViewIcon(M5Canvas& canvas, IconGlyph icon, int cx, int cy, int r, uint16_t ink) {
    switch (icon) {
        case IconGlyph::Power:
            // Material Symbols "power_settings_new": a broken circle with a
            // vertical line through the gap at the top.
            canvas.drawArc(cx, cy, r * 0.55f, r * 0.68f, 300, 600, ink);
            canvas.drawWideLine(cx, cy - r * 0.9f, cx, cy - r * 0.15f, r * 0.09f, ink);
            break;
        case IconGlyph::Colors:
            // Material Symbols "colors": a paint drop (teardrop) glyph.
            canvas.fillCircle(cx, cy + r * 0.15f, r * 0.42f, ink);
            canvas.fillTriangle(cx - r * 0.3f, cy - r * 0.05f, cx + r * 0.3f, cy - r * 0.05f, cx, cy - r * 0.6f, ink);
            break;
        case IconGlyph::Monitoring:
            // Material Symbols "monitoring": an upward trending line chart.
            canvas.drawWideLine(cx - r * 0.55f, cy + r * 0.3f, cx - r * 0.15f, cy - r * 0.05f, r * 0.09f, ink);
            canvas.drawWideLine(cx - r * 0.15f, cy - r * 0.05f, cx + r * 0.15f, cy + r * 0.2f, r * 0.09f, ink);
            canvas.drawWideLine(cx + r * 0.15f, cy + r * 0.2f, cx + r * 0.55f, cy - r * 0.45f, r * 0.09f, ink);
            canvas.fillCircle(cx + r * 0.55f, cy - r * 0.45f, r * 0.1f, ink);
            break;
        case IconGlyph::Percent:
            // Material Symbols "percent": two circles joined by a diagonal slash.
            canvas.fillCircle(cx - r * 0.32f, cy - r * 0.32f, r * 0.16f, ink);
            canvas.fillCircle(cx + r * 0.32f, cy + r * 0.32f, r * 0.16f, ink);
            canvas.drawWideLine(cx - r * 0.5f, cy + r * 0.5f, cx + r * 0.5f, cy - r * 0.5f, r * 0.09f, ink);
            break;
        case IconGlyph::ToggleOn:
            // Material Symbols "toggle_on": a simple pill-shaped switch track
            // (outline only, so it stays a single flat tone) with a solid knob
            // at the "on" (right) end.
            canvas.drawRoundRect(cx - r * 0.55f, cy - r * 0.28f, r * 1.1f, r * 0.56f, r * 0.28f, ink);
            canvas.fillCircle(cx + r * 0.27f, cy, r * 0.2f, ink);
            break;
        case IconGlyph::Sliders:
            // Material Symbols "sliders": three vertical tracks with knobs at
            // different levels.
            for (int i = -1; i <= 1; ++i) {
                float x = cx + i * r * 0.42f;
                canvas.drawLine(x, cy - r * 0.65f, x, cy + r * 0.65f, ink);
                canvas.fillCircle(x, cy - r * 0.3f * i, r * 0.15f, ink);
            }
            break;
        case IconGlyph::Scene:
            // Material Symbols "scene": a framed photo with a sun and mountain peaks.
            canvas.drawRoundRect(cx - r * 0.65f, cy - r * 0.5f, r * 1.3f, r * 1.0f, r * 0.12f, ink);
            canvas.fillCircle(cx - r * 0.3f, cy - r * 0.2f, r * 0.12f, ink);
            canvas.fillTriangle(cx - r * 0.5f, cy + r * 0.35f, cx - r * 0.05f, cy - r * 0.05f, cx + r * 0.35f, cy + r * 0.35f,
                                ink);
            canvas.fillTriangle(cx + r * 0.05f, cy + r * 0.35f, cx + r * 0.4f, cy + r * 0.05f, cx + r * 0.55f, cy + r * 0.35f,
                                ink);
            break;
        case IconGlyph::Schedule:
            // Material Symbols "schedule": clock face with hour/minute hands.
            canvas.drawCircle(cx, cy, r * 0.75f, ink);
            canvas.drawLine(cx, cy, cx, cy - r * 0.45f, ink);
            canvas.drawLine(cx, cy, cx + r * 0.32f, cy + r * 0.1f, ink);
            break;
        case IconGlyph::Generic:
        default:
            canvas.fillCircle(cx, cy, r * 0.3f, ink);
            break;
    }
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
    for (auto& entry : _entries) {
        for (const auto& cmdTemplate : entry.config.commandTemplates) {
            if (cmdTemplate.id == commandId) {
                entry.view->onCommand(command);
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

    canvas.fillCircle(kIconCenterX, kIconCenterY, kIconRadius, styleColor);
    drawViewIcon(canvas, style.icon, kIconCenterX, kIconCenterY, kIconRadius, BLACK);

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
        uint16_t nextColor = canvas.color565(nextStyle.r / 3, nextStyle.g / 3, nextStyle.b / 3);
        canvas.fillCircle(kNextPeekCenterX, kNextPeekCenterY, kNextPeekRadius, nextColor);
        drawViewIcon(canvas, nextStyle.icon, kNextPeekCenterX, kNextPeekCenterY, kNextPeekRadius, BLACK);
    }

    // Mirrors the next-entry peek above the current content, so turning the
    // bezel counter-clockwise (toward the previous entry) has the same
    // preview hint as turning it clockwise.
    if (count > 1) {
        size_t prevIndex = (_activeIndex + count - 1) % count;
        const ViewStyle& prevStyle = styleForClassFullName(_entries[prevIndex].config.classFullName, prevIndex);
        uint16_t prevColor = canvas.color565(prevStyle.r / 3, prevStyle.g / 3, prevStyle.b / 3);
        canvas.fillCircle(kPrevPeekCenterX, kPrevPeekCenterY, kPrevPeekRadius, prevColor);
        drawViewIcon(canvas, prevStyle.icon, kPrevPeekCenterX, kPrevPeekCenterY, kPrevPeekRadius, BLACK);
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

    if (_mode == Mode::Carousel) {
        renderCarousel(canvas);
        return;
    }

    if (IView* view = activeView()) {
        view->render(canvas);
    }
}

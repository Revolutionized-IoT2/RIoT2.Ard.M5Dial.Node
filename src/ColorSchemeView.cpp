#include "ColorSchemeView.h"

#include <math.h>

#include <memory>
#include <vector>

#include "Buzzer.h"
#include "ViewFactory.h"

namespace {

using riot2::views::ColorSchemeModel;

constexpr int kDisplaySize = 240;
constexpr int kWheelSegments = 36;  // 10-degree wedges around the ring - still a smooth
                                     // gradient, but half the fillArc() calls per frame
constexpr int kRingWidth = 34;      // wider than a plain hue ring - easier to tap a shade

struct Geometry {
    int cx, cy, outerR, innerR, swatchRadius;
};

Geometry computeGeometry(int width, int height) {
    Geometry g;
    g.cx = width / 2;
    g.cy = height / 2;
    int minDim = g.cx < g.cy ? g.cx : g.cy;
    g.outerR = minDim - 6;
    g.innerR = g.outerR - kRingWidth;
    g.swatchRadius = g.innerR - 16;
    return g;
}

// Angle (degrees, 0 = top / 12 o'clock, increasing clockwise) of a point
// relative to the wheel's center - this is our own bookkeeping convention,
// used consistently for hit-testing and the shade marker. It intentionally
// does NOT match `fillArc()`'s native angle - see `toLibraryAngle()` below.
float angleFromCenterDeg(int x, int y, int cx, int cy) {
    float dx = static_cast<float>(x - cx);
    float dy = static_cast<float>(y - cy);
    float angle = atan2f(dx, -dy) * (180.0f / PI);
    if (angle < 0.0f) {
        angle += 360.0f;
    }
    return angle;
}

// LovyanGFX's `fillArc()` measures its angle from East (3 o'clock), not
// North - converts our own 0=North/clockwise angle (above) to the angle
// `fillArc()` actually expects, so the rendered rim lines up with the
// marker/hit-testing math instead of sitting a quarter-turn off.
float toLibraryAngle(float degNorthClockwise) { return degNorthClockwise - 90.0f; }

// White text on a dark background color, black text on a light one, using
// the standard perceived-luminance weighting (so labels stay readable
// against any swatch color they're drawn on top of).
uint16_t contrastTextColor(uint8_t r, uint8_t g, uint8_t b) {
    float luminance = 0.299f * r + 0.587f * g + 0.114f * b;
    return luminance >= 140.0f ? BLACK : WHITE;
}

// Greedily splits `text` into lines that each fit within `maxWidth` (at the
// canvas's current text size), breaking on spaces, so a long label wraps
// onto two or more centered rows instead of overflowing the swatch.
std::vector<String> wrapText(M5Canvas& canvas, const String& text, int maxWidth) {
    std::vector<String> lines;
    int start = 0;
    int len = static_cast<int>(text.length());
    while (start < len) {
        int lastGoodEnd = -1;
        for (int i = start + 1; i <= len; ++i) {
            if (i < len && text[i] != ' ') {
                continue;
            }
            if (canvas.textWidth(text.substring(start, i)) <= maxWidth) {
                lastGoodEnd = i;
            } else {
                break;
            }
        }
        if (lastGoodEnd <= start) {
            lastGoodEnd = len;  // a single word too long to wrap - keep it whole
        }
        String line = text.substring(start, lastGoodEnd);
        line.trim();
        lines.push_back(line);
        start = lastGoodEnd;
        while (start < len && text[start] == ' ') {
            ++start;
        }
    }
    return lines;
}

}  // namespace

void ColorSchemeView::begin(const DeviceConfiguration& config) {
    _model.begin(config);
}

void ColorSchemeView::onTouch(int x, int y) {
    Geometry geo = computeGeometry(kDisplaySize, kDisplaySize);
    float dx = static_cast<float>(x - geo.cx);
    float dy = static_cast<float>(y - geo.cy);
    float dist = sqrtf(dx * dx + dy * dy);

    if (!_model.isPicking()) {
        if (dist <= geo.swatchRadius) {
            // Tapped the center: start picking, previewing the nearest of
            // the main colors to the last confirmed hue (never without a
            // report template).
            _model.startPicking();
        }
        return;
    }

    float shade;
    if (dist <= geo.swatchRadius) {
        // Center tap: confirm the pure main hue.
        shade = ColorSchemeModel::kPureShade;
    } else if (dist >= geo.innerR && dist <= geo.outerR) {
        // Rim tap: confirm the shade under the touch point.
        shade = angleFromCenterDeg(x, y, geo.cx, geo.cy) / 360.0f;
    } else {
        return;  // tapped the gap between swatch and rim - keep picking
    }

    Report report;
    if (_model.confirm(shade, report)) {
        Buzzer::confirm();
        publishReport(report);
    }
}

void ColorSchemeView::onEncoderChange(int delta) {
    _model.step(delta);
}

void ColorSchemeView::onCommand(const Command& command) {
    _model.onCommand(command);
}

void ColorSchemeView::render(M5Canvas& canvas) {
    canvas.fillScreen(BLACK);

    Geometry geo = computeGeometry(canvas.width(), canvas.height());
    float hue = _model.shownHue();
    float shadeT = _model.shownShade();  // picking always previews the pure main hue

    // Outer rim: a black -> hue -> white shade sweep, so a hue's shades
    // are immediately tappable all the way around as soon as it changes.
    float step = 360.0f / kWheelSegments;
    for (int i = 0; i < kWheelSegments; ++i) {
        float angle0 = i * step;
        float angle1 = angle0 + step + 1.0f;  // slight overlap avoids seams
        float t = (angle0 + step / 2.0f) / 360.0f;
        ColorSchemeModel::Rgb rgb = ColorSchemeModel::shadeToRgb(hue, t);
        canvas.fillArc(geo.cx, geo.cy, geo.outerR, geo.innerR, toLibraryAngle(angle0), toLibraryAngle(angle1),
                       canvas.color565(rgb.red, rgb.green, rgb.blue));
    }

    // Marker: shows the confirmed shade's position on the rim once idle.
    if (!_model.isPicking()) {
        float angleRad = (_model.shade() * 360.0f) * (PI / 180.0f);
        int markerR = (geo.outerR + geo.innerR) / 2;
        int mx = geo.cx + static_cast<int>(markerR * sinf(angleRad));
        int my = geo.cy - static_cast<int>(markerR * cosf(angleRad));
        canvas.fillCircle(mx, my, 7, WHITE);
        canvas.drawCircle(mx, my, 7, BLACK);
    }

    // Center swatch: pure hue while choosing the main color, or the
    // confirmed shade once one has been picked.
    ColorSchemeModel::Rgb swatch = ColorSchemeModel::shadeToRgb(hue, shadeT);
    uint16_t swatchColor = canvas.color565(swatch.red, swatch.green, swatch.blue);
    canvas.fillCircle(geo.cx, geo.cy, geo.swatchRadius, swatchColor);

    canvas.setTextDatum(middle_center);
    canvas.setTextSize(2);
    canvas.setTextColor(contrastTextColor(swatch.red, swatch.green, swatch.blue), swatchColor);
    String label = _model.isPicking() ? String("rotate hue, tap to pick shade") : ColorSchemeModel::toHex(swatch);
    int maxTextWidth = geo.swatchRadius * 2 - 16;
    std::vector<String> lines = wrapText(canvas, label, maxTextWidth);
    const int lineHeight = 20;
    int startY = geo.cy - (static_cast<int>(lines.size()) - 1) * lineHeight / 2;
    for (size_t i = 0; i < lines.size(); ++i) {
        canvas.drawString(lines[i], geo.cx, startY + static_cast<int>(i) * lineHeight);
    }
}

namespace {
struct ColorSchemeViewRegistrar {
    ColorSchemeViewRegistrar() {
        ViewFactory::instance().registerCreator(
            "RIoT2.Ard.M5Dial.Node.ColorSchemeView", []() { return std::make_unique<ColorSchemeView>(); },
            []() { return ColorSchemeModel::buildTemplate("RIoT2.Ard.M5Dial.Node.ColorSchemeView"); });
    }
} colorSchemeViewRegistrar;
}  // namespace
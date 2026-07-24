#include "ColorSchemeView.h"

#include <math.h>

#include <memory>

#include "Buzzer.h"
#include "ViewColors.h"
#include "ViewFactory.h"

namespace {

constexpr int kDisplaySize = 240;
constexpr int kMainColorCount = 16;  // bezel rotation cycles through this many main colors
constexpr float kMainColorStep = 360.0f / kMainColorCount;
constexpr int kWheelSegments = 72;  // 5-degree wedges around the ring
constexpr int kRingWidth = 34;      // wider than a plain hue ring - easier to tap a shade

const uint16_t kAccentColor = ViewColors::toRGB565(ViewColors::ColorScheme);            // this view's assigned color
const uint16_t kPickingColor = ViewColors::toRGB565(ViewColors::ColorSchemeSecondary);  // pale highlight while picking

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
// relative to the wheel's center - matches the convention `fillArc()`
// already uses elsewhere in this codebase (see PercentageView/TimerView).
float angleFromCenterDeg(int x, int y, int cx, int cy) {
    float dx = static_cast<float>(x - cx);
    float dy = static_cast<float>(y - cy);
    float angle = atan2f(dx, -dy) * (180.0f / PI);
    if (angle < 0.0f) {
        angle += 360.0f;
    }
    return angle;
}

float normalizeHue(float hue) {
    hue = fmodf(hue, 360.0f);
    if (hue < 0.0f) {
        hue += 360.0f;
    }
    return hue;
}

// Snaps a hue to the nearest of the 16 fixed main colors, returning its index.
int nearestColorIndex(float hue) {
    int index = static_cast<int>(roundf(normalizeHue(hue) / kMainColorStep)) % kMainColorCount;
    if (index < 0) {
        index += kMainColorCount;
    }
    return index;
}

// Standard HSV (S=1, V=1) to 8-bit RGB conversion - the "pure" main color.
void hueToRgb8(float hue, uint8_t& r, uint8_t& g, uint8_t& b) {
    hue = normalizeHue(hue);
    float h = hue / 60.0f;
    float x = 255.0f * (1.0f - fabsf(fmodf(h, 2.0f) - 1.0f));

    float rf, gf, bf;
    if (h < 1.0f) {
        rf = 255.0f; gf = x; bf = 0.0f;
    } else if (h < 2.0f) {
        rf = x; gf = 255.0f; bf = 0.0f;
    } else if (h < 3.0f) {
        rf = 0.0f; gf = 255.0f; bf = x;
    } else if (h < 4.0f) {
        rf = 0.0f; gf = x; bf = 255.0f;
    } else if (h < 5.0f) {
        rf = x; gf = 0.0f; bf = 255.0f;
    } else {
        rf = 255.0f; gf = 0.0f; bf = x;
    }

    r = static_cast<uint8_t>(rf);
    g = static_cast<uint8_t>(gf);
    b = static_cast<uint8_t>(bf);
}

// Shade of `hue` at position `t`: 0 = black, 0.5 = the pure main hue,
// 1 = white - darkening scales channels down (hue-preserving), lightening
// blends toward white (also hue-preserving, since it's an affine mix).
void shadeToRgb8(float hue, float t, uint8_t& r, uint8_t& g, uint8_t& b) {
    uint8_t hr, hg, hb;
    hueToRgb8(hue, hr, hg, hb);

    if (t <= 0.5f) {
        float f = t / 0.5f;  // 0 (black) -> 1 (pure hue)
        r = static_cast<uint8_t>(hr * f);
        g = static_cast<uint8_t>(hg * f);
        b = static_cast<uint8_t>(hb * f);
    } else {
        float f = (t - 0.5f) / 0.5f;  // 0 (pure hue) -> 1 (white)
        r = static_cast<uint8_t>(hr + (255 - hr) * f);
        g = static_cast<uint8_t>(hg + (255 - hg) * f);
        b = static_cast<uint8_t>(hb + (255 - hb) * f);
    }
}

String shadeToHex(float hue, float t) {
    uint8_t r, g, b;
    shadeToRgb8(hue, t, r, g, b);
    char buf[8];
    snprintf(buf, sizeof(buf), "#%02X%02X%02X", r, g, b);
    return String(buf);
}

// Recovers the (hue, shadeT) that produced an inbound "#RRGGBB" command,
// assuming it came from `shadeToHex()` above (darkening/lightening both
// preserve hue exactly, so this round-trips cleanly).
void hexToHueAndShade(const String& hex, float& hue, float& shadeT) {
    String s = hex;
    if (s.startsWith("#")) {
        s.remove(0, 1);
    }
    if (s.length() < 6) {
        hue = 0.0f;
        shadeT = 0.5f;
        return;
    }

    long r = strtol(s.substring(0, 2).c_str(), nullptr, 16);
    long g = strtol(s.substring(2, 4).c_str(), nullptr, 16);
    long b = strtol(s.substring(4, 6).c_str(), nullptr, 16);

    long maxV = max(r, max(g, b));
    long minV = min(r, min(g, b));
    float delta = static_cast<float>(maxV - minV);

    if (delta < 0.001f) {
        hue = 0.0f;
    } else if (maxV == r) {
        hue = normalizeHue(60.0f * fmodf((g - b) / delta, 6.0f));
    } else if (maxV == g) {
        hue = normalizeHue(60.0f * ((b - r) / delta + 2.0f));
    } else {
        hue = normalizeHue(60.0f * ((r - g) / delta + 4.0f));
    }

    float value = maxV / 255.0f;
    if (value < 0.999f) {
        shadeT = value / 2.0f;  // darkened shade
    } else {
        float saturation = (maxV == 0) ? 0.0f : delta / static_cast<float>(maxV);
        shadeT = 1.0f - saturation / 2.0f;  // pure hue or a white-ward tint
    }
}

}  // namespace

void ColorSchemeView::begin(const DeviceConfiguration& config) {
    _reportId = config.reportTemplates.empty() ? "" : config.reportTemplates[0].id;
    _commandId = config.commandTemplates.empty() ? "" : config.commandTemplates[0].id;
    _hue = 0.0f;
    _shadeT = 0.5f;
    _pendingHue = 0.0f;
    _pendingColorIndex = 0;
    _picking = false;
}

void ColorSchemeView::onTouch(int x, int y) {
    if (_reportId.length() == 0) {
        return;
    }

    Geometry geo = computeGeometry(kDisplaySize, kDisplaySize);
    float dx = static_cast<float>(x - geo.cx);
    float dy = static_cast<float>(y - geo.cy);
    float dist = sqrtf(dx * dx + dy * dy);

    if (!_picking) {
        if (dist <= geo.swatchRadius) {
            // Tapped the center: start picking, previewing the nearest of
            // the 16 main colors to the last confirmed hue.
            _picking = true;
            _pendingColorIndex = nearestColorIndex(_hue);
            _pendingHue = _pendingColorIndex * kMainColorStep;
        }
        return;
    }

    if (dist <= geo.swatchRadius) {
        // Center tap: confirm the pure main hue.
        _hue = _pendingHue;
        _shadeT = 0.5f;
    } else if (dist >= geo.innerR && dist <= geo.outerR) {
        // Rim tap: confirm the shade under the touch point.
        _hue = _pendingHue;
        _shadeT = angleFromCenterDeg(x, y, geo.cx, geo.cy) / 360.0f;
    } else {
        return;  // tapped the gap between swatch and rim - keep picking
    }

    _picking = false;
    Buzzer::confirm();
    publishReport(Report{_reportId, String("\"") + shadeToHex(_hue, _shadeT) + "\""});
}

void ColorSchemeView::onEncoderChange(int delta) {
    _pendingColorIndex = ((_pendingColorIndex + (delta > 0 ? 1 : -1)) % kMainColorCount + kMainColorCount) % kMainColorCount;
    _pendingHue = _pendingColorIndex * kMainColorStep;
}

void ColorSchemeView::onCommand(const Command& command) {
    if (_picking || _commandId.length() == 0 || command.id != _commandId) {
        return;
    }

    hexToHueAndShade(command.value.as<String>(), _hue, _shadeT);
}

void ColorSchemeView::render(M5Canvas& canvas) {
    canvas.fillScreen(BLACK);

    Geometry geo = computeGeometry(canvas.width(), canvas.height());
    float hue = _picking ? _pendingHue : _hue;
    float shadeT = _picking ? 0.5f : _shadeT;  // picking always previews the pure main hue

    // Outer rim: a black -> hue -> white shade sweep, so a hue's shades
    // are immediately tappable all the way around as soon as it changes.
    float step = 360.0f / kWheelSegments;
    for (int i = 0; i < kWheelSegments; ++i) {
        float angle0 = i * step;
        float angle1 = angle0 + step + 1.0f;  // slight overlap avoids seams
        float t = (angle0 + step / 2.0f) / 360.0f;
        uint8_t r, g, b;
        shadeToRgb8(hue, t, r, g, b);
        canvas.fillArc(geo.cx, geo.cy, geo.outerR, geo.innerR, angle0, angle1, canvas.color565(r, g, b));
    }

    // Marker: shows the confirmed shade's position on the rim once idle.
    if (!_picking) {
        float angleRad = (_shadeT * 360.0f) * (PI / 180.0f);
        int markerR = (geo.outerR + geo.innerR) / 2;
        int mx = geo.cx + static_cast<int>(markerR * sinf(angleRad));
        int my = geo.cy - static_cast<int>(markerR * cosf(angleRad));
        canvas.fillCircle(mx, my, 7, WHITE);
        canvas.drawCircle(mx, my, 7, BLACK);
    }

    // Center swatch: pure hue while choosing the main color, or the
    // confirmed shade once one has been picked.
    uint8_t sr, sg, sb;
    shadeToRgb8(hue, shadeT, sr, sg, sb);
    canvas.fillCircle(geo.cx, geo.cy, geo.swatchRadius, canvas.color565(sr, sg, sb));

    canvas.setTextDatum(middle_center);
    canvas.setTextSize(1);
    canvas.setTextColor(_picking ? kPickingColor : kAccentColor, BLACK);
    canvas.drawString(_picking ? "rotate hue, tap to pick shade" : shadeToHex(hue, shadeT), geo.cx, geo.cy);
}

namespace {
struct ColorSchemeViewRegistrar {
    ColorSchemeViewRegistrar() {
        ViewFactory::instance().registerView("RIoT2.Ard.M5Dial.Node.ColorSchemeView",
                                              []() { return std::make_unique<ColorSchemeView>(); });
    }
} colorSchemeViewRegistrar;
}  // namespace

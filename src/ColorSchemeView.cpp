#include "ColorSchemeView.h"

#include <memory>

#include "Buzzer.h"
#include "ViewColors.h"
#include "ViewFactory.h"

namespace {

struct Swatch {
    const char* name;
    uint32_t color;
};

const Swatch kSwatches[] = {
    {"Red", 0xF800},   {"Orange", 0xFC00}, {"Yellow", 0xFFE0}, {"Green", 0x07E0},
    {"Cyan", 0x07FF},  {"Blue", 0x001F},   {"Purple", 0x801F}, {"Magenta", 0xF81F},
    {"White", 0xFFFF}, {"Warm White", 0xFEDA},
};
constexpr int kSwatchCount = sizeof(kSwatches) / sizeof(kSwatches[0]);

const uint16_t kAccentColor = ViewColors::toRGB565(ViewColors::ColorScheme);            // this view's assigned color
const uint16_t kPickingColor = ViewColors::toRGB565(ViewColors::ColorSchemeSecondary);  // pale highlight while picking

}  // namespace

void ColorSchemeView::begin(const DeviceConfiguration& config) {
    _reportId = config.reportTemplates.empty() ? "" : config.reportTemplates[0].id;
    _commandId = config.commandTemplates.empty() ? "" : config.commandTemplates[0].id;
    _selectedIndex = 0;
    _pendingIndex = 0;
    _picking = false;
}

void ColorSchemeView::onTouch(int x, int y) {
    (void)x;
    (void)y;
    if (_reportId.length() == 0) {
        return;
    }

    if (!_picking) {
        _picking = true;
        _pendingIndex = _selectedIndex;
    } else {
        _picking = false;
        _selectedIndex = _pendingIndex;
        String value = String("\"") + kSwatches[_selectedIndex].name + "\"";
        Buzzer::confirm();
        publishReport(Report{_reportId, value});
    }
}

void ColorSchemeView::onEncoderChange(int delta) {
    _pendingIndex = (_pendingIndex + (delta > 0 ? 1 : -1) + kSwatchCount) % kSwatchCount;
}

void ColorSchemeView::onCommand(const Command& command) {
    if (_picking || _commandId.length() == 0 || command.id != _commandId) {
        return;
    }

    String name = command.value.as<String>();
    for (int i = 0; i < kSwatchCount; ++i) {
        if (name.equalsIgnoreCase(kSwatches[i].name)) {
            _selectedIndex = i;
            return;
        }
    }
}

void ColorSchemeView::render(M5Canvas& canvas) {
    canvas.fillScreen(BLACK);

    int index = _picking ? _pendingIndex : _selectedIndex;
    int cx = canvas.width() / 2;
    int cy = canvas.height() / 2;
    int radius = (cx < cy ? cx : cy) - 30;

    canvas.fillCircle(cx, cy, radius, kSwatches[index].color);
    canvas.drawCircle(cx, cy, radius, _picking ? kPickingColor : kAccentColor);

    canvas.setTextColor(WHITE, BLACK);
    canvas.setTextDatum(middle_center);
    canvas.setTextSize(2);
    canvas.drawString(kSwatches[index].name, cx, canvas.height() - 20);

    if (_picking) {
        canvas.setTextColor(kPickingColor, BLACK);
        canvas.setTextSize(1);
        canvas.drawString("rotate + tap to confirm", cx, 16);
    }
}

namespace {
struct ColorSchemeViewRegistrar {
    ColorSchemeViewRegistrar() {
        ViewFactory::instance().registerView("RIoT2.Ard.M5Dial.Node.ColorSchemeView",
                                              []() { return std::make_unique<ColorSchemeView>(); });
    }
} colorSchemeViewRegistrar;
}  // namespace

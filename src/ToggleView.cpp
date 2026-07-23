#include "ToggleView.h"

#include <memory>

#include "Buzzer.h"
#include "ViewColors.h"
#include "ViewFactory.h"

namespace {
const uint16_t kOnColor = ViewColors::toRGB565(ViewColors::Toggle);  // this view's assigned color
constexpr uint32_t kOffColor = 0x39C7;                               // dark grey
}  // namespace

void ToggleView::begin(const DeviceConfiguration& config) {
    _name = config.name;
    _reportId = config.reportTemplates.empty() ? "" : config.reportTemplates[0].id;
    _hasCommand = false;
    _commandId = "";

    if (!config.reportTemplates.empty()) {
        const String& address = config.reportTemplates[0].address;
        for (const auto& cmd : config.commandTemplates) {
            if (address.length() > 0 && cmd.address == address) {
                _commandId = cmd.id;
                _hasCommand = true;
                break;
            }
        }
    }

    _active = false;
}

void ToggleView::toggle() {
    if (_reportId.length() == 0) {
        return;
    }
    _active = !_active;
    Buzzer::tap();
    publishReport(Report{_reportId, _active ? "true" : "false"});
}

void ToggleView::onTouch(int x, int y) {
    (void)x;
    (void)y;
    toggle();
}

void ToggleView::onCommand(const Command& command) {
    if (!_hasCommand || command.id != _commandId) {
        return;
    }
    _active = command.value.is<bool>() ? command.value.as<bool>() : command.value.as<int>() != 0;
}

void ToggleView::render(M5Canvas& canvas) {
    canvas.fillScreen(BLACK);

    int cx = canvas.width() / 2;
    int cy = canvas.height() / 2;
    int radius = (cx < cy ? cx : cy) - 30;

    canvas.fillCircle(cx, cy, radius, _active ? kOnColor : kOffColor);
    canvas.drawCircle(cx, cy, radius, WHITE);

    canvas.setTextColor(WHITE, _active ? kOnColor : kOffColor);
    canvas.setTextDatum(middle_center);
    canvas.setTextSize(3);
    canvas.drawString(_active ? "ON" : "OFF", cx, cy);

    canvas.setTextColor(WHITE, BLACK);
    canvas.setTextSize(1);
    canvas.drawString(_name, cx, canvas.height() - 20);
}

namespace {
struct ToggleViewRegistrar {
    ToggleViewRegistrar() {
        ViewFactory::instance().registerView("RIoT2.Ard.M5Dial.Node.ToggleView",
                                              []() { return std::make_unique<ToggleView>(); });
    }
} toggleViewRegistrar;
}  // namespace

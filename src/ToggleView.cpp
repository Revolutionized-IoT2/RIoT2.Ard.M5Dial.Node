#include "ToggleView.h"

#include <memory>

#include "Buzzer.h"
#include "ViewColors.h"
#include "ViewFactory.h"

namespace {
const uint16_t kLabelColor = ViewColors::toRGB565(ViewColors::Toggle);            // this view's assigned color
const uint16_t kOnColor = ViewColors::toRGB565(ViewColors::Toggle);               // primary "on" track, per CLAUDE.md
const uint16_t kOffColor = ViewColors::toRGB565(ViewColors::ToggleSecondary);     // pale secondary "off" track, per CLAUDE.md
constexpr int kSwitchWidth = 100;
constexpr int kSwitchHeight = 44;

void drawSwitch(M5Canvas& canvas, int cx, int cy, const String& name, bool active) {
    int trackX = cx - kSwitchWidth / 2;
    int trackY = cy - kSwitchHeight / 2;
    int radius = kSwitchHeight / 2;

    canvas.setTextColor(kLabelColor, BLACK);
    canvas.setTextDatum(middle_center);
    canvas.setTextSize(2);
    canvas.drawString(name, cx, cy - kSwitchHeight / 2 - 24);

    canvas.fillRoundRect(trackX, trackY, kSwitchWidth, kSwitchHeight, radius, active ? kOnColor : kOffColor);

    int knobRadius = radius - 4;
    int knobX = active ? trackX + kSwitchWidth - radius : trackX + radius;
    canvas.fillCircle(knobX, cy, knobRadius, WHITE);
}
}  // namespace

void ToggleView::begin(const DeviceConfiguration& config) {
    _slots.clear();

    for (const auto& report : config.reportTemplates) {
        Slot slot;
        slot.report = report;

        for (const auto& cmd : config.commandTemplates) {
            if (report.address.length() > 0 && cmd.address == report.address) {
                slot.command = cmd;
                slot.hasCommand = true;
                break;
            }
        }

        _slots.push_back(slot);
        if (_slots.size() >= 2) {
            break;
        }
    }
}

int ToggleView::slotAt(int x, int y) const {
    (void)x;
    if (_slots.empty()) {
        return -1;
    }
    if (_slots.size() == 1) {
        return 0;
    }

    int rowHeight = 240 / static_cast<int>(_slots.size());
    int index = y / rowHeight;
    if (index < 0) {
        index = 0;
    }
    if (index >= static_cast<int>(_slots.size())) {
        index = static_cast<int>(_slots.size()) - 1;
    }
    return index;
}

void ToggleView::toggle(int index) {
    if (index < 0 || index >= static_cast<int>(_slots.size()) || _slots[index].report.id.length() == 0) {
        return;
    }
    Slot& slot = _slots[index];
    slot.active = !slot.active;
    Buzzer::tap();
    publishReport(Report{slot.report.id, slot.active ? "true" : "false"});
}

void ToggleView::onTouch(int x, int y) {
    toggle(slotAt(x, y));
}

void ToggleView::onCommand(const Command& command) {
    for (auto& slot : _slots) {
        if (slot.hasCommand && slot.command.id == command.id) {
            slot.active = command.value.is<bool>() ? command.value.as<bool>() : command.value.as<int>() != 0;
            return;
        }
    }
}

void ToggleView::render(M5Canvas& canvas) {
    canvas.fillScreen(BLACK);

    if (_slots.empty()) {
        canvas.setTextColor(WHITE);
        canvas.setTextDatum(middle_center);
        canvas.setTextSize(2);
        canvas.drawString("No switches", canvas.width() / 2, canvas.height() / 2);
        return;
    }

    int cx = canvas.width() / 2;

    if (_slots.size() == 1) {
        drawSwitch(canvas, cx, canvas.height() / 2, _slots[0].report.name, _slots[0].active);
    } else {
        int rowHeight = canvas.height() / static_cast<int>(_slots.size());
        for (size_t i = 0; i < _slots.size(); ++i) {
            int cy = static_cast<int>(i) * rowHeight + rowHeight / 2;
            drawSwitch(canvas, cx, cy, _slots[i].report.name, _slots[i].active);
        }
    }
}

namespace {
struct ToggleViewRegistrar {
    ToggleViewRegistrar() {
        ViewFactory::instance().registerView("RIoT2.Ard.M5Dial.Node.ToggleView",
                                              []() { return std::make_unique<ToggleView>(); });
    }
} toggleViewRegistrar;
}  // namespace

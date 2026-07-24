#include "ButtonView.h"

#include <memory>

#include "Buzzer.h"
#include "ViewColors.h"
#include "ViewFactory.h"

namespace {
constexpr int kDisplaySize = 240;
const uint16_t kActiveColor = ViewColors::toRGB565(ViewColors::Button);           // this view's assigned color
const uint16_t kBorderColor = ViewColors::toRGB565(ViewColors::ButtonSecondary);  // pale themed row border
constexpr uint32_t kInactiveColor = 0x39C7;                                       // dark grey
constexpr unsigned long kFlashMs = 200;
}  // namespace

void ButtonView::begin(const DeviceConfiguration& config) {
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
        if (_slots.size() >= 4) {
            break;
        }
    }
}

int ButtonView::slotAt(int x, int y) const {
    (void)x;
    if (_slots.empty()) {
        return -1;
    }

    int rowHeight = kDisplaySize / static_cast<int>(_slots.size());
    int index = y / rowHeight;
    if (index < 0) {
        index = 0;
    }
    if (index >= static_cast<int>(_slots.size())) {
        index = static_cast<int>(_slots.size()) - 1;
    }
    return index;
}

void ButtonView::onTouch(int x, int y) {
    int index = slotAt(x, y);
    if (index < 0) {
        return;
    }

    _slots[index].flashUntilMs = millis() + kFlashMs;
    Buzzer::tap();
    publishReport(Report{_slots[index].report.id, "true"});
}

void ButtonView::onCommand(const Command& command) {
    for (auto& slot : _slots) {
        if (slot.hasCommand && slot.command.id == command.id) {
            bool value = command.value.is<bool>() ? command.value.as<bool>() : command.value.as<int>() != 0;
            slot.active = value;
            return;
        }
    }
}

void ButtonView::render(M5Canvas& canvas) {
    canvas.fillScreen(BLACK);

    if (_slots.empty()) {
        canvas.setTextColor(WHITE);
        canvas.setTextDatum(middle_center);
        canvas.setTextSize(2);
        canvas.drawString("No buttons", canvas.width() / 2, canvas.height() / 2);
        return;
    }

    int rowHeight = canvas.height() / static_cast<int>(_slots.size());
    unsigned long now = millis();

    for (size_t i = 0; i < _slots.size(); ++i) {
        int y0 = static_cast<int>(i) * rowHeight;
        bool flashing = now < _slots[i].flashUntilMs;
        uint32_t color = (_slots[i].active || flashing) ? kActiveColor : kInactiveColor;

        canvas.fillRect(4, y0 + 4, canvas.width() - 8, rowHeight - 8, color);
        canvas.drawRect(4, y0 + 4, canvas.width() - 8, rowHeight - 8, kBorderColor);
        canvas.setTextColor(WHITE);
        canvas.setTextDatum(middle_center);
        canvas.setTextSize(2);
        canvas.drawString(_slots[i].report.name, canvas.width() / 2, y0 + rowHeight / 2);
    }
}

namespace {
struct ButtonViewRegistrar {
    ButtonViewRegistrar() {
        ViewFactory::instance().registerView("RIoT2.Ard.M5Dial.Node.ButtonView",
                                              []() { return std::make_unique<ButtonView>(); });
    }
} buttonViewRegistrar;
}  // namespace

#include "ValueView.h"

#include <memory>

#include "ViewFactory.h"

namespace {
constexpr size_t kMaxSlots = 2;
}  // namespace

void ValueView::begin(const DeviceConfiguration& config) {
    _slots.clear();

    for (size_t i = 0; i < config.commandTemplates.size() && i < kMaxSlots; ++i) {
        const auto& cmd = config.commandTemplates[i];
        Slot slot;
        slot.id = cmd.id;
        slot.name = cmd.name;
        slot.unit = findParameter(config.deviceParameters, "unit" + String(i + 1));
        _slots.push_back(slot);
    }
}

void ValueView::onCommand(const Command& command) {
    for (auto& slot : _slots) {
        if (slot.id == command.id) {
            slot.value = command.value.as<String>();
            return;
        }
    }
}

void ValueView::render(M5Canvas& canvas) {
    canvas.fillScreen(BLACK);
    canvas.setTextColor(WHITE);
    canvas.setTextDatum(middle_center);

    if (_slots.empty()) {
        canvas.setTextSize(2);
        canvas.drawString("No values", canvas.width() / 2, canvas.height() / 2);
        return;
    }

    int rowHeight = canvas.height() / static_cast<int>(_slots.size());
    for (size_t i = 0; i < _slots.size(); ++i) {
        int centerY = static_cast<int>(i) * rowHeight + rowHeight / 2;

        canvas.setTextSize(1);
        canvas.drawString(_slots[i].name, canvas.width() / 2, centerY - 14);

        canvas.setTextSize(3);
        canvas.drawString(_slots[i].value + _slots[i].unit, canvas.width() / 2, centerY + 10);
    }
}

namespace {
struct ValueViewRegistrar {
    ValueViewRegistrar() {
        ViewFactory::instance().registerView("RIoT2.Ard.M5Dial.Node.ValueView",
                                              []() { return std::make_unique<ValueView>(); });
    }
} valueViewRegistrar;
}  // namespace

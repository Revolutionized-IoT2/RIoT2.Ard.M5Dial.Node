#include "ButtonView.h"

#include <memory>
#include <vector>

#include "Buzzer.h"
#include "ViewColors.h"
#include "ViewFactory.h"

namespace {
constexpr int kDisplaySize = 240;
const uint16_t kHeaderColor = ViewColors::toRGB565(ViewColors::Button);            // this view's assigned color
const uint16_t kSubHeaderColor = ViewColors::toRGB565(ViewColors::ButtonSecondary);  // pale companion color
constexpr unsigned long kFlashMs = 200;
constexpr int kButtonWidth = 100;
constexpr int kButtonHeight = 56;
constexpr int kGap = 12;
constexpr int kCornerRadius = 12;

struct Rect {
    int x, y, w, h;
};

// Lays out 1-4 buttons: a single button or a centered pair for 1-2, a 2x2
// grid (rows filled top-to-bottom, left-to-right) for 3-4, all centered on
// screen regardless of count.
std::vector<Rect> layoutButtons(size_t count, int width, int height) {
    std::vector<Rect> rects;
    if (count == 0) {
        return rects;
    }

    size_t row0Count = count <= 2 ? count : 2;
    size_t row1Count = count > 2 ? count - 2 : 0;

    int cx = width / 2;
    int cy = height / 2;
    int rowSpacing = kButtonHeight + kGap;
    int row0Y = row1Count > 0 ? cy - rowSpacing / 2 : cy;
    int row1Y = cy + rowSpacing / 2;

    auto addRow = [&](size_t rowCount, int centerY) {
        if (rowCount == 0) {
            return;
        }
        int totalWidth = static_cast<int>(rowCount) * kButtonWidth + static_cast<int>(rowCount - 1) * kGap;
        int startX = cx - totalWidth / 2;
        for (size_t i = 0; i < rowCount; ++i) {
            int x = startX + static_cast<int>(i) * (kButtonWidth + kGap);
            rects.push_back(Rect{x, centerY - kButtonHeight / 2, kButtonWidth, kButtonHeight});
        }
    };

    addRow(row0Count, row0Y);
    addRow(row1Count, row1Y);

    return rects;
}

// Draws `text` centered at (cx, cy), starting at size 2 and stepping down to
// size 1 if needed, then - as a last resort - truncating with an ellipsis
// at size 1, so a long button label never spills past `maxWidth` (the
// button's own width, minus padding).
void drawFittedButtonText(M5Canvas& canvas, const String& text, int cx, int cy, int maxWidth, uint16_t color,
                          uint16_t bgColor) {
    canvas.setTextDatum(middle_center);

    for (int size = 2; size >= 1; --size) {
        canvas.setTextSize(size);
        if (canvas.textWidth(text) <= maxWidth) {
            canvas.setTextColor(color, bgColor);
            canvas.drawString(text, cx, cy);
            return;
        }
    }

    canvas.setTextSize(1);
    String truncated = text;
    while (truncated.length() > 1 && canvas.textWidth(truncated + "..") > maxWidth) {
        truncated.remove(truncated.length() - 1);
    }
    canvas.setTextColor(color, bgColor);
    canvas.drawString(truncated + "..", cx, cy);
}
}  // namespace

void ButtonView::begin(const DeviceConfiguration& config) {
    _slots.clear();
    _header = findParameter(config.deviceParameters, "header", config.name);
    _subHeader = findParameter(config.deviceParameters, "subHeader", "");

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
    std::vector<Rect> rects = layoutButtons(_slots.size(), kDisplaySize, kDisplaySize);
    for (size_t i = 0; i < rects.size(); ++i) {
        const Rect& r = rects[i];
        if (x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h) {
            return static_cast<int>(i);
        }
    }
    return -1;
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
    canvas.setTextDatum(middle_center);

    if (_header.length() > 0) {
        canvas.setTextColor(kHeaderColor, BLACK);
        canvas.setTextSize(3);
        canvas.drawString(_header, canvas.width() / 2, 30);
    }

    if (_slots.empty()) {
        canvas.setTextColor(WHITE, BLACK);
        canvas.setTextSize(2);
        canvas.drawString("No buttons", canvas.width() / 2, canvas.height() / 2);
        return;
    }

    std::vector<Rect> rects = layoutButtons(_slots.size(), canvas.width(), canvas.height());
    unsigned long now = millis();

    for (size_t i = 0; i < _slots.size(); ++i) {
        const Rect& r = rects[i];
        bool on = _slots[i].active || now < _slots[i].flashUntilMs;
        uint16_t bgColor = on ? kHeaderColor : kSubHeaderColor;
        uint16_t textColor = on ? WHITE : BLACK;

        canvas.fillRoundRect(r.x, r.y, r.w, r.h, kCornerRadius, bgColor);
        drawFittedButtonText(canvas, _slots[i].report.name, r.x + r.w / 2, r.y + r.h / 2, r.w - 12, textColor, bgColor);
    }

    if (_subHeader.length() > 0) {
        canvas.setTextColor(kSubHeaderColor, BLACK);
        canvas.setTextSize(1);
        canvas.drawString(_subHeader, canvas.width() / 2, canvas.height() - 20);
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

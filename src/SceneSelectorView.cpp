#include "SceneSelectorView.h"

#include <memory>

#include "Buzzer.h"
#include "ViewColors.h"
#include "ViewFactory.h"

namespace {
const uint16_t kBrowsingColor = ViewColors::toRGB565(ViewColors::Scene);       // this view's assigned color
const uint16_t kHintColor = ViewColors::toRGB565(ViewColors::SceneSecondary);  // pale accent for hint text
}  // namespace

void SceneSelectorView::begin(const DeviceConfiguration& config) {
    _items = config.reportTemplates;
    _selectedIndex = 0;
    _pendingIndex = 0;
    _browsing = false;
}

void SceneSelectorView::onTouch(int x, int y) {
    (void)x;
    (void)y;
    if (_items.empty()) {
        return;
    }

    if (!_browsing) {
        _browsing = true;
        _pendingIndex = _selectedIndex;
    } else {
        _browsing = false;
        _selectedIndex = _pendingIndex;
        Buzzer::confirm();
        publishReport(Report{_items[_selectedIndex].id, "true"});
    }
}

void SceneSelectorView::onEncoderChange(int delta) {
    if (_items.empty()) {
        return;
    }
    int count = static_cast<int>(_items.size());
    _pendingIndex = (_pendingIndex + (delta > 0 ? 1 : -1) + count) % count;
}

void SceneSelectorView::render(M5Canvas& canvas) {
    canvas.fillScreen(BLACK);

    int cx = canvas.width() / 2;
    int cy = canvas.height() / 2;

    if (_items.empty()) {
        canvas.setTextColor(WHITE);
        canvas.setTextDatum(middle_center);
        canvas.setTextSize(2);
        canvas.drawString("No scenes", cx, cy);
        return;
    }

    int index = _browsing ? _pendingIndex : _selectedIndex;
    int count = static_cast<int>(_items.size());
    int prevIndex = (index - 1 + count) % count;
    int nextIndex = (index + 1) % count;

    canvas.setTextDatum(middle_center);

    if (count > 1) {
        canvas.setTextColor(0x8410);  // dim grey
        canvas.setTextSize(1);
        canvas.drawString(_items[prevIndex].name, cx, cy - 60);
        canvas.drawString(_items[nextIndex].name, cx, cy + 60);
    }

    canvas.setTextColor(_browsing ? kBrowsingColor : WHITE);
    canvas.setTextSize(2);
    canvas.drawString(_items[index].name, cx, cy);

    canvas.setTextColor(kHintColor);
    canvas.setTextSize(1);
    canvas.drawString(_browsing ? "rotate + tap to confirm" : "tap to browse", cx, cy + 100);
}

namespace {
struct SceneSelectorViewRegistrar {
    SceneSelectorViewRegistrar() {
        ViewFactory::instance().registerCreator("RIoT2.Ard.M5Dial.Node.SceneSelectorView",
                                              []() { return std::make_unique<SceneSelectorView>(); });
    }
} sceneSelectorViewRegistrar;
}  // namespace

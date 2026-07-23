#include "ViewFactory.h"

ViewFactory& ViewFactory::instance() {
    static ViewFactory factory;
    return factory;
}

void ViewFactory::registerView(const String& classFullName, Creator creator) {
    _creators.push_back({classFullName, creator});
}

std::unique_ptr<IView> ViewFactory::create(const String& classFullName) const {
    for (const auto& entry : _creators) {
        if (entry.first == classFullName) {
            return entry.second();
        }
    }
    return nullptr;
}

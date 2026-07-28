#pragma once

#include <riot2/Factory.h>

#include "IView.h"

// classFullName-keyed registry for this project's IView implementations.
// See riot2::Factory for the generic implementation (shared with
// PeripheralFactory); concrete views register themselves here via a static
// initializer in their own .cpp file.
using ViewFactory = riot2::Factory<IView>;

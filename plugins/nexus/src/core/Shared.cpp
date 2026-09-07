#include "Shared.h"

AddonAPI* APIDefs = nullptr;
HMODULE   AddonModuleHandle = nullptr;
cba::Settings CurrentSettings{};
std::string   AddonDir;

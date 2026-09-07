#pragma once

// This pulls in the official Nexus addon API definitions (AddonAPI,
// AddonDefinition, ERenderType, ...). Get Nexus.h from:
// https://github.com/RaidcoreGG/RCGG-lib-nexus-api
// and drop it (plus its dependencies) into an include path.
#include "Nexus.h"
#include "Settings.h"
#include <string>

extern AddonAPI* APIDefs;
extern HMODULE   AddonModuleHandle;
extern cba::Settings CurrentSettings;
extern std::string   AddonDir;

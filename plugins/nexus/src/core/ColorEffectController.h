#pragma once
#include <windows.h>
#include <magnification.h>

namespace cba
{
class ColorEffectController
{
public:
    bool Initialize();
    bool Apply(MAGCOLOREFFECT effect);
    bool Clear();
    void Shutdown();
    bool IsInitialized() const { return _initialized; }

private:
    bool _initialized = false;
};

ColorEffectController& GetColorEffectController();
void InstallCrashGuard();
void RemoveCrashGuard();
}

#pragma once
#include <windows.h>
#include <magnification.h>

// Do not hook SetUnhandledExceptionFilter. CBA never installs a custom
// unhandled-exception filter anywhere in this codebase - ArenaNet's and
// ArcDPS's own crash reporters must stay the ones that catch a crash in
// the game process, not something CBA swaps in. (InstallCrashGuard/
// RemoveCrashGuard, no-op stubs that used to document this same fact,
// were removed as dead code 2026-09-09 - this comment replaces them as
// the actual documentation of the guarantee, without the unused stubs.)

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
}

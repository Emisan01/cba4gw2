#include "ColorEffectController.h"

#pragma comment(lib, "Magnification.lib")

namespace cba
{
	bool ColorEffectController::Initialize()
	{
		_initialized = MagInitialize();
		return _initialized;
	}

	bool ColorEffectController::Apply(MAGCOLOREFFECT aEffect)
	{
		if (!_initialized) return false;
		return MagSetFullscreenColorEffect(&aEffect);
	}

	bool ColorEffectController::Clear()
	{
		if (!_initialized) return false;
		MAGCOLOREFFECT identity{};
		identity.transform[0][0] = 1.0f;
		identity.transform[1][1] = 1.0f;
		identity.transform[2][2] = 1.0f;
		identity.transform[3][3] = 1.0f;
		identity.transform[4][4] = 1.0f;
		return MagSetFullscreenColorEffect(&identity);
	}

	void ColorEffectController::Shutdown()
	{
		if (_initialized)
		{
			Clear();
			MagUninitialize();
			_initialized = false;
		}
	}

	namespace
	{
		ColorEffectController s_controller;
	}

	ColorEffectController& GetColorEffectController()
	{
		return s_controller;
	}

	void InstallCrashGuard()
	{
		// No-op: Do not hook SetUnhandledExceptionFilter in game processes
	}

	void RemoveCrashGuard()
	{
		// No-op
	}
}

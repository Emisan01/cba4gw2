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
		LPTOP_LEVEL_EXCEPTION_FILTER s_previousFilter = nullptr;

		LONG WINAPI CrashGuardFilter(EXCEPTION_POINTERS* aExceptionInfo)
		{
			// Best-effort only. If we get here, the process is already
			// unwinding after a genuine exception (SEH), which is a much
			// friendlier scenario than a hard TerminateProcess or an access
			// violation deep in GW2's own render thread with corrupted state.
			// Still worth trying before we pass it on.
			s_controller.Clear();

			if (s_previousFilter)
				return s_previousFilter(aExceptionInfo);

			return EXCEPTION_CONTINUE_SEARCH;
		}
	}

	ColorEffectController& GetColorEffectController()
	{
		return s_controller;
	}

	void InstallCrashGuard()
	{
		s_previousFilter = SetUnhandledExceptionFilter(CrashGuardFilter);
	}

	void RemoveCrashGuard()
	{
		SetUnhandledExceptionFilter(s_previousFilter);
		s_previousFilter = nullptr;
	}
}

#include "ColorEffectController.h"
#include "Shared.h"

#pragma comment(lib, "Magnification.lib")

std::atomic<unsigned int> g_DwmClearRejectCount{0};

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

		BOOL success = MagSetFullscreenColorEffect(&aEffect);
		bool newSuccess = (success != FALSE);

		// Nur loggen wenn sich der Status ändert (Log-Spam verhindern)
		if (newSuccess != g_DwmLastCallSuccessful)
		{
			if (!newSuccess && APIDefs && APIDefs->Log)
			{
				APIDefs->Log(ELogLevel_WARNING, "cba4gw2", "MagSetFullscreenColorEffect REJECTED by Windows OS! (Check HDR or Exclusive Fullscreen)");
			}
			else if (newSuccess && APIDefs && APIDefs->Log)
			{
				APIDefs->Log(ELogLevel_INFO, "cba4gw2", "MagSetFullscreenColorEffect recovered - OS blocking resolved");
			}
		}

		// Globale Status-Variable aktualisieren (für die UI)
		g_DwmLastCallSuccessful = newSuccess;
		return g_DwmLastCallSuccessful;
	}

	bool ColorEffectController::Clear()
	{
		if (!_initialized) return true;
		MAGCOLOREFFECT identity{};
		identity.transform[0][0] = 1.0f;
		identity.transform[1][1] = 1.0f;
		identity.transform[2][2] = 1.0f;
		identity.transform[3][3] = 1.0f;
		identity.transform[4][4] = 1.0f;
		BOOL success = MagSetFullscreenColorEffect(&identity);
		bool newSuccess = (success != FALSE);
		if (!newSuccess) g_DwmClearRejectCount.fetch_add(1);

		// Nur loggen wenn sich der Status ändert (Log-Spam verhindern)
		if (newSuccess != g_DwmLastCallSuccessful)
		{
			if (!newSuccess && APIDefs && APIDefs->Log)
			{
				APIDefs->Log(ELogLevel_WARNING, "cba4gw2", "MagSetFullscreenColorEffect (Clear) REJECTED by Windows OS! (Check HDR or Exclusive Fullscreen)");
			}
			else if (newSuccess && APIDefs && APIDefs->Log)
			{
				APIDefs->Log(ELogLevel_INFO, "cba4gw2", "MagSetFullscreenColorEffect (Clear) recovered - OS blocking resolved");
			}
		}

		g_DwmLastCallSuccessful = newSuccess;
		return g_DwmLastCallSuccessful;
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

}

#pragma once
#include <magnification.h>

namespace cba
{
	// Same three axes as the exe. "Mixed" is not a fourth simulation matrix —
	// it's Deutan (red-green axis) and Tritan (blue-yellow axis) applied in
	// sequence, exactly like MixedCorrectionMatrix() below did in C#.
	enum class BalanceType
	{
		Protan,
		Deutan,
		Tritan
	};

	// Calculates daltonization matrices using the Fidaner, Lin, and Ozguven
	// approach (2005): simulate the color balance profile, calculate the color error, and
	// redistribute that error through the remaining channels.
	//
	// This is a practical approximation, not a clinically calibrated solution.
	// (Same disclaimer as the original — still true here, nothing changed
	// about the math, only the language it's written in.)
	namespace ColorMatrix
	{
		// severity01: 0.0 = no correction, 1.0 = full correction for anopia.
		void CorrectionMatrix(BalanceType aType, double aSeverity01, double aOut3x3[3][3]);

		// Mixed mode: red-green uses Deutan as its base type, blue-yellow uses Tritan.
		void MixedCorrectionMatrix(double aRgSeverity01, double aBySeverity01, double aOut3x3[3][3]);

		// Packs a 3x3 into the 5x5 MAGCOLOREFFECT the Magnification API expects.
		// Row/column 3 and 4 stay identity (alpha passthrough), same as the
		// C# ToMagColorEffect() did.
		MAGCOLOREFFECT ToMagColorEffect(const double aM3x3[3][3]);

		// Physiological simulation (LMS/Brettel): shows how the colour appears to
		// someone with the given balance profile at full anopia severity.
		void SimulatePixel(double aR, double aG, double aB,
		                   BalanceType aType,
		                   double& aOutR, double& aOutG, double& aOutB);

		// Applies a pre-computed 3×3 correction matrix to a single pixel.
		// Used for the "with filter" preview row without touching the system effect.
		void ApplyPixel(double aR, double aG, double aB,
		                const double aMatrix[3][3],
		                double& aOutR, double& aOutG, double& aOutB);

		// Eye-Sensitive Mode (2026-09-09) - its own independent layer, not a
		// CVD-correction concept. Composes with whatever CorrectionMatrix/
		// MixedCorrectionMatrix produced (see Recompute() in ModuleMain.cpp -
		// multiplied in downstream, like a tinted lens sitting in front of an
		// already-corrected image), never replaces it. All three params are
		// 0.0 (off) to 1.0 (max), independent and combinable per Emi's spec.
		//   aBlueFilter01: reduces blue channel gain (Night-Light style).
		//   aWarmTint01: shifts toward red/amber, reduces blue further.
		//   aSaturationReduction01: blends toward Rec.601 luminance-preserving
		//     grey - a real desaturation, not a brightness/contrast trick
		//     (those would need a translation term the 5x5 MAGCOLOREFFECT
		//     deliberately doesn't use here - see ToMagColorEffect's comment).
		void EyeComfortMatrix(double aBlueFilter01, double aWarmTint01,
		                      double aSaturationReduction01, double aOut3x3[3][3]);
	}
}

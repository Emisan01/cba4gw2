#pragma once
#include <magnification.h>

namespace cba
{
	// Same three axes as the exe. "Mixed" is not a fourth simulation matrix —
	// it's Deutan (red-green axis) and Tritan (blue-yellow axis) applied in
	// sequence, exactly like MixedCorrectionMatrix() below did in C#.
	enum class DeficiencyType
	{
		Protan,
		Deutan,
		Tritan
	};

	// Calculates daltonization matrices using the Fidaner, Lin, and Ozguven
	// approach (2005): simulate the deficiency, calculate the color error, and
	// redistribute that error through the remaining channels.
	//
	// This is a practical approximation, not a clinically calibrated solution.
	// (Same disclaimer as the original — still true here, nothing changed
	// about the math, only the language it's written in.)
	namespace ColorMatrix
	{
		// severity01: 0.0 = no correction, 1.0 = full correction for anopia.
		void CorrectionMatrix(DeficiencyType aType, double aSeverity01, double aOut3x3[3][3]);

		// Mixed mode: red-green uses Deutan as its base type, blue-yellow uses Tritan.
		void MixedCorrectionMatrix(double aRgSeverity01, double aBySeverity01, double aOut3x3[3][3]);

		// Packs a 3x3 into the 5x5 MAGCOLOREFFECT the Magnification API expects.
		// Row/column 3 and 4 stay identity (alpha passthrough), same as the
		// C# ToMagColorEffect() did.
		MAGCOLOREFFECT ToMagColorEffect(const double aM3x3[3][3]);

		// Physiological simulation (LMS/Brettel): shows how the colour appears to
		// someone with the given deficiency at full anopia severity.
		void SimulatePixel(double aR, double aG, double aB,
		                   DeficiencyType aType,
		                   double& aOutR, double& aOutG, double& aOutB);

		// Applies a pre-computed 3×3 correction matrix to a single pixel.
		// Used for the "with filter" preview row without touching the system effect.
		void ApplyPixel(double aR, double aG, double aB,
		                const double aMatrix[3][3],
		                double& aOutR, double& aOutG, double& aOutB);
	}
}

#include "ColorMatrix.h"
#include <algorithm>

namespace cba
{
	namespace
	{
		// ── Hunt-Pointer-Estévez (HPE) RGB to LMS Matrix (Viénot et al. 1999) ─
		constexpr double kRgbToLms[3][3] = {
			{ 17.8824,   43.5161,   4.11935  },
			{  3.45565,  27.1554,   3.86714  },
			{  0.0299566, 0.184309,  1.46709  }
		};

		// ── Inverse Matrix: LMS to RGB ───────────────────────────────────────
		constexpr double kLmsToRgb[3][3] = {
			{  0.0809444479, -0.1305044092,  0.1167210664 },
			{ -0.0102485335,  0.0540193266, -0.1136147082 },
			{ -0.0003652969, -0.0041216147,  0.6935114049 }
		};

		// ── CVD Dichromacy Projection in LMS Space (Viénot, Brettel & Mollon 1999)
		// Preserves the equi-energy neutral axis (white remains white).
		// Deuteranopia (M projection): M' = 0.494207 * L + 1.24827 * S
		constexpr double kCvdDeutan[3][3] = {
			{ 1.0,      0.0, 0.0     },
			{ 0.494207, 0.0, 1.24827 },
			{ 0.0,      0.0, 1.0     }
		};

		// Protanopia (L projection): L' = 2.02344 * M - 2.52581 * S
		constexpr double kCvdProtan[3][3] = {
			{ 0.0, 2.02344, -2.52581 },
			{ 0.0, 1.0,      0.0     },
			{ 0.0, 0.0,      1.0     }
		};

		// Tritanopia (S projection): S' = -0.395913 * L + 0.801109 * M
		constexpr double kCvdTritan[3][3] = {
			{  1.0,       0.0,      0.0 },
			{  0.0,       1.0,      0.0 },
			{ -0.395913,  0.801109, 0.0 }
		};

		// ── Daltonization Shift Matrices (Fidaner et al. 2005) ────────────────
		// Redistributes the lost color component (error) to visible channels.
		// Protanopia (L-defect, red lost): shift red error into green and blue
		constexpr double ShiftProtan[3][3] = {
			{ 0.0, 0.0, 0.0 },
			{ 0.7, 0.0, 0.0 },
			{ 0.7, 0.0, 0.0 }
		};
		// Deuteranopia (M-defect, green lost): shift green error into red and blue
		constexpr double ShiftDeutan[3][3] = {
			{ 0.0, 0.7, 0.0 },
			{ 0.0, 0.0, 0.0 },
			{ 0.0, 0.7, 0.0 }
		};
		// Tritanopia (S-defect, blue lost): shift blue error into red and green
		constexpr double ShiftTritan[3][3] = {
			{ 0.0, 0.0, 0.7 },
			{ 0.0, 0.0, 0.7 },
			{ 0.0, 0.0, 0.0 }
		};

		constexpr double Identity3[3][3] =
		{
			{ 1, 0, 0 },
			{ 0, 1, 0 },
			{ 0, 0, 1 }
		};

		void CopyMatrix(const double aSrc[3][3], double aDst[3][3])
		{
			for (int i = 0; i < 3; i++)
				for (int j = 0; j < 3; j++)
					aDst[i][j] = aSrc[i][j];
		}

		void Multiply(const double aA[3][3], const double aB[3][3], double aOut[3][3])
		{
			double result[3][3]{};
			for (int i = 0; i < 3; i++)
				for (int j = 0; j < 3; j++)
				{
					double sum = 0;
					for (int k = 0; k < 3; k++)
						sum += aA[i][k] * aB[k][j];
					result[i][j] = sum;
				}
			CopyMatrix(result, aOut);
		}

		void SimulationMatrix(BalanceType aType, double aOut[3][3])
		{
			// Computes: LmsToRgb * CvdSpace * RgbToLms
			const double (*cvd)[3] =
				(aType == BalanceType::Deutan) ? kCvdDeutan :
				(aType == BalanceType::Protan) ? kCvdProtan : kCvdTritan;

			double temp[3][3];
			Multiply(cvd, kRgbToLms, temp);
			Multiply(kLmsToRgb, temp, aOut);
		}


		void Add(const double aA[3][3], const double aB[3][3], double aOut[3][3])
		{
			for (int i = 0; i < 3; i++)
				for (int j = 0; j < 3; j++)
					aOut[i][j] = aA[i][j] + aB[i][j];
		}

		void Negate(const double aA[3][3], double aOut[3][3])
		{
			for (int i = 0; i < 3; i++)
				for (int j = 0; j < 3; j++)
					aOut[i][j] = -aA[i][j];
		}

		void Lerp(const double aA[3][3], const double aB[3][3], double aT, double aOut[3][3])
		{
			for (int i = 0; i < 3; i++)
				for (int j = 0; j < 3; j++)
					aOut[i][j] = aA[i][j] + (aB[i][j] - aA[i][j]) * aT;
		}

		double Clamp01(double aV)
		{
			return std::clamp(aV, 0.0, 1.0);
		}

		double ClampNeg1To1(double aV)
		{
			return std::clamp(aV, -1.0, 1.0);
		}

		// (Simulation is now fully consistent since both use the Brettel LMS matrices)
	}

	void ColorMatrix::CorrectionMatrix(BalanceType aType, double aSeverity01, double aOut3x3[3][3])
	{
		double clampedSev = std::clamp(aSeverity01, 0.0, 1.25);
		if (clampedSev <= 0.0001)
		{
			CopyMatrix(Identity3, aOut3x3);
			return;
		}

		double sim[3][3];
		SimulationMatrix(aType, sim);

		const double (*shift)[3] =
			(aType == BalanceType::Deutan) ? ShiftDeutan :
			(aType == BalanceType::Protan) ? ShiftProtan : ShiftTritan;

		// Out = In + (In - In*Sim) * Shift
		// As column vectors: Out = In + Shift * (In - Sim*In) = (I + Shift - Shift*Sim) * In
		// So Matrix M = I + Shift - Shift * Sim
		double errTimesSim[3][3];
		Multiply(shift, sim, errTimesSim);

		double negErrTimesSim[3][3];
		Negate(errTimesSim, negErrTimesSim);

		double sum[3][3];
		Add(Identity3, shift, sum);

		double full[3][3];
		Add(sum, negErrTimesSim, full);

		Lerp(Identity3, full, clampedSev, aOut3x3);
	}

	void ColorMatrix::MixedCorrectionMatrix(double aRgSeverity01, double aBySeverity01, double aOut3x3[3][3])
	{
		double rg[3][3];
		CorrectionMatrix(BalanceType::Deutan, aRgSeverity01, rg);

		double by[3][3];
		CorrectionMatrix(BalanceType::Tritan, aBySeverity01, by);

		Multiply(by, rg, aOut3x3);
	}

	MAGCOLOREFFECT ColorMatrix::ToMagColorEffect(const double aM3x3[3][3])
	{
		// MAGCOLOREFFECT.transform is a flat float[5][5], row-major.
		// Rows/cols 3-4 stay identity so alpha passes through untouched —
		// same layout the C# MagColorEffect struct used.
		MAGCOLOREFFECT effect{};
		effect.transform[0][0] = (float)aM3x3[0][0];
		effect.transform[1][0] = (float)aM3x3[0][1];
		effect.transform[2][0] = (float)aM3x3[0][2];
		effect.transform[0][1] = (float)aM3x3[1][0];
		effect.transform[1][1] = (float)aM3x3[1][1];
		effect.transform[2][1] = (float)aM3x3[1][2];
		effect.transform[0][2] = (float)aM3x3[2][0];
		effect.transform[1][2] = (float)aM3x3[2][1];
		effect.transform[2][2] = (float)aM3x3[2][2];
		effect.transform[3][3] = 1.0f;
		effect.transform[4][4] = 1.0f;
		return effect;
	}

	void ColorMatrix::SimulatePixel(double aR, double aG, double aB,
	                                BalanceType aType,
	                                double& aOutR, double& aOutG, double& aOutB)
	{
		// LMS-based physiological simulation (Brettel et al.).
		// This shows what a person with the given balance profile actually perceives -
		// full anopia severity, because we want to make the contrast difference visible.
		const double (*cvd)[3] =
			(aType == BalanceType::Deutan) ? kCvdDeutan :
			(aType == BalanceType::Protan) ? kCvdProtan : kCvdTritan;

		// RGB → LMS
		double l = kRgbToLms[0][0]*aR + kRgbToLms[0][1]*aG + kRgbToLms[0][2]*aB;
		double m = kRgbToLms[1][0]*aR + kRgbToLms[1][1]*aG + kRgbToLms[1][2]*aB;
		double s = kRgbToLms[2][0]*aR + kRgbToLms[2][1]*aG + kRgbToLms[2][2]*aB;

		// Apply CVD matrix in LMS space
		double ls = cvd[0][0]*l + cvd[0][1]*m + cvd[0][2]*s;
		double ms = cvd[1][0]*l + cvd[1][1]*m + cvd[1][2]*s;
		double ss = cvd[2][0]*l + cvd[2][1]*m + cvd[2][2]*s;

		// LMS → RGB
		aOutR = Clamp01(kLmsToRgb[0][0]*ls + kLmsToRgb[0][1]*ms + kLmsToRgb[0][2]*ss);
		aOutG = Clamp01(kLmsToRgb[1][0]*ls + kLmsToRgb[1][1]*ms + kLmsToRgb[1][2]*ss);
		aOutB = Clamp01(kLmsToRgb[2][0]*ls + kLmsToRgb[2][1]*ms + kLmsToRgb[2][2]*ss);
	}

	void ColorMatrix::ApplyPixel(double aR, double aG, double aB,
	                             const double aMatrix[3][3],
	                             double& aOutR, double& aOutG, double& aOutB)
	{
		// Applies any pre-computed 3×3 correction matrix to a single pixel.
		// Used in the UI to render the "with filter" commander-tag row without
		// touching the system-wide Magnification effect.
		aOutR = Clamp01(aMatrix[0][0]*aR + aMatrix[0][1]*aG + aMatrix[0][2]*aB);
		aOutG = Clamp01(aMatrix[1][0]*aR + aMatrix[1][1]*aG + aMatrix[1][2]*aB);
		aOutB = Clamp01(aMatrix[2][0]*aR + aMatrix[2][1]*aG + aMatrix[2][2]*aB);
	}
}

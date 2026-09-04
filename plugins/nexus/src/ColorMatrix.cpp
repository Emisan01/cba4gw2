#include "ColorMatrix.h"
#include <algorithm>

namespace cba
{
	namespace
	{
		// Simplified RGB simulation matrices per type (full severity = anopia).
		// Values taken verbatim from the exe's ColorMatrix.cs.
		constexpr double SimProtan[3][3] =
		{
			{ 0.56667, 0.43333, 0.00000 },
			{ 0.55833, 0.44167, 0.00000 },
			{ 0.00000, 0.24167, 0.75833 }
		};

		constexpr double SimDeutan[3][3] =
		{
			{ 0.62500, 0.37500, 0.00000 },
			{ 0.70000, 0.30000, 0.00000 },
			{ 0.00000, 0.30000, 0.70000 }
		};

		constexpr double SimTritan[3][3] =
		{
			{ 0.95000, 0.05000, 0.00000 },
			{ 0.00000, 0.43333, 0.56667 },
			{ 0.00000, 0.47500, 0.52500 }
		};

		// Redistributes the lost color component through the remaining channels.
		constexpr double ErrorRedistribution[3][3] =
		{
			{ 0.0, 0.0, 0.0 },
			{ 0.7, 1.0, 0.0 },
			{ 0.7, 0.0, 1.0 }
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

		void SimulationMatrix(DeficiencyType aType, double aOut[3][3])
		{
			switch (aType)
			{
				case DeficiencyType::Protan: CopyMatrix(SimProtan, aOut); return;
				case DeficiencyType::Deutan: CopyMatrix(SimDeutan, aOut); return;
				case DeficiencyType::Tritan: CopyMatrix(SimTritan, aOut); return;
				default:                     CopyMatrix(Identity3, aOut); return;
			}
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
	}

	void ColorMatrix::CorrectionMatrix(DeficiencyType aType, double aSeverity01, double aOut3x3[3][3])
	{
		double sim[3][3];
		SimulationMatrix(aType, sim);

		// M = I + Err - Err * Sim
		double errTimesSim[3][3];
		Multiply(ErrorRedistribution, sim, errTimesSim);

		double negErrTimesSim[3][3];
		Negate(errTimesSim, negErrTimesSim);

		double sum[3][3];
		Add(Identity3, ErrorRedistribution, sum);

		double full[3][3];
		Add(sum, negErrTimesSim, full);

		Lerp(Identity3, full, Clamp01(aSeverity01), aOut3x3);
	}

	void ColorMatrix::MixedCorrectionMatrix(double aRgSeverity01, double aBySeverity01, double aOut3x3[3][3])
	{
		double rg[3][3];
		CorrectionMatrix(DeficiencyType::Deutan, aRgSeverity01, rg);

		double by[3][3];
		CorrectionMatrix(DeficiencyType::Tritan, aBySeverity01, by);

		Multiply(by, rg, aOut3x3);
	}

	MAGCOLOREFFECT ColorMatrix::ToMagColorEffect(const double aM3x3[3][3])
	{
		// MAGCOLOREFFECT.transform is a flat float[5][5], row-major.
		// Rows/cols 3-4 stay identity so alpha passes through untouched —
		// same layout the C# MagColorEffect struct used.
		MAGCOLOREFFECT effect{};
		effect.transform[0][0] = (float)aM3x3[0][0];
		effect.transform[0][1] = (float)aM3x3[0][1];
		effect.transform[0][2] = (float)aM3x3[0][2];
		effect.transform[1][0] = (float)aM3x3[1][0];
		effect.transform[1][1] = (float)aM3x3[1][1];
		effect.transform[1][2] = (float)aM3x3[1][2];
		effect.transform[2][0] = (float)aM3x3[2][0];
		effect.transform[2][1] = (float)aM3x3[2][1];
		effect.transform[2][2] = (float)aM3x3[2][2];
		effect.transform[3][3] = 1.0f;
		effect.transform[4][4] = 1.0f;
		return effect;
	}
}

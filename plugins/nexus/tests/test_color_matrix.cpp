// Unit tests for core/ColorMatrix.cpp - the one part of this addon with zero
// ImGui/Nexus/Win32-runtime dependency, so it's the one part that can be
// tested standalone without a running GW2 process.
//
// These test documented mathematical INVARIANTS (white-point preservation,
// identity at zero severity, output clamping, the ToMagColorEffect transpose
// convention) rather than fixed "golden" output numbers. A test that just
// re-encodes the implementation's own arithmetic as its expected value can't
// catch a bug in that arithmetic - these can, because each property has to
// hold for reasons independent of exactly how the matrices are multiplied.
//
// No external test framework - single assert-style runner, consistent with
// this codebase's existing "simpler is better" stance (see the comment atop
// Settings.h about not pulling in JSON for six settings). Build with:
//   cmake --build build --config Release --target cba_tests
// then run build/bin/Release/cba_tests.exe - nonzero exit code = failure,
// output lists which check(s) failed.

#include "ColorMatrix.h"
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <initializer_list>

namespace
{
	int g_failures = 0;
	int g_checks = 0;

	void Check(bool aCondition, const char* aName)
	{
		++g_checks;
		if (!aCondition)
		{
			++g_failures;
			std::printf("FAIL: %s\n", aName);
		}
	}

	bool Near(double aA, double aB, double aEps = 1e-6)
	{
		return std::fabs(aA - aB) <= aEps;
	}

	bool MatrixNear(const double aA[3][3], const double aB[3][3], double aEps = 1e-6)
	{
		for (int i = 0; i < 3; ++i)
			for (int j = 0; j < 3; ++j)
				if (!Near(aA[i][j], aB[i][j], aEps))
					return false;
		return true;
	}

	const double kIdentity[3][3] = { {1,0,0}, {0,1,0}, {0,0,1} };
}

using namespace cba;

static void TestIdentityAtZeroSeverity()
{
	for (BalanceType t : { BalanceType::Protan, BalanceType::Deutan, BalanceType::Tritan })
	{
		double m[3][3];
		ColorMatrix::CorrectionMatrix(t, 0.0, m);
		Check(MatrixNear(m, kIdentity), "CorrectionMatrix(severity=0) is identity");
	}
}

static void TestSeverityIsClamped()
{
	// Negative severity must behave like 0 (documented range is 0.0-1.25).
	double mNeg[3][3];
	ColorMatrix::CorrectionMatrix(BalanceType::Deutan, -5.0, mNeg);
	Check(MatrixNear(mNeg, kIdentity), "CorrectionMatrix clamps negative severity to identity");

	// Above the documented max (1.25) must equal exactly at 1.25 (clamped).
	double mOver[3][3];
	double mMax[3][3];
	ColorMatrix::CorrectionMatrix(BalanceType::Protan, 999.0, mOver);
	ColorMatrix::CorrectionMatrix(BalanceType::Protan, 1.25, mMax);
	Check(MatrixNear(mOver, mMax), "CorrectionMatrix clamps severity above 1.25");
}

static void TestSimulatePixelPreservesWhite()
{
	// Documented invariant (ColorMatrix.cpp comment): the CVD projection
	// matrices preserve the equi-energy neutral axis - white stays white
	// under full-severity dichromacy simulation, for all three types.
	for (BalanceType t : { BalanceType::Protan, BalanceType::Deutan, BalanceType::Tritan })
	{
		double r, g, b;
		ColorMatrix::SimulatePixel(1.0, 1.0, 1.0, t, r, g, b);
		Check(Near(r, 1.0, 1e-3) && Near(g, 1.0, 1e-3) && Near(b, 1.0, 1e-3),
		      "SimulatePixel(white) stays white");

		double r0, g0, b0;
		ColorMatrix::SimulatePixel(0.0, 0.0, 0.0, t, r0, g0, b0);
		Check(Near(r0, 0.0, 1e-3) && Near(g0, 0.0, 1e-3) && Near(b0, 0.0, 1e-3),
		      "SimulatePixel(black) stays black");
	}
}

static void TestSimulatePixelOutputIsClamped()
{
	// Saturated/extreme inputs must not blow past [0,1] on the way out -
	// SimulatePixel clamps explicitly, this just checks that stuck.
	for (BalanceType t : { BalanceType::Protan, BalanceType::Deutan, BalanceType::Tritan })
	{
		double r, g, b;
		ColorMatrix::SimulatePixel(1.0, 0.0, 0.0, t, r, g, b);
		Check(r >= 0.0 && r <= 1.0 && g >= 0.0 && g <= 1.0 && b >= 0.0 && b <= 1.0,
		      "SimulatePixel(pure red) output stays in [0,1]");
	}
}

static void TestMixedAtZeroSeverityIsIdentity()
{
	double m[3][3];
	ColorMatrix::MixedCorrectionMatrix(0.0, 0.0, m);
	Check(MatrixNear(m, kIdentity), "MixedCorrectionMatrix(0,0) is identity");
}

static void TestCorrectionActuallyChangesAffectedPixel()
{
	// A correction matrix that equals identity at nonzero severity would be
	// a silent no-op bug - Protan at full severity must change a pixel that
	// isn't on the neutral axis (this is the whole point of the feature).
	double m[3][3];
	ColorMatrix::CorrectionMatrix(BalanceType::Protan, 1.0, m);
	Check(!MatrixNear(m, kIdentity, 1e-3), "CorrectionMatrix(severity=1.0) is not a no-op");

	double r, g, b;
	ColorMatrix::ApplyPixel(0.85, 0.27, 0.24, m, r, g, b); // GW2's reference "red" tag color
	Check(!Near(r, 0.85, 1e-3) || !Near(g, 0.27, 1e-3) || !Near(b, 0.24, 1e-3),
	      "CorrectionMatrix visibly shifts a saturated red pixel");
}

static void TestToMagColorEffectTransposeAndPassthrough()
{
	// ToMagColorEffect's comment says: MAGCOLOREFFECT.transform is row-major
	// float[5][5], and the 3x3 block gets TRANSPOSED into it (column-vector
	// convention for the Magnification API vs. our row-vector 3x3 math), with
	// rows/cols 3-4 staying pure identity so alpha passes through untouched.
	double m[3][3] = { {1,2,3}, {4,5,6}, {7,8,9} };
	MAGCOLOREFFECT e = ColorMatrix::ToMagColorEffect(m);

	bool transposedCorrectly = true;
	for (int row = 0; row < 3; ++row)
		for (int col = 0; col < 3; ++col)
			if (!Near((double)e.transform[col][row], m[row][col], 1e-5))
				transposedCorrectly = false;
	Check(transposedCorrectly, "ToMagColorEffect transposes the 3x3 block correctly");

	Check(Near((double)e.transform[3][3], 1.0) && Near((double)e.transform[4][4], 1.0),
	      "ToMagColorEffect sets identity alpha passthrough (diag)");

	bool restIsZero = true;
	for (int row = 0; row < 5; ++row)
		for (int col = 0; col < 5; ++col)
		{
			bool isDiag34 = (row == 3 && col == 3) || (row == 4 && col == 4);
			bool isIn3x3 = (row < 3 && col < 3);
			if (!isDiag34 && !isIn3x3 && !Near((double)e.transform[col][row], 0.0))
				restIsZero = false;
		}
	Check(restIsZero, "ToMagColorEffect leaves everything outside the 3x3 block and alpha diag at zero");
}

static void TestEyeComfortMatrixIsIdentityAtZero()
{
	double m[3][3];
	ColorMatrix::EyeComfortMatrix(0.0, 0.0, 0.0, m);
	Check(MatrixNear(m, kIdentity), "EyeComfortMatrix(0,0,0) is identity (module fully off is a true no-op)");
}

static void TestEyeComfortBlueFilterReducesBlue()
{
	double m[3][3];
	ColorMatrix::EyeComfortMatrix(1.0, 0.0, 0.0, m);
	double r, g, b;
	ColorMatrix::ApplyPixel(1.0, 1.0, 1.0, m, r, g, b);
	Check(b < 0.99, "Full blue filter visibly reduces blue on a white pixel");
	Check(Near(r, 1.0, 1e-3) && Near(g, 1.0, 1e-3), "Blue filter alone leaves red/green untouched");
}

static void TestEyeComfortSaturationReductionIsGreyscaleAtMax()
{
	double m[3][3];
	ColorMatrix::EyeComfortMatrix(0.0, 0.0, 1.0, m);
	double r, g, b;
	ColorMatrix::ApplyPixel(0.9, 0.2, 0.1, m, r, g, b);
	Check(Near(r, g, 1e-6) && Near(g, b, 1e-6), "Full saturation reduction produces a true greyscale output (R=G=B)");
}

// ── Pipeline composition (2026-09-11) ────────────────────────────────────
// These two back the tag-enhancer fix in UpdateTagEnhancerConflicts(): it
// used to score candidate replacement colours as Sim(colour), but what a
// user actually perceives is Sim(M x colour), because the DWM correction
// matrix M is applied to everything on screen including the scanner's own
// overlay markers. See COLOR_MATH.md section 8.

static double PerceivedDistance(BalanceType aType, const double aM[3][3],
                                const double aC1[3], const double aC2[3])
{
	double a[3], b[3];
	ColorMatrix::ApplyPixel(aC1[0], aC1[1], aC1[2], aM, a[0], a[1], a[2]);
	ColorMatrix::ApplyPixel(aC2[0], aC2[1], aC2[2], aM, b[0], b[1], b[2]);
	double sa[3], sb[3];
	ColorMatrix::SimulatePixel(a[0], a[1], a[2], aType, sa[0], sa[1], sa[2]);
	ColorMatrix::SimulatePixel(b[0], b[1], b[2], aType, sb[0], sb[1], sb[2]);
	double dr = sa[0] - sb[0], dg = sa[1] - sb[1], db = sa[2] - sb[2];
	return std::sqrt(dr * dr + dg * dg + db * db);
}

static void TestCorrectionIncreasesPerceivedSeparation()
{
	// The core premise of Fidaner et al. (2005) daltonization, and the reason
	// the tag enhancer MUST model M: for a pair that a dichromat confuses,
	// pushing it through the correction first has to leave it MORE separated
	// in perception, not less. GW2's own red and green commander tags are the
	// canonical confusable pair for a deutan.
	const double red[3]   = { 0.851, 0.275, 0.235 }; // #d9463c
	const double green[3] = { 0.247, 0.616, 0.302 }; // #3f9d4d

	double corrected[3][3];
	ColorMatrix::CorrectionMatrix(BalanceType::Deutan, 1.0, corrected);

	double uncorrectedDist = PerceivedDistance(BalanceType::Deutan, kIdentity, red, green);
	double correctedDist   = PerceivedDistance(BalanceType::Deutan, corrected, red, green);

	Check(correctedDist > uncorrectedDist,
	      "Correction increases a deutan's perceived red/green tag separation");
}

static void TestOmittingTheDisplayMatrixChangesTheAnswer()
{
	// Guards the specific bug that was fixed: evaluating perception WITHOUT
	// the display matrix is not an approximation of evaluating it WITH one -
	// it is a different answer. If this ever becomes false, the enhancer's
	// whole reason for calling EffectiveDisplayMatrix() is gone.
	const double tag[3] = { 0.247, 0.616, 0.302 };

	double corrected[3][3];
	ColorMatrix::CorrectionMatrix(BalanceType::Deutan, 1.0, corrected);

	double withM[3], withoutM[3];
	{
		double d[3];
		ColorMatrix::ApplyPixel(tag[0], tag[1], tag[2], corrected, d[0], d[1], d[2]);
		ColorMatrix::SimulatePixel(d[0], d[1], d[2], BalanceType::Deutan, withM[0], withM[1], withM[2]);
	}
	ColorMatrix::SimulatePixel(tag[0], tag[1], tag[2], BalanceType::Deutan,
	                           withoutM[0], withoutM[1], withoutM[2]);

	bool differs = !Near(withM[0], withoutM[0], 1e-3)
	            || !Near(withM[1], withoutM[1], 1e-3)
	            || !Near(withM[2], withoutM[2], 1e-3);
	Check(differs, "Sim(M x colour) and Sim(colour) are genuinely different results");
}

int main()
{
	TestIdentityAtZeroSeverity();
	TestSeverityIsClamped();
	TestSimulatePixelPreservesWhite();
	TestSimulatePixelOutputIsClamped();
	TestMixedAtZeroSeverityIsIdentity();
	TestCorrectionActuallyChangesAffectedPixel();
	TestToMagColorEffectTransposeAndPassthrough();
	TestEyeComfortMatrixIsIdentityAtZero();
	TestEyeComfortBlueFilterReducesBlue();
	TestEyeComfortSaturationReductionIsGreyscaleAtMax();
	TestCorrectionIncreasesPerceivedSeparation();
	TestOmittingTheDisplayMatrixChangesTheAnswer();

	std::printf("%d/%d checks passed\n", g_checks - g_failures, g_checks);
	return g_failures == 0 ? 0 : 1;
}

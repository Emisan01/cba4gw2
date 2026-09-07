#pragma once
#include <algorithm>
#include <cmath>

namespace cba
{
	// ── Conflict Resolution & Replacement Color Calculation ────────
	inline void RgbToHsv(float r, float g, float b, float& h, float& s, float& v)
	{
		float maxVal = r;
		if (g > maxVal) maxVal = g;
		if (b > maxVal) maxVal = b;

		float minVal = r;
		if (g < minVal) minVal = g;
		if (b < minVal) minVal = b;

		float delta = maxVal - minVal;
		v = maxVal;
		s = (maxVal > 1e-5f) ? (delta / maxVal) : 0.0f;
		if (delta < 1e-5f) {
			h = 0.0f;
		} else if (maxVal == r) {
			h = 60.0f * std::fmod(((g - b) / delta), 6.0f);
			if (h < 0.0f) h += 360.0f;
		} else if (maxVal == g) {
			h = 60.0f * (((b - r) / delta) + 2.0f);
		} else {
			h = 60.0f * (((r - g) / delta) + 4.0f);
		}
	}

	inline void HsvToRgb(float h, float s, float v, float& r, float& g, float& b)
	{
		float c = v * s;
		float hPrime = std::fmod(h / 60.0f, 6.0f);
		if (hPrime < 0.0f) hPrime += 6.0f;
		float x = c * (1.0f - std::abs(std::fmod(hPrime, 2.0f) - 1.0f));
		float m = v - c;
		if (hPrime < 1.0f)      { r = c; g = x; b = 0; }
		else if (hPrime < 2.0f) { r = x; g = c; b = 0; }
		else if (hPrime < 3.0f) { r = 0; g = c; b = x; }
		else if (hPrime < 4.0f) { r = 0; g = x; b = c; }
		else if (hPrime < 5.0f) { r = x; g = 0; b = c; }
		else                    { r = c; g = 0; b = x; }
		r += m; g += m; b += m;
		r = std::clamp(r, 0.0f, 1.0f);
		g = std::clamp(g, 0.0f, 1.0f);
		b = std::clamp(b, 0.0f, 1.0f);
	}

	inline float RelativeLuma(float r, float g, float b)
	{
		return 0.2126f * r + 0.7152f * g + 0.0722f * b;
	}
}

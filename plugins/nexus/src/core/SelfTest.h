#pragma once
#include <string>
#include <vector>

namespace cba
{
	struct SelfTestCheck
	{
		const char* category;
		const char* name;
		bool passed;
		// Informational status, not a hard pass/fail - e.g. "OS currently
		// blocking the DWM call" is an expected state under exclusive
		// fullscreen, not a bug. UI should render these distinctly from a
		// real red failure instead of alarming on them.
		bool isInfo = false;
		std::string detail; // empty on a clean pass, reason otherwise
	};

	// Runs a battery of read-only runtime checks: registry integrity, live
	// color-math invariants (same properties as tests/test_color_matrix.cpp,
	// run in-process instead of the separate offline cba_tests.exe), a
	// Settings export/import roundtrip, and the specific cross-field
	// consistency invariants behind the bugs found and fixed 2026-09-09
	// (Strength determinism, Auto-Brightness/GammaGain sync, slot/index
	// bounds). Never mutates CurrentSettings and never triggers a Recompute -
	// safe to run at any time, including mid-session with the filter active.
	//
	// What this can NOT catch: anything visual/perceptual (does the filter
	// look right, is a panel readable, did a slider move where expected) -
	// that still needs the human test checklist, this only covers what's
	// mechanically checkable from in-process state.
	std::vector<SelfTestCheck> RunSelfTest();
}

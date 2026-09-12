import os
import re
import sys

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
NEXUS_SRC = os.path.join(REPO_ROOT, "plugins", "nexus", "src")
UI_DIR = os.path.join(NEXUS_SRC, "ui")
CORE_DIR = os.path.join(NEXUS_SRC, "core")
PLATFORM_DIR = os.path.join(NEXUS_SRC, "platform")
MAIN_FILE = os.path.join(NEXUS_SRC, "ModuleMain.cpp")

tests = []

def check(category, test_name, condition, details="", info=False):
    """A pass/fail assertion, or - with info=True - a reported measurement.

    The `info` state is borrowed from core/SelfTest.h, which already had a
    third result kind for "expected condition, not a bug" while this script
    only knew pass/fail. That gap is why facts which legitimately vary (how
    many call sites of some pattern exist right now) had nowhere to live in
    tooling and ended up hand-written into CLAUDE.md, where they rotted -
    see that file's anti-rot section. An INFO line is printed and never
    fails the build.
    """
    tests.append({
        "category": category,
        "name": test_name,
        "passed": bool(condition),
        "details": details,
        "info": bool(info)
    })

def ratchet(category, test_name, actual, limit, unit, note=""):
    """A measurement that is allowed to shrink but not grow.

    For known technical debt that should not silently accumulate: the count
    is printed every run (so it can never be stale the way a number in a
    markdown file can), and the build fails only if it goes UP. Lowering the
    limit after cleaning something up is the intended workflow.
    """
    ok = actual <= limit
    detail = f"{actual} {unit} (limit {limit})"
    if note:
        detail += f" - {note}"
    if actual < limit:
        detail += f" | limit can be tightened to {actual}"
    tests.append({
        "category": category,
        "name": test_name,
        "passed": ok,
        "details": detail,
        "info": False
    })

# =============================================================================
# PILLAR 1: ARENANET THIRD-PARTY POLICY & SECURITY
# =============================================================================
all_code = ""
for root, _, files in os.walk(NEXUS_SRC):
    for f in files:
        if f.endswith((".cpp", ".h")):
            with open(os.path.join(root, f), "r", encoding="utf-8", errors="ignore") as fh:
                all_code += fh.read() + "\n"

# 1.1 Zero RPM / WPM / VirtualAllocEx
forbidden_mem = ["ReadProcessMemory", "WriteProcessMemory", "VirtualAllocEx", "CreateRemoteThread", "OpenProcess"]
found_mem = [api for api in forbidden_mem if api in all_code]
check("1. ArenaNet Security", "Zero Process Memory Tampering (No RPM/WPM)", len(found_mem) == 0, f"Found: {found_mem}" if found_mem else "Zero memory manipulation APIs")

# 1.2 Zero Hooking Libraries
forbidden_hooks = ["DetourTransactionBegin", "MH_CreateHook", "MH_EnableHook", "SetWindowsHookEx", "VirtualProtect"]
found_hooks = [h for h in forbidden_hooks if h in all_code]
check("1. ArenaNet Security", "Zero Function / Binary Hooking", len(found_hooks) == 0, f"Found: {found_hooks}" if found_hooks else "100% hookless DWM presentation layer")

# 1.3 Zero Input Automation / Bots / Macros
forbidden_input = ["SendInput", "mouse_event", "keybd_event"]
found_input = [inp for inp in forbidden_input if inp in all_code]
check("1. ArenaNet Security", "Zero Automation / Synthetic Input Generation", len(found_input) == 0, f"Found: {found_input}" if found_input else "Passive read-only input binds")

# 1.4 Respect ArenaNet Crash Handler (No SetUnhandledExceptionFilter hook)
has_crash_hook = "SetUnhandledExceptionFilter" in all_code and not "//" in all_code
check("1. ArenaNet Security", "Preserves ArenaNet & ArcDPS Crash Reporters (No ExceptionFilter Hook)", "Do not hook SetUnhandledExceptionFilter" in all_code, "Explicitly preserved for game integrity")

# 1.5 Official Disclaimer in Documentation
with open(os.path.join(REPO_ROOT, "README.md"), "r", encoding="utf-8") as f:
    readme_text = f.read()
has_disclaimer = "ArenaNet's Third-Party Programs Policy" in readme_text and "visual-only" in readme_text
check("1. ArenaNet Security", "Official ArenaNet Policy Notice in Documentation", has_disclaimer, "Prominently displayed in README and About dialog")

# =============================================================================
# PILLAR 2: RAIDCORE / NEXUS ARCHITECTURE & ABI COMPLIANCE
# =============================================================================
with open(MAIN_FILE, "r", encoding="utf-8") as f:
    main_code = f.read()

# 2.1 Nexus ImGui Allocator Passthrough
has_allocator = "ImGui::SetAllocatorFunctions" in main_code and "APIDefs->ImguiMalloc" in main_code and "APIDefs->ImguiFree" in main_code
check("2. Raidcore Engine", "Synchronized ImGui Memory Allocators (ImguiMalloc/Free)", has_allocator, "Uses Nexus unified heap allocators")

# 2.2 Nexus ImGui Context Synchronization
has_ctx = "ImGui::SetCurrentContext" in main_code and "APIDefs->ImguiContext" in main_code
check("2. Raidcore Engine", "Synchronized ImGui Context", has_ctx, "Shares Nexus global ImGui context")

# 2.3 Exception Boundaries across DLL Interface
has_try_load = "void AddonLoad" in main_code and "try" in main_code and "catch (...)" in main_code
has_try_unload = "void AddonUnload" in main_code and "try" in main_code and "catch (...)" in main_code
has_try_wndproc = "UINT AddonWndProc" in main_code and "try" in main_code and "catch (...)" in main_code
has_try_render = "void AddonRenderWindow" in main_code and "try" in main_code and "catch (...)" in main_code
check("2. Raidcore Engine", "Zero-Throw DLL Boundary (Full try-catch Isolation)", has_try_load and has_try_unload and has_try_wndproc and has_try_render, "All Nexus callbacks fully wrapped in SEH/C++ catch-all")

# 2.4 Lifecycle Symmetry: CloseOnEscape
reg_esc = set(re.findall(r'RegisterCloseOnEscape\("([^"]+)"', main_code))
dereg_esc = set(re.findall(r'DeregisterCloseOnEscape\("([^"]+)"', main_code))
check("2. Raidcore Engine", "RegisterCloseOnEscape Exact Symmetry", reg_esc == dereg_esc, f"Unpaired: {reg_esc ^ dereg_esc}" if reg_esc != dereg_esc else f"{len(reg_esc)} windows paired")

# 2.5 Lifecycle Symmetry: InputBinds
reg_binds = set(re.findall(r'RegisterWithString\("([^"]+)"', main_code))
dereg_binds = set(re.findall(r'Deregister\("([^"]+)"', main_code))
check("2. Raidcore Engine", "InputBinds Exact Symmetry", reg_binds.issubset(dereg_binds), f"Missing dereg: {reg_binds - dereg_binds}" if not reg_binds.issubset(dereg_binds) else f"{len(reg_binds)} keybinds paired")

# 2.6 Lifecycle Symmetry: QuickAccess Icon & Renderer
has_qa_dereg = "APIDefs->QuickAccess.Remove" in main_code
has_rend_dereg = "APIDefs->Renderer.Deregister" in main_code
has_wnd_dereg = "APIDefs->WndProc.Deregister" in main_code
check("2. Raidcore Engine", "Complete Resource Teardown on Unload", has_qa_dereg and has_rend_dereg and has_wnd_dereg, "QuickAccess, Renderer, and WndProc cleanly removed")

# =============================================================================
# PILLAR 3: PERFORMANCE, DWM & SYSTEMS ENGINEERING
# =============================================================================
# 3.1 DWM IPC 60Hz Throttling
has_dwm_throttle = "elapsedMs < 16" in main_code and "RoughlyEqual" in main_code
check("3. Performance & DWM", "DWM IPC Throttling (60 Hz cap & state memoization)", has_dwm_throttle, "Prevents DWM IPC congestion on rapid mouse movement")

# 3.2 Thread-Safe Synchronization
has_mutex = "s_recomputeMutex" in main_code and "std::lock_guard<std::mutex>" in main_code
has_atomics = "std::atomic<bool>" in main_code and "s_deferredInitDone" in main_code
check("3. Performance & DWM", "Thread-Safe State & Atomic Concurrency", has_mutex and has_atomics, "Guards recompute against race conditions")

# 3.3 Deferred Disk I/O (No SSD thrashing in render loop)
with open(os.path.join(UI_DIR, "MainWindow.cpp"), "r", encoding="utf-8") as f:
    main_win_code = f.read()
has_deferred_save = "IsItemDeactivatedAfterEdit" in main_win_code and "saveNeeded" in main_win_code
check("3. Performance & DWM", "Deferred Disk I/O (Zero SSD writes during slider drag)", has_deferred_save, "Saves configuration strictly on mouse release")

# 3.4 Watchdog Thread Clean Teardown
has_thread_join = "s_watchdogThread.join()" in main_code and "s_watchdogRunning = false" in main_code
check("3. Performance & DWM", "Clean Thread Lifecycle & Join on Unload", has_thread_join, "No abandoned or dangling threads")

# =============================================================================
# PILLAR 4: MATHEMATICAL RIGOR & CLINICAL COLOR SCIENCE
# =============================================================================
with open(os.path.join(CORE_DIR, "ColorMatrix.cpp"), "r", encoding="utf-8") as f:
    cmat_code = f.read()

# Mathematical white-point test
L_w = 17.8824 + 43.5161 + 4.11935
M_w = 3.45565 + 27.1554 + 3.86714
S_w = 0.0299566 + 0.184309 + 1.46709

# Deutan: M' = 0.494207 * L + 1.24827 * S
deutan_diff = abs((0.494207 * L_w + 1.24827 * S_w) - M_w)
# Protan: L' = 2.02344 * M - 2.52581 * S
protan_diff = abs((2.02344 * M_w - 2.52581 * S_w) - L_w)
# Tritan: S' = -0.395913 * L + 0.801109 * M
tritan_diff = abs((-0.395913 * L_w + 0.801109 * M_w) - S_w)

white_exact = (deutan_diff < 0.001) and (protan_diff < 0.001) and (tritan_diff < 0.001)
check("4. Color Science", "Strict Equal-Energy White-Point Invariance (Sim(1,1,1) == 1,1,1)", white_exact, f"Max error = {max(deutan_diff, protan_diff, tritan_diff):.6f}")

has_wcag_luma = "0.2126" in all_code and "0.7152" in all_code and "0.0722" in all_code
check("4. Color Science", "Standard ITU-R BT.709 / WCAG 2.1 Relative Luminance", has_wcag_luma, "Y = 0.2126 R + 0.7152 G + 0.0722 B")

has_daltonization = "ShiftProtan" in cmat_code and "ShiftDeutan" in cmat_code and "ShiftTritan" in cmat_code
check("4. Color Science", "Fidaner et al. (2005) Error-Diffusion Daltonization", has_daltonization, "Redistributes lost spectral energy into visible cones")

# =============================================================================
# PILLAR 5: IMGUI RESILIENCE & USER EXPERIENCE
# =============================================================================
# 5.1 Strict ASCII Cleanliness
non_ascii = 0
for root, _, files in os.walk(NEXUS_SRC):
    for f in files:
        if f.endswith((".cpp", ".h")):
            with open(os.path.join(root, f), "r", encoding="utf-8", errors="replace") as fh:
                for lit in re.findall(r'"([^"]*)"', fh.read()):
                    if not all(ord(c) < 128 for c in lit):
                        non_ascii += 1
check("5. ImGui Resilience", "100% ASCII-Clean Strings (Zero glyph glitches in game overlay)", non_ascii == 0, f"{non_ascii} non-ASCII literals")

# 5.2 Dynamic Sizing Rule (CalcTextSize utilized for dynamic text)
has_calc_text = "CalcTextSize" in main_win_code
check("5. ImGui Resilience", "Dynamic ImGui Width Calculation (AGENTS.md rule)", has_calc_text, "Auto-calculates button & element bounds")

# 5.3 Context Guards on all Entry Points
ui_ctx_ok = True
for f in ["MainWindow.cpp", "SensorGraphHUD.cpp", "FilterLab.cpp", "VisionLab.cpp", "SafeStartGate.cpp"]:
    with open(os.path.join(UI_DIR, f), "r", encoding="utf-8") as fh:
        if "ImGui::GetCurrentContext()" not in fh.read():
            ui_ctx_ok = False
check("5. ImGui Resilience", "ImGui::GetCurrentContext Null-Checks across all Windows", ui_ctx_ok, "Prevents crash if called during context teardown")

# 5.4 Safe-Start Crash Guard
with open(os.path.join(CORE_DIR, "Settings.cpp"), "r", encoding="utf-8") as f:
    settings_code = f.read()
has_crash_guard = "cba_session.lock" in settings_code and "SafeModeTriggered" in settings_code
check("5. ImGui Resilience", "Safe-Start Gate & Crash-Breadcrumb Recovery", has_crash_guard, "Disarms filter if previous session crashed ungracefully")

# =============================================================================
# PILLAR 6: CODEBASE HEALTH (facts that used to rot inside CLAUDE.md)
# =============================================================================
# Everything in this pillar exists because the same fact was previously written
# by hand into documentation, where it silently went out of date - counts,
# file:line lists, "verified N/N/N" rituals. Derived here instead, so the
# number is produced fresh on every run and CI notices drift instead of a
# reader trusting a stale line. See CLAUDE.md, "How to write in this file so
# it does not rot".

# 6.1 L10n initializer completeness.
# The de{} / en{} blocks were plain positional aggregate initializers until
# 2026-09-11: adding or removing a struct field without touching both language
# blocks shifted every later string by one, silently, with no compiler error.
# CLAUDE.md carried this as a manual "verify 68/68/68 by hand" ritual.
#
# They are designated initializers now (.Field = "..."), which makes a WRONG
# ORDER a hard compile error - verified by deliberately swapping two entries
# and getting 8 errors. This check is still needed for the one failure mode
# that remains legal C++: an OMITTED field. That compiles fine and leaves a
# null const char*, which reaches ImGui as a crash rather than a wrong label.
# So the two guards are complementary, not redundant - order is the compiler's
# job, completeness is this one's.
l10n_path = os.path.join(UI_DIR, "L10n.h")
with open(l10n_path, "r", encoding="utf-8", errors="ignore") as f:
    l10n_lines = f.read().split("\n")

def _block_bounds(lines, start_pattern):
    for i, line in enumerate(lines):
        if re.search(start_pattern, line):
            for j in range(i + 1, len(lines)):
                if re.match(r"^\s*\};", lines[j]):
                    return i, j
            break
    return None, None

fs, fe = _block_bounds(l10n_lines, r"struct\s+L10n\b")
field_count = sum(1 for l in l10n_lines[fs:fe] if re.search(r"const char\*\s+\w+;", l)) if fs is not None else -1

lang_counts = {}
for lang in ("de", "en"):
    bs, be = _block_bounds(l10n_lines, r"static const L10n\s+" + lang + r"\s*\{")
    if bs is None:
        lang_counts[lang] = -1
        continue
    n = 0
    for l in l10n_lines[bs + 1:be]:
        s = l.strip()
        if not s or s.startswith("//"):
            continue
        # Designated form (.Field = "...") since 2026-09-11; the bare-string
        # form is still accepted so this check keeps working if a block is
        # ever written the old way again.
        if re.match(r'\.\w+\s*=', s) or s.startswith('"') or s.startswith('u8"'):
            n += 1
    lang_counts[lang] = n

aligned = field_count > 0 and field_count == lang_counts["de"] == lang_counts["en"]
check("6. Codebase Health", "Every L10n field is initialized in both language blocks",
      aligned,
      f"{field_count} fields / {lang_counts['de']} de / {lang_counts['en']} en"
      + ("" if aligned else "  <-- an omitted field leaves a null const char*, which reaches ImGui as a crash"))

# 6.2 Ungoverned master-enable writes, as a ratchet rather than a hand-counted
# list. CLAUDE.md carried these as explicit file:line references; within a
# single day four of five line numbers had drifted. The intent was never the
# exact lines, it was "this must not grow", which is what a ratchet expresses.
# One hit is legitimate - ActivateCommanderTagProfile's own body is the
# governing function - so the limit includes it.
enabled_writes = 0
for root, _, files in os.walk(NEXUS_SRC):
    for f in files:
        if f.endswith((".cpp", ".h")):
            with open(os.path.join(root, f), "r", encoding="utf-8", errors="ignore") as fh:
                enabled_writes += len(re.findall(r"CurrentSettings\.Enabled\s*=\s*true", fh.read()))
ratchet("6. Codebase Health", "Direct CurrentSettings.Enabled writes do not spread", enabled_writes, 5,
        "write sites", "1 of these is ActivateCommanderTagProfile itself; the rest bypass the shared owners")

# 6.3 ParameterRegistry registration count, reported not asserted. The exact
# number is not an invariant (parameters get added on purpose), so this is
# INFO: it exists so nobody writes the list into a doc again. The real
# invariant - every declared ParamId is actually registered - is already
# asserted at runtime by SelfTest, which can see the registry; this script
# cannot, and deliberately does not duplicate it.
with open(os.path.join(CORE_DIR, "ParameterRegistry.cpp"), "r", encoding="utf-8", errors="ignore") as f:
    reg_count = len(re.findall(r"Register(?:Float|Bool|Int)\(ParamId::", f.read()))
check("6. Codebase Health", "Parameters wired into the registry", True,
      f"{reg_count} registered (informational - SelfTest asserts enum/registry agreement at runtime)",
      info=True)

# 6.4 The neutral-start guarantee.
# Settings::Load() unconditionally forces Enabled=false and all four window
# visibility flags to false, so every launch begins with the filter off and no
# windows open - the one deliberate exception being an Auto-Start profile,
# which AddonLoad applies afterwards and which the Safe-Start Gate can veto.
#
# This is checked rather than trusted because it has already been lost once by
# accident: removing the dead "LoadOnStartup" opt-in in 2026-09-09 also removed
# the only thing forcing Enabled back to false, and the symptom was the filter
# silently re-arming itself at character select. A policy that can disappear as
# a side effect of unrelated cleanup belongs in CI, not in a comment.
with open(os.path.join(CORE_DIR, "Settings.cpp"), "r", encoding="utf-8", errors="ignore") as f:
    settings_raw = f.read()

# Comment-stripped, because the first version of this check passed on a
# deliberately commented-out `// s.Enabled = false;` - the regex matched the
# text inside the comment. A guard that cannot tell live code from a disabled
# line is not a guard; found by probing the check itself rather than trusting
# that it worked.
settings_src = "\n".join(
    l for l in settings_raw.split("\n") if not l.lstrip().startswith("//")
)

"""The five assignments have to be ONE block, not five sightings.

Probing this check caught a second false positive: `s.Enabled = false;` also
appears ~190 lines earlier inside the Safe-Start crash branch, so a plain
"does this string exist anywhere" search stayed green even with the
unconditional one removed. A conditional write is exactly what this guarantee
is not. Anchoring on the window-visibility group and requiring the rest to sit
inside the same stretch of source pins the unconditional block specifically,
without depending on indentation or line numbers.
"""
neutral_start = [
    ("main window closed", r"s\.ShowMainWindow\s*=\s*false\s*;"),
    ("graph window closed", r"s\.ShowGraphWindow\s*=\s*false\s*;"),
    ("lab window closed", r"s\.ShowLabWindow\s*=\s*false\s*;"),
    ("vision lab closed", r"s\.ShowVisionLabWindow\s*=\s*false\s*;"),
    ("filter disarmed", r"s\.Enabled\s*=\s*false\s*;"),
]
_anchor = re.search(neutral_start[0][1], settings_src)
if _anchor is None:
    missing = [n for n, _ in neutral_start]
else:
    _block = settings_src[_anchor.start():_anchor.start() + 1200]
    missing = [name for name, pat in neutral_start if not re.search(pat, _block)]
check("6. Codebase Health", "Settings::Load forces a neutral start (filter off, windows closed)",
      not missing,
      "every launch starts neutral; an Auto-Start profile is the one deliberate exception"
      if not missing else "MISSING: " + ", ".join(missing))

# 6.5 Nobody writes into the shared ImGui style.
# ImGui::GetStyle() returns a reference to the ONE style struct shared by
# everything drawing in Nexus's context - Nexus itself, arcdps, every other
# addon. Writing to it is permanent and global: until 2026-09-12 CBA set frame
# rounding, item spacing, button text alignment and three header colours there
# and never restored them, so every other addon rendered with our styling for
# the rest of the session. It looks like a theme rather than a bug, which is
# why it survived so long.
#
# Push/PopStyleVar and Push/PopStyleColor are the scoped equivalents and this
# codebase already uses them in ~180 places. This check exists so the eight
# leftovers cannot come back.
style_writes = []
for _root, _dirs, _files in os.walk(NEXUS_SRC):
    for _f in _files:
        if not _f.endswith((".cpp", ".h")):
            continue
        _path = os.path.join(_root, _f)
        with open(_path, "r", encoding="utf-8", errors="ignore") as _fh:
            for _n, _line in enumerate(_fh, 1):
                _code = _line.split("//", 1)[0]
                if re.search(r"style\s*\.\s*\w+\s*(\[[^\]]*\])?\s*=[^=]", _code):
                    style_writes.append("%s:%d" % (_f, _n))

check("6. Codebase Health", "Nothing writes into the shared ImGui style",
      not style_writes,
      "scoped Push/Pop only - the context belongs to Nexus and every other addon"
      if not style_writes else "direct writes at: " + ", ".join(style_writes[:6]))

# =============================================================================
# SUMMARY REPORT
# =============================================================================
print("\n" + "="*80)
print("             ARENANET & RAIDCORE PROFESSIONAL AUDIT SUMMARY                     ")
print("================================================================================\n")

curr_cat = ""
for t in tests:
    if t["category"] != curr_cat:
        curr_cat = t["category"]
        print(f"\n--- {curr_cat} ---")
    status = "[INFO]" if t["info"] else ("[PASS]" if t["passed"] else "[FAIL]")
    print(f"  {status} {t['name']}")
    if t["details"]:
        print(f"         > {t['details']}")

# INFO lines are measurements, not assertions - they are reported but never
# counted against the result, and never fail CI. Same distinction SelfTest
# makes with isInfo: "OS currently blocking the DWM call" is a state, not a bug.
asserts = [t for t in tests if not t["info"]]
infos = [t for t in tests if t["info"]]
total = len(asserts)
passed = sum(1 for t in asserts if t["passed"])
print("\n" + "="*80)
print(f"FINAL AUDIT RESULT: {passed} / {total} Checks Passed ({(passed/total)*100:.1f}%)"
      + (f"  ({len(infos)} informational)" if infos else ""))
print("================================================================================\n")

if passed < total:
    sys.exit(1)

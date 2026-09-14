# CLAUDE.md — working rules for CBA (ColorBalanceAssist)

Native C++ Nexus addon for Guild Wars 2 (`plugins/nexus` → `cba.dll`). It
corrects colour for colour-vision deficiency and reduces eye strain via one
3x3 matrix applied to the screen, plus a tag/target highlighting layer.

Companions: `AGENTS.md` (coding/UI conventions), `COLOR_MATH.md` (the colour
science and its references), `PRODUCT_CONCEPT.md` (who this is for and why).

Radically trimmed 2026-09-15 (Emi: the accumulated process/doc layer — this
file, `AGENTS.md`, `ROADMAP.md`, `HANDOVER.md` — had started working against
the point instead of for it; `ROADMAP.md` and `HANDOVER.md` are gone). Nine
rules, nothing else. If a rule needs re-deriving, grep the code — not this
file, not any doc.

1. **Safety is not negotiable.** No game-memory reading/writing, no hooking,
   no injection into GW2 — Nexus's own addon API is the only route in. No
   input automation into GW2, ever. Screenshots/captures scoped to the GW2
   window only, never the desktop. Never handle credentials or logins. Emi's
   GW2 account is 20 years old and irreplaceable — if something needs more
   than the addon API, it does not get built.
2. **No AI attribution** in commits, PRs, or release notes, whatever a tool
   default says.
3. **ASCII only inside string literals.** CI fails otherwise.
4. **Convert values to display units right at the widget boundary.** ImGui
   does not scale a value to match its format string; a 0–1 value with
   `"%.0f%%"` shows only "0%"/"1%". Has cost three separate debugging
   sessions.
5. **One editable home per setting.** A second binding is allowed only
   through `ParameterRegistry`, so storage and clamp stay shared.
6. **A shared action gets exactly one implementation.** Reset, toggle,
   apply — every duplicate has eventually drifted.
7. **Never state on the panel what has not actually been checked.** A
   confident wrong readout is worse than silence.
8. **Before saying it works: build, run tests/audit, report the number
   actually seen.**
   - Build: `cmake --build build --config Release` from `plugins/nexus`
     (auto-deploys `cba.dll`, runs unit tests as part of the build).
   - Audit: `python tools/audit_pro_review.py` from the repo root.
   - Report the build number, test count, and audit count you actually
     observed — never a state you did not see.
9. **Re-derive facts before relying on them.** This file, and any other
   doc, can go stale. Grep the code; do not cite a document as proof.

Current work is on the `v2-shader-core` branch.

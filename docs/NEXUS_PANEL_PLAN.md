# Nexus-Panel: Umbauplan

Ergebnis eines Design-Workflows (11 Agenten, 2026-09-12): Inventar, vier
unabhaengige Entwuerfe aus vier Prioritaeten, drei Juroren mit drei Brillen,
eine Synthese. Gewaehlte Basis: **Proposal 2 - "Drei Rubriken, eine Schublade, eine Zeile"**

> **Noch nichts davon ist umgesetzt.** Offene Fragen an Emi stehen unten und
> muessen vor Commit F/H beantwortet sein.

Alle Zeilennummern sind zum Zeitpunkt der Analyse gueltig und driften.
Vor dem Anfassen neu herleiten (Regel 22).

---

## Ziel-Layout

### 1. Steuerung / Controls

*An, aus, Profil wechseln - und zwei Wege zurueck, wenn etwas verstellt ist.*

- ROW 1, code unchanged (MainWindow.cpp:365-521): [EIN/AUS] (ToggleMasterEnabled, saveNeeded=false stays - it saves internally) | live status field | [UI zuruecksetzen] (ResetUiLayout) | [Filter zuruecksetzen] (ResetFilterSettingsAndDisable). Both resets are already shared implementations; both tooltips stay verbatim.
- The status field's channel-split ground moves into a shared DrawReadoutField() helper (grafted from P3). It is the panel's only 'this is a reading, not a label' idiom and today it exists as ~70 inline lines in exactly one place. Behaviour identical - keep the ChannelsSplit, do NOT replace it with pre-measure-then-draw: the draw list has no z-order and the comment at :421-427 records why.
- OUT of the status field: the '(Commander Tag, Eye-Sensitive, ...)' FeatureModuleRegistry list. Nothing is lost for a non-Advanced user because after this pass every module reports its own state in its own rubric - the Eye Comfort checkbox is visible in section 2, the tag status line in section 3. Do NOT justify this by pointing at SensorGraphHUD.cpp:1143: that surface is behind the Advanced gate, so for the panel's own audience it is not a second home.
- NEW, conditional, zero rows when healthy: when RenderBackend==1 && !ShaderColorPipeline::IsReady(), one gold line under the status field printing LastError(). This is the reading half of :1396-1411 and the only part of the backend block a player can act on. Without it, a non-Advanced user whose shader fails to build sees a status field saying the filter is on while nothing is painted - the panel stating something it has not checked.
- ROW 2 - Profile, UNGATED: 'Profile:' + [Speichern] + chips [1][2][3] + the gold autostart asterisk. Save and the chips ship in the SAME commit and are never split: the chip is the save's only visible confirmation (it lights up and the asterisk stays put), and a save with no retrieval path on the same surface is what sank P4.
- ROW 3 - Autostart, one row: DrawAutoStartControl(aCompact=true) with the slot radios inline instead of indented on their own line, and 'Nach einem Absturz wird das uebersprungen.' moved into the checkbox tooltip. Only the aCompact branch changes; the Main Window's full version at :2502 is untouched. The no-slot sentence at :231 stays VISIBLE TEXT, not a tooltip - this ImGui has no BeginDisabled - and it now points at a Save button that finally sits on the same gate level as it, one row above.
- ROW 4 - [x] Erweiterter Modus + [Studio oeffnen ->] / [Studio schliessen] on one row (:564-613), code unchanged, tooltip reworded (see removals/open questions). Sole writer of AdvancedModeUnlocked in the tree.
- OUT: 'Im Hintergrund aktiv lassen' + its two consequence lines (:523-562).

### 2. Augenschonung / Eye Comfort

*Nimmt dem Bild die Grelle bei langen Sessions - unabhaengig von der Farbkorrektur.*

- 'Aktivieren' checkbox (:734-744), unchanged, ungated, tooltip unchanged. Retention feature: PRODUCT_CONCEPT 2D forbids it sitting behind the Advanced gate or behind a collapsed fold.
- The dead-end guard (:745-764) carried over VERBATIM: amber 'Wirkt erst, wenn der Filter oben an ist.' + [Jetzt einschalten]. Zero rows whenever the master filter is on. Not cosmetic - Recompute() returns early while the master filter is off, so without it the three sliders move and change nothing.
- Three sliders, one row each instead of two, and NOT the way any of the four proposals did it. Delete the TextDisabled label row and pass the label to SliderFloat normally - ImGui draws a visible label to the RIGHT of the widget natively, so this needs no SameLine, no width math, no conditional stacked fallback and no folding of text into the format string. One code path, not two. Reserve room for the Reset button with PushItemWidth before the call. The *100.0f read and /100.0f write at :788/:791 are carried over CHARACTER FOR CHARACTER, written out three times, never a loop or a table - the format strings stay "%.0f%%" on a 0..100 display value.
- Brightness, still OUTSIDE the Aktivieren gate as today (it works on the plain correction too - deliberate, :818-823), reduced from nine rows to TWO: row A 'Erhalt: NN%' alone, cyan, live from GetBrightnessRetention(); row B [SliderFloat GammaGain 0.70-1.30 via SetGammaGainManual][Reset 1.00x][Auto].
- The retention percentage stays a VISIBLE ROW, not a tooltip. All four proposals moved it into one, and all three judges independently flagged that as the weakest possible place for the panel's one measured number. What goes instead is the recommendation value, the sub-heading, the explainer line and the Optimalwert button - the recommendation survives inside the Auto tooltip because that is the control it describes.
- The Auto checkbox is routed through a NEW shared SetAutoBrightness(bool) in ModuleMain.cpp beside the existing SetGammaGainManual (ModuleMain.cpp:600), and MainWindow.cpp:2724 plus SensorGraphHUD.cpp:1060 are repointed at it in the same commit.
- KEEP the invisible SyncAutoBrightnessGain(changed, saveNeeded) call at :830 even though most of the block around it goes. It renders nothing and it is the per-frame drift correction; drop it and Auto silently stops tracking whenever this panel is the only open surface.

### 3. Commander-Tag-Kontrast / Commander Tag Contrast

*Verschiebt nur die Tag-Farben, die du tatsaechlich verwechselst.*

- The subtitle above is the existing line at :947, now passed as PanelSection's aSubtitle argument instead of being hand-written six lines under a call that passes nullptr (:936). That is exactly the drift the helper's own comment at :305-324 exists to stop.
- OUT: the two extra pitch lines at :951-956. One repeats the subtitle; the other advertises Eye Comfort under the acquisition feature's heading.
- The three-step guided flow (:971-1193) carried over UNCHANGED and still auto-starting at :1048. Not compacted, not moved, not turned into a button. It is the only ungated editor of Type and Severity in the entire product (the only Severity01 slider is SensorGraphHUD.cpp:928, behind the gate), and it already collapses to two rows the instant a profile exists. All mechanical guards stay: 46px tiles, the >=60px width floors at :1014-1015 and :1143 (a non-positive InvisibleButton size inside someone else's narrow column is an ImGui assert), and the step-3 preview building its matrix from explicit parameters rather than ActiveCorrectionMatrix so it cannot race the Watchdog.
- THE ONE CHANGE to the entry: the gold 'Noch nicht eingerichtet' hint (:958-968) changes its condition from 'unconfigured' to 'unconfigured AND s_setupDismissed', and becomes a real [Sehtest starten] SmallButton that sets s_setupStep = 1. Today it renders directly above the very flow it describes - a caption on its own content - and a user who clicks 'Ich kann alle gut unterscheiden' has no visible way back into the flow for the rest of the session. This deletes a duplicate line and creates the missing re-entry path in one condition change, with no new static and no new row. Because the flow is only suppressed when s_setupDismissed is true, the button is never a no-op - which is precisely how P1's version failed.
- s_setupDismissed STAYS and keeps its session-only semantics. Do not delete it: the comment at :1038-1044 records that it is the only thing making the 'I can tell them all apart' button stick, and without it the user is dropped back into question 1 on the next frame.
- MOVED UP out of the Advanced fold: the three [Protan] [Deutan] [Tritan] buttons (:1416-1445) become step 1's 'Ich kenne meinen Typ' branch, with a Back button. Same shared ActivateCommanderTagProfile(), one implementation, and the control finally sits inside the flow it always belonged to.
- CONFIGURED resting state, four rows, all unchanged: 'Dein Profil: Deutan (60%)' + [Sehtest wiederholen] (:1179-1193); the counted status line with all three branches including the one that spells out that 0 of 9 is a success state, + [Aus] or [Einschalten] (:1196-1258); the nine-tag 'Original -> Kontrastfarbe' proof row (:1260-1304); the Ctrl+Shift+V hold-to-compare hint (:1306-1324).
- Both re-entry paths survive intact - [Sehtest wiederholen] at :1188 and [Einschalten] at :1235 - each was added after a one-way door was found. ActivateCommanderTagProfile's call sites are :990, :1239 and :1434; re-derive with grep -n "ActivateCommanderTagProfile" before touching any of them.
- Rule 13: s_setupStep / s_setupDismissed / s_setupStrength (:42-47) still have no reset in AddonUnload. Fix that in this pass and add no further file-scope statics.

### 4. Einrichtung / Setup  *(zugeklappt)*

*Wo das CBA-Symbol sitzt. Einmal einstellen, dann nie wieder anfassen.*

- Replaces the 'Erweitert' TreeNode (:1351). Renamed because 'Erweitert' sitting two sections below an 'Erweiterter Modus' checkbox reads as the same concept and is not - P2's catch, and the only pure comprehension find nobody else made.
- Contents, and nothing else, ever: 'CBA-Symbol in der Schnellzugriffsleiste zeigen' (:1515), 'Eigenes Symbol erzwingen' (:1521), X-Position slider + Reset (:1533-1548).
- These three STAY on the panel rather than moving to the Studio. Grep proves they have no editor anywhere else in the tree, and one of them is self-hiding: a user who switches the quick-access icon off would otherwise need to find Advanced Mode to switch it back on. This is the relocation that cost P3 a point from two judges.
- While the block is open anyway: the Reset's hard-coded 405.0f must stop being its own definition. It is FOUR homes, not the two every proposal claimed - Settings.h:96 (the default), Settings.cpp:135 (the ini fallback), ModuleMain.cpp:632 and MainWindow.cpp:1545. Route the two reset sites at the one owner in Settings.h.
- Move the PopStyleVar(2) from :1341 to the END of the function. Today it fires BEFORE the fold renders, so everything inside Advanced silently draws with different frame rounding and item spacing than the panel above it.
- Keep TreeNodeEx and its own ImGui per-window storage - no new file-scope static, so this adds no rule-13 debt. Note that TreeNode open state is not persisted to imgui.ini, so this reopens collapsed every launch; that is correct for setup-once content and the reason nothing that must be remembered may live behind a fold.

### 5. Fusszeile / Footer

*(keine Unterzeile - es ist eine Signatur, kein Abschnitt)*

- One line: cyan 'Color Balance Assist' + disabled '(cba4gw2)' on the same row.
- OUT: the separator above it and the tagline 'Farb- und Kontrasthilfe fuer Guild Wars 2 - nicht nur bei Farbsehschwaeche'.
- Checked rather than assumed (P3 did this and it is the behaviour rule 22 asks for): Nexus renders our OptionsRender callback inside a CollapsingHeader it labels itself, under the addon's own name entry - C:/Users/Emi/Desktop/Nexus/src/UI/Widgets/MainWindow/Addons/Addons.cpp around :1304-1307. So the footer was re-titling an already-titled box at the foot of a scrolling column.
- The tagline's job is positioning for someone deciding whether to install. It moves to the Nexus addon description and the README, where it reaches that person.

---

## Was das Panel verlaesst

**DrawContrastTestSwatches call at MainWindow.cpp:1509 - DELETE THE CALL ONLY. The helper (:49-201), its five colour pairs and Settings.ContrastPairIndex stay alive for the Studio call at :2224; removing the function would silently break an out-of-scope window.**

- Wohin: Stays in the Studio (Main Window :2224). Nothing is relocated - the call site is simply gone from this panel, as Emi asked.
- Risiko: Honest accounting first, because all four proposals billed this as roughly 205px reclaimed and that is wrong: the call sits INSIDE the collapsed Advanced fold opened at :1351, so the default-state height saving is ZERO. The win is ~166 source lines and one fewer editor. Do NOT write 'ContrastPairIndex now has exactly one editing home' anywhere - every proposal claimed that and it is false: FilterLab.cpp:41-59 carries a third hand-copied kPairs table with its own combo on the same ##contrast_pair_combo id, so TWO editors remain. The real risk of this removal is the one Emi accepted by asking for it: the panel loses its only side-by-side before/after surface, so a user running Eye Comfort alone has no comparison affordance here at all (the Ctrl+Shift+V hint only renders while the enhancer is on). That is a real hole in the measure/correct/verify loop.

**Render backend radios (:1359-1394).**

- Wohin: Studio, into the diagnostics block that already prints the backend state at MainWindow.cpp:3045. The readiness/error READING does not go with them - it becomes the conditional gold line under the status field in Steuerung.
- Risiko: Unique as an editor, so if the Studio destination is not written in the same commit the setting becomes unreachable for everyone, not just non-Advanced users. Mitigated by CLAUDE.md recording DWM as kept only for comparison until retirement - a developer A/B switch is not player-facing. If DWM outlives its planned retirement this decision has to be revisited together with the SystemWide removal below; they are one decision, not two.

**'Im Hintergrund aktiv lassen' (SystemWide) checkbox + its two consequence lines (:523-562).**

- Wohin: Main Window :2833 already has the same checkbox on the same t.KeepActiveBackground label.
- Risiko: On RenderBackend==0 this is a real, functioning control and the panel was its only ungated home, so a DWM user who never ticks Advanced Mode loses it. Defensible only because the shader backend is the default and the backend switch leaves at the same time (a DWM user is by definition already in Advanced Mode). Emi did not ask to lose this - flagged in openQuestionsForEmi. Separately and more urgently: the two consequence lines at :549-560 state an effect that does not occur on the default backend (ShouldShaderPassRun() at ModuleMain.cpp:837 never consults SystemWide), which is a live rule-15 violation, and the Studio copy repeats the same unqualified claim around :2841. Deleting the panel copy moves the problem rather than ending it - the Studio wording needs qualifying as a separate follow-up.

**Profile-code field + [Generate] + [Import] (:1447-1496).**

- Wohin: Studio, which already carries the capability twice (:1843/:1852 and :2585/:2601).
- Risiko: Low - nothing becomes unreachable for an Advanced user, and a non-Advanced user has no realistic use for sharing a profile code. But Emi did not ask to lose it, so it is flagged. Recorded for later and deliberately out of scope: those two Studio copies are themselves a rule-10 duplication, and the panel's Import path asks ImportPresetString for an error string and then discards it.

**Active-module list '(Commander Tag, Eye-Sensitive, ...)' inside the status field (:444-462).**

- Wohin: Nowhere - deleted, and replaced rather than redirected: after this pass each rubric reports its own state in its own section (the Eye Comfort checkbox, the tag status line). Do NOT justify it by pointing at SensorGraphHUD.cpp:1143.
- Risiko: The supposed second home is behind the Advanced gate, so for this panel's audience it is not one. If the per-rubric state readouts turn out not to cover a module the list did cover (Hybrid Mode, Filter Lab), that module loses its only visibility for a non-Advanced user - check the FeatureModuleRegistry contents before deleting: grep -rn "FeatureModuleRegistry::Get().Register" plugins/nexus/src

**Brightness sub-heading + explainer (:834-838), the 'Empfehlung: N.NNx' half of the readout (:840-846), and the [Optimalwert (x.xx)] button (:848-868).**

- Wohin: The recommendation value survives in the Auto checkbox's tooltip, which is the control it describes. The full readout and the explicit Apply-Target button keep their homes in the Studio (:2707, :2716) and the Sensor Graph HUD (:997, :1038).
- Risiko: This is the one removal that touches the 'supply proof' pillar rather than the 'remove a judgment' one, which is why the 'Erhalt: NN%' half STAYS as a visible row against all four proposals. The Optimalwert button is redundant with the Auto checkbox two pixels away - both call the same shared ApplyAutoBrightnessGain - so its deletion is safe. If Emi finds the recommendation load-bearing at a glance, putting it back on the same row costs zero extra rows.

**The panel's hand-rolled save-into-first-empty-slot body (:636-654) and its chip-load body (:683-690).**

- Wohin: Not deleted as a feature - both rows stay on the panel and become UNGATED. The bodies are replaced by calls to new shared SaveSettingsToSlot(int) / LoadSettingsFromSlot(int), which the Studio also calls.
- Risiko: If the extraction is skipped and the existing copy is simply ungated, rule 10 is strictly worse after this pass than before - a hand-rolled duplicate becomes the one every user touches. That is the precise failure that sank P4. The extraction is not theoretical: slot save exists at :638, :2020 and :2320, and the :2020 copy writes Mixed = false and never writes MixedRg01 or MixedBy01, so a mixed-profile user who presses that button silently loses their mixed severities. Re-derive with: grep -n "Slots\[targetSlot\]" plugins/nexus/src/ui/MainWindow.cpp

**The two extra pitch lines under the Commander Tag heading (:951-956), and the hand-written subtitle at :947.**

- Wohin: The good sentence at :947 becomes PanelSection's aSubtitle argument. The other two are deleted outright - one repeats it, the other advertises Eye Comfort under the acquisition feature's heading.
- Risiko: Negligible. Each promise now lives as its own section's subtitle, which is where PRODUCT_CONCEPT's one-line rule wants it.

**Doubled separator block: :709-711 emits Spacing/Separator/Spacing and :728-730 emits it again with only comments between.**

- Wohin: One survives.
- Risiko: None. Two horizontal rules back to back every frame is visible proof of accretion.

**Four dead comment blocks that render nothing, roughly 50 source lines: the stale orientation blurb (:339-352, its text was already removed and only an ImGui::Spacing() survives), the removed-fullscreen-banner rationale (:354-363), the two 'moved away' notes (:1326-1339), and the SystemWide relocation note (:1550-1556).**

- Wohin: Git history - the reasons are in commit f4aacf0 and its neighbours.
- Risiko: Low, and rule 21 supports it: none of these are re-derivable knowledge and none of them describe a line that still exists. Every comment that records which bug caused which surviving line STAYS - this is not a comment cull, it is the removal of comments describing absences.

**Branding footer (:1561-1576) shrunk from three rendered rows plus a rule to one line; the tagline deleted.**

- Wohin: Tagline to the Nexus addon description and the README. Studio keeps its credits at :3103.
- Risiko: None worth naming - Nexus already titles the box (Addons.cpp:1304-1307).

**The Advanced Mode tooltip's sentence at :580-581, 'Without Advanced Mode, this compact panel is the entire interface.'**

- Wohin: Reworded, not deleted. Something like: 'Ohne Advanced Mode bedienst du den Filter komplett hier - Korrektur, Augenschonung, Tag-Kontrast. Advanced Mode fuegt Studio, Sensor-Graph, Labor und Vision Lab fuer Feinarbeit und Messung hinzu.'
- Risiko: Leaving the old wording is itself a rule-15 violation once the backend switch and the profile code leave the panel - the panel would be stating something about itself that is no longer checked. Two of three judges flagged this independently.

---

## Fragen an Emi (blockierend)

1. Vier Dinge verlassen das Panel, die du nicht zum Loeschen benannt hast. Sag bei jedem Ja oder Nein: (a) 'Im Hintergrund aktiv lassen' - auf dem Standard-Shader-Backend wirkungslos, aber auf DWM echt; bleibt im Studio. (b) Die Backend-Radios (Shader/DWM) - Entwickler-Umschalter, wandert ins Studio. (c) Profil-Code mit Generate/Import - existiert im Studio schon zweimal. (d) Die Modul-Liste neben dem Status-Punkt. Alles andere in der Entfernungsliste ist entweder dein ausdruecklicher Wunsch oder ein Duplikat.

2. Die Kontrast-Swatches raus bringt NULL Hoehe im Normalzustand - der Aufruf steht bei :1509 INNERHALB des zugeklappten Erweitert-Ordners, also sieht man ihn heute ohnehin nur nach einem Klick. Der Gewinn sind ~166 Quellzeilen und ein Editor weniger. Willst du sie trotzdem raus (ich nehme an ja), oder war die Annahme 'die fressen Platz' der eigentliche Grund?

3. Profile und Autostart werden ungatet - das kostet einen frischen Nutzer ohne gespeichertes Profil zwei Zeilen, die er noch nicht braucht. Ich zahle das, weil sonst der Autostart-Text 'erst ein Profil speichern' auf einen Speichern-Knopf zeigt, den er nicht erreichen kann. Einverstanden, oder soll der Profil-Block hinter Advanced bleiben und der Autostart-Satz stattdessen ganz verschwinden?

4. Der Sehtest bleibt wie er ist und oeffnet sich weiterhin von selbst - ich kompaktiere ihn NICHT. Er ist der einzige ungatete Editor fuer Typ und Staerke im ganzen Produkt und schrumpft sowieso auf zwei Zeilen, sobald ein Profil existiert. Zwei Vorschlaege wollten ihn zu einem Knopf machen; das ist eine Produktentscheidung ueber die ersten 60 Sekunden, nicht ueber Layout, und die gehoert dir. Soll er ein Knopf werden?

5. Der Helligkeits-Block behaelt die gemessene Zeile 'Erhalt: NN%' sichtbar; nur der Empfehlungswert wandert in den Tooltip der Auto-Checkbox. Alle vier Entwuerfe wollten auch den Prozentwert in einen Tooltip stecken, alle drei Pruefer haben genau das als schwaechste Stelle markiert. Reicht dir der Prozentwert, oder soll auch die Empfehlung sichtbar bleiben (kostet keine zusaetzliche Zeile)?

6. Ausserhalb des Auftrags, aber es ist ein echter Fehler: die zwei Erklaerzeilen unter 'Im Hintergrund aktiv lassen' (:549-560) behaupten eine Wirkung, die es auf dem Standard-Backend nicht gibt - ShouldShaderPassRun() bei ModuleMain.cpp:837 fragt SystemWide nie ab. Die Studio-Kopie um :2841 sagt dasselbe unqualifiziert. Das Verschieben loest es nicht, es verschiebt die falsche Aussage nur. Eigener Commit dafuer?

7. Ebenfalls ausserhalb des Auftrags, aber es korrigiert eine Behauptung, die sonst als 'erledigt' notiert wird: nach dem Entfernen des Swatch-Aufrufs hat ContrastPairIndex NICHT einen Editor, sondern zwei - FilterLab.cpp:41-59 hat eine dritte handkopierte kPairs-Tabelle mit derselben ##contrast_pair_combo-ID. Soll ich das aufnehmen oder als offener Punkt notieren?

---

## Umsetzungsreihenfolge

PRECONDITION COMMIT A - shared slot I/O. Extract SaveSettingsToSlot(int) and LoadSettingsFromSlot(int) into one pair (ui/UIState or ModuleMain, whichever already owns Slots) and repoint all three existing sites: MainWindow.cpp:636-655, :683-690, :2020-2040, :2320-2340, :2389-2395, :2458-2465. SaveSettingsToSlot must set ActiveSlotIdx through ParameterRegistry so the chip lights up - that is the save's only visible confirmation. Fixing the :2020 drift (Mixed = false, MixedRg01/MixedBy01 never written) is the point of this commit, not a side effect. No UI change ships here. Add a unit test in plugins/nexus/tests/ that round-trips a mixed profile through save+load and asserts MixedRg01/MixedBy01 survive, then BREAK it against the old :2020 body and watch it fail before believing it (rule 19).

PRECONDITION COMMIT B - shared SetAutoBrightness(bool) in ModuleMain.cpp beside SetGammaGainManual (:600), owning the ApplyAutoBrightnessGain side effect. Repoint MainWindow.cpp:871, MainWindow.cpp:2724 and SensorGraphHUD.cpp:1060. Check SensorGraphHUD.cpp:705 - it sets the flag directly inside another control and probably wants the setter too. No UI change ships here either. Add the assertion to core/SelfTest.cpp, not to the audit script: this is live in-process state, which rule 20 puts in SelfTest.

COMMIT C - pure deletions, no layout change. Remove the swatch CALL at :1509 (helper untouched), the module list (:444-462), the doubled separator, the four dead comment blocks, the SystemWide block (:523-562), the two pitch lines (:951-956), the brightness sub-heading/explainer/recommendation/Optimalwert rows, and shrink the footer. Move PopStyleVar(2) from :1341 to the end of the function. This is the commit that answers 'bau mal alles weg was zuviel wurde' on its own, and it is revertible in one step if Emi wants any of it back.

COMMIT D - relocations, each landing WITH its destination in the same commit so nothing is ever unreachable in between: backend radios to the Studio diagnostics block near :3045, profile code + Generate + Import to the Studio. Add the conditional shader-not-ready gold line to Steuerung in this same commit, because it is the reading the backend block was carrying.

COMMIT E - structure. PanelSection gets used for all four rubrics including Steuerung; the Commander Tag subtitle moves from the hand-written :947 into the aSubtitle argument; 'Erweitert' becomes 'Einrichtung' holding only the toolbar trio; DrawReadoutField() is extracted from the status field. Reword the Advanced Mode tooltip here.

COMMIT F - the ungating, and only now, on top of Commit A: Save + chips + autostart become one-row-each and leave the AdvancedModeUnlocked gate. Extend DrawAutoStartControl's aCompact branch to inline the radios and move the crash sentence into the tooltip; do not touch the full branch used by Main Window :2502. Test the no-slot state explicitly - the 'erst ein Profil speichern' sentence must still be visible TEXT, not a tooltip.

COMMIT G - Eye Comfort compaction. Delete the three TextDisabled label rows and pass the labels to SliderFloat directly with PushItemWidth reserving the Reset button. Copy the *100.0f / /100.0f boundary conversion character for character from :788/:791, three explicit calls, no loop, no table. Verify by reading the rendered percentages at 0, 50 and 100 - rule 8's failure mode is a slider that only ever shows 0% and 1%.

COMMIT H - Commander Tag entry. Re-condition the hint at :958-968 to (unconfigured AND s_setupDismissed) and make it a [Sehtest starten] button; move the three type buttons into step 1 as the 'Ich kenne meinen Typ' branch with a Back button. Add s_setupStep / s_setupDismissed / s_setupStrength to the AddonUnload reset (rule 13) in this commit.

COMMIT I - the 405.0f consolidation: four homes today (Settings.h:96, Settings.cpp:135, ModuleMain.cpp:632, MainWindow.cpp:1545), route the two reset sites at the Settings.h default.

VERIFY, and claim nothing before it: cmake --build build --config Release from plugins/nexus (report the build number and the test count actually seen), python tools/audit_pro_review.py green (report the check count actually seen), then open the panel in Nexus at Emi's real column width and LOOK at it. Every height figure in all four proposals and in this synthesis is arithmetic from source literals - 26px master button, 46px tiles at :1016, 56px preview at :1144, ItemSpacing (8,6) at :337, 13px text - and none of it has been seen on a screen. Do not repeat any of it as measured until it has been.

---

## Groesste Brocken im heutigen Panel

- Guided-entry wizard (lines 971-1193, ~223 source lines, 3 wizard steps + 2 lambdas): the single largest block on the panel. It is the acquisition feature and must stay, but it currently renders question text + three 46px tiles + a skip button + Back buttons + a 56px preview + a strength readout + two 30px buttons, all inline, above everything else in the Commander Tag rubric.

- Eye Comfort brightness sub-block (lines 818-925, ~108 lines): heading + explainer + retention readout + Apply Target button + Automatic checkbox + Manual slider + Reset. Every one of these exists a second time in Main Window Section 2 (2680-2732) AND a third time in SensorGraphHUD (1005-1108). Three homes for one story; two of them are direct CurrentSettings.AutoBrightness writes, not registry-backed.

- DrawContrastTestSwatches (call at 1501-1510, helper 49-201, ~166 lines): a 5-entry combo plus two 162px-tall cards. Emi explicitly wants it out of this panel. It is also called from RenderMainWindow (2224), and FilterLab.cpp:30-80 carries a THIRD hand-copied kPairs table + identical ##contrast_pair_combo.

- Toolbar-icon cluster inside Advanced (1515-1550, ~36 lines): Show quick-access icon, Force custom toolbar icon, X-Position slider 0-2500, Reset. Three nested levels of setup-once preference that no player touches twice.

- Render backend radios (1359-1412, ~54 lines): DWM vs Shader plus a shader-readiness status line. CLAUDE.md already records the DWM path as slated for retirement; a vestigial A/B switch is occupying panel space.

- Profile-code Export/Import (1447-1496, ~50 lines): InputText + Generate + Import + three tooltips. There are three separate export/import implementations in the tree (MainWindow.cpp 1468/1480, 1844/1856, 2587/2607) - a rule-10 violation.

- Heading style drift: PanelSection() was written to be the one look for every block (305-324), but the panel uses four different heading idioms - PanelSection with subtitle (731), PanelSection with nullptr subtitle plus a hand-written TextDisabled doing the subtitle's job (936/947), a bare TextDisabled heading (834 Brightness, 632 Profiles), and TreeNodeEx (1351).

- Double separator: lines 709-711 emit Spacing/Separator/Spacing and lines 728-730 emit Spacing/Separator/Spacing again with only comments in between - two horizontal rules stacked with nothing between them, every frame.

- Three redundant marketing lines in the Commander Tag rubric (942-956): a TextDisabled subtitle plus 'Commander-Tags im Zerg auseinanderhalten.' plus 'Augen schonen bei langen Sessions.' - the last of which describes Eye Comfort, a different rubric entirely.

- DrawAutoStartControl is called unconditionally at 707, but the Save button that creates the profile it needs is gated behind AdvancedModeUnlocked at 630. On a fresh install without Advanced Mode the panel prints 'Start automatically with GW2: save a profile first.' with no reachable way to save one - a dead end, and exactly the failure the no-BeginDisabled rule exists to prevent.

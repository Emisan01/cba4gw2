# Profil-Slots: kanonischer Filterzustand

Ergebnis eines adversarischen Workflows (2026-09-12): drei unabhaengige
Klassifikationen jedes Settings-Feldes, dann ein Angreifer fuer Auslassungen
und einer fuer Uebertreibungen, dann eine Entscheidung, die mehrfach GEGEN
die Mehrheit der Durchgaenge entschieden hat.

> Emis Vorgabe: "alle Filterzustaende wie sie eben gerade sind, reiner
> IST-Zustand, unabhaengig von aller anderer Logik" - alles ausser
> Fensterpositionen.

---

## Slot-Felder

**Used** (`bool`) — Schluessel `Slot<N>_Used`  
Clamp: none (value == "1"); but forced false by the post-parse validation pass if Slot<N>_Type failed to parse strictly

**Name** (`std::string`) — Schluessel `Slot<N>_Name`  
Clamp: strip CR, LF and all chars < 0x20 on WRITE (Save does none today); on READ truncate to 63 bytes and drop control chars. 63 because the rename control's buffer is char[64] at MainWindow.cpp:2454. The only untrusted free-text field in the slot, by design.

**Type** (`BalanceType`) — Schluessel `Slot<N>_Type`  
Clamp: STRICT parse of Protan/Deutan/Tritan. Unlike the top-level ParseType (Settings.cpp:29-34, silently defaults to Deutan), an unrecognised value invalidates the whole slot: reset it to a default-constructed ProfileSlot with Used=false. Restoring a correction for the wrong deficiency from a corrupt file is exactly the confident-wrong-readout rule 15 forbids.

**Severity01** (`float (was double)`) — Schluessel `Slot<N>_Sev`  
Clamp: [0.0, 1.25] from ParamMeta of ParamId::Severity01. Currently UNCLAMPED (Settings.cpp:200, bare safeStod) - SelfTest.cpp:294-303 only reports the damage after AutoStart has applied it.

**Mixed** (`bool`) — Schluessel `Slot<N>_Mixed`  
Clamp: none (value == "1")

**MixedRgSeverity01** (`float (was double, renamed from MixedRg01)`) — Schluessel `Slot<N>_MixedRg`  
Clamp: [0.0, 1.25] from ParamMeta of ParamId::MixedRgSeverity01. Currently unclamped (Settings.cpp:202).

**MixedBySeverity01** (`float (was double, renamed from MixedBy01)`) — Schluessel `Slot<N>_MixedBy`  
Clamp: [0.0, 1.25] from ParamMeta of ParamId::MixedBySeverity01. Currently unclamped (Settings.cpp:203).

**GammaGain** (`float`) — Schluessel `Slot<N>_Gamma`  
Clamp: [0.70, 1.30] from ParamMeta of ParamId::GammaGain. Already clamped today (Settings.cpp:204) but from retyped literals - move to the shared bound.

**EyeComfortModeEnabled** (`bool`) — Schluessel `Slot<N>_EyeComfort`  
Clamp: none (value == "1")

**BlueFilter01** (`float`) — Schluessel `Slot<N>_BlueFilter`  
Clamp: [0.0, 1.0] from ParamMeta of ParamId::BlueFilter01

**WarmTint01** (`float`) — Schluessel `Slot<N>_WarmTint`  
Clamp: [0.0, 1.0] from ParamMeta of ParamId::WarmTint01

**SaturationReduction01** (`float`) — Schluessel `Slot<N>_SatRed`  
Clamp: [0.0, 1.0] from ParamMeta of ParamId::SaturationReduction01

**CommanderTagMode** (`int`) — Schluessel `Slot<N>_CmdrMode`  
Clamp: [0, 1] from ParamMeta of ParamId::CommanderTagMode. Note the top-level key is unclamped today (Settings.cpp:120, bare safeStoi) while the registry declares [0,1] - fix both in the same pass or there are two ranges for one value again.

**EnhancerTolerance** (`float`) — Schluessel `Slot<N>_EnhancerTol`  
Clamp: [0.04, 0.20] from ParamMeta of ParamId::EnhancerTolerance

**EnableHybridMode** (`bool`) — Schluessel `Slot<N>_Hybrid`  
Clamp: none (value == "1"). ApplySlot must follow the field write with GetHybridScanner().SetEnabled(...) - see loadSemantics; the field alone never reaches HybridScanner::mEnabled.


---

## Bewusst draussen

**Enabled**  
Settings.cpp:250 forces it false unconditionally every launch and audit check 6.4 (tools/audit_pro_review.py, the five-assignment block anchored on ShowMainWindow) pins that. Arming already has exactly one authority - AutoStartSlot's apply path sets Enabled = true itself at ModuleMain.cpp:1607. A second one is CLAUDE.md rule 10, the same defect shape as the three drifted writers this task exists to fix. Rule to state in the UI: a slot stores what the filter looks like, never whether it is running.

**AutoBrightness**  
OVERRULES passes 1, 2, 3 and attacker 1, who all include it. SyncAutoBrightnessGain (ModuleMain.cpp:614-624) returns early only on !AutoBrightness - it never reads AutoBrightnessSource - and overwrites GammaGain from GetBrightnessRetention().recommendedGain, driven from MainWindow.cpp:830, MainWindow.cpp:2722 and SensorGraphHUD.cpp:998. So a slot that stores both a gain and the flag=true advertises a gain it never applies; the panel shows 1.12 for one frame and then snaps. Ordering does not save it, it only picks which value dies first. Resolution: store the gain, and have ApplySlot set AutoBrightness=false, which is exactly what the existing shared setter SetGammaGainManual already does (ModuleMain.cpp:602-604) and matches the house rule that a manual act beats the automatic. The picture the user saw is the gain, not the automation that produced it.

**AutoBrightnessSource**  
OVERRULES passes 1 and 2. Re-derived: grep -rn ApplySensorBrightnessCorrection plugins/nexus/src gives one definition (ModuleMain.cpp:876), one declaration (UIState.h:165) and one call site (ModuleMain.cpp:1304), guarded by sensorSteersBrightness = AutoBrightness && AutoBrightnessSource == 1. Source 2 is therefore unreachable dead code even though the function body accepts it (ModuleMain.cpp:880, 916-920). Source 1 is a no-op unless RenderBackend == 1 (ModuleMain.cpp:878), and RenderBackend is correctly not in the slot - so restoring this can only be right by accident, on a machine whose backend happens to match. It also silently allocates the FilterSensor's two 4K mip chains (ModuleMain.cpp:1298-1301) on a profile load.

**SensorBrightnessTarget**  
OVERRULES passes 1 and 2. Its only consumer is the source==2 branch that is never called (see above). Beyond being dead, it is a luminance measured from one frame on one display in one zone, not a description of the filter - a profile saved in a bright map would chase a stale level in a dark instance until GammaGain pins at its 1.30 clamp. Both attackers converge here and the code agrees.

**LabFilters (the vector and every sub-field: Enabled, Name, TargetRgb, ReplaceRgb, ToleranceTones, Diffusion, ActionType, LayerPriority)**  
OVERRULES passes 1 and 2; sides with pass 3 and attacker 2. Four independent reasons, any one sufficient. (1) CLAUDE.md marks the Lab data model for rebuild as an orderable layer stack - persisting today's shape three times over IS the migration this task exists to prevent. (2) It cannot express 'no Lab layers': two different seeders refill an empty vector with different content (Settings.cpp:209-232 seeds two filters, FilterLab.cpp:216-227 seeds a different single one), and SelfTest pins non-emptiness because FilterLab.cpp:231 clamps to (0, count-1), UB on an empty vector. A slot meaning 'Lab: nothing' would come back with live targets. (3) Destructive: a Lab stack is a document with no undo anywhere in this codebase; a profile chip click would replace hand-built layers. (4) Cost: kMaxLabFilters is 64 (Settings.cpp:161) times 12 keys times 3 slots = up to 2304 lines rewritten by Save on every saveNeeded UI frame through a linear if/else parser. If Emi wants Lab stacks saved they need their own named store, not three unnamed copies inside colour profiles.

**LabModeEnabled**  
OVERRULES passes 1 and 2. With LabFilters excluded, the switch is a pointer with no referent - it turns Lab on over whatever happens to be in the stack right now, which may be a half-finished experiment the user kept safe by leaving Lab mode off. Include the switch and the stack together or neither; neither is the v1 answer.

**CommanderTagLayerPriority**  
OVERRULES all three passes, which all include it. It is not an independent scalar: MoveFilterLayer resolves a layer id to an int& that is EITHER CurrentSettings.CommanderTagLayerPriority OR CurrentSettings.LabFilters[id].LayerPriority and std::swaps the two (FilterLayers.cpp:46-51). The rendered order is one permutation split across two storages, and GetFilterLayerOrder's std::stable_sort (FilterLayers.cpp:25-26) breaks ties by insertion order, so vector position is part of it too. Storing one half is not partial information, it is misinformation - restoring it can reorder layers the slot never saw. It is also inert in the common case: the value only reaches a pixel as layerRankOf(-1) (ModuleMain.cpp:469), the Com-Tag row's INDEX in the sorted list, which is 0 whatever the stored value when Com-Tag is the only contributing layer. The slot carries no layers, therefore it carries no layer ordering.

**DiagnosisHint**  
OVERRULES passes 2 and 3. Re-derived: outside Settings.cpp its only reader in the whole source is an ImGui text box at SensorGraphHUD.cpp:968-979 - zero readers in the matrix path (FilterLayers.cpp) and zero in the target path (ModuleMain.cpp:280-500). Emi asked for Filterzustaende - state, not captions. It is also the field that makes a slot personal rather than technical (an AQ/HRR label about the user), and it would double the untrusted free-text surface in a format with no escaping. Keep exactly one string per slot.

**ShowMainWindow / ShowGraphWindow / ShowLabWindow / ShowVisionLabWindow**  
Forced false at Settings.cpp:235-238 and pinned by audit check 6.4; window chrome, never the picture.

**RenderBackend**  
Picks which painter applies an identical matrix (ShouldScreenEffectBeActive vs ShouldShaderPassRun) - a per-machine capability and consent decision, and the live A/B comparison Emi is running. A slot that flipped it could move the correction onto the whole desktop via DWM.

**SystemWide**  
Decides whether the DWM correction survives Alt-Tab (ModuleMain.cpp:830) - scope of effect, not filter state. This is the one exclusion where restoring it could tint a browser the user is reading. Say so in the UI rather than leaving it to be discovered.

**SelectedLabFilterIndex**  
An editor cursor into a vector the slot does not carry. Note it is also unclamped on top-level read today (Settings.cpp:144) and FilterLab.cpp:231 clamps to (0, count-1) - fix in the same pass.

**ContrastPairIndex**  
Re-derived before agreeing: it only selects which of kPairs[5] the preview swatches draw (MainWindow.cpp:75-85, FilterLab.cpp:49). Zero game pixels.

**AutoStartSlot**  
Bank metadata pointing INTO the bank. A slot that restored it would silently repoint which profile auto-starts next launch, and it is the single sanctioned exception to the neutral-start rule (ModuleMain.cpp:1596, vetoed by SafeModeTriggered).

**Slots[3]**  
A slot cannot contain the slot bank.

**CleanExit / SafeModeTriggered**  
Crash breadcrumbs derived from cba_session.lock at Settings.cpp:52-56. A slot that restored them would forge or clear a crash record and could bypass the Safe-Start gate.

**AdvancedModeUnlocked**  
Permission-shaped. A slot able to set it false hides the window the user is standing in; able to set it true bypasses the Core/Advanced gate entirely.

**AlwaysDirectStart**  
Safe-Start gate policy - a profile must never change how the next launch handles a crash.

**Language / UiTheme / UiOpacity / DebugMode / GraphMode / MainGraphMode / ShowQuickAccessIcon / MovableToolbarIcon / ToolbarIconPosX / ToolbarIconPosY**  
Application chrome and device state. Re-derived the two least obvious: grep -rn UiOpacity plugins/nexus/src shows only ImGui window alpha and CBA's own graph strokes - no overlay, no shader reader; GraphMode/MainGraphMode only choose a graph drawing style. ToolbarIconPos* is a screen coordinate that is wrong at another resolution; ResetUiLayout (ModuleMain.cpp:630) is the only thing that should write it.

**A per-slot schema version key (Slot<N>_Ver)**  
OVERRULES passes 2 and 3 and attacker 1, who all want one. The parser is key-based and additive: unknown keys fall through the if/else chain silently and missing keys keep struct defaults (Settings.cpp:191-206), which is the same forward/backward-compat property ExportPresetString deliberately relies on. With a flat scalar payload and no nested block, a version integer buys one branch nobody will write, and its only realistic use - refusing a newer slot - is worse for the user than restoring 14 of 15 fields. It earns its keep the moment a nested sub-block goes in, i.e. when the rebuilt Lab stack lands. Replace it with one hard rule instead: never reuse an existing Slot key name for a different meaning - append a new name.


---

## Ladesemantik

MUST NOT: Settings::Load must never write a single live CurrentSettings field from slot data. It parses Slot<N>_* keys into Slots[N] and nothing else. That is what keeps the neutral-start guarantee intact by construction rather than by argument: the slot payload is not a load-time surface at all, and the unconditional block at Settings.cpp:235-250 (four Show* flags plus Enabled) stays exactly as audit check 6.4 pins it, untouched by this change. Load must also never let a slot key influence any non-Slot key - which today it can, because Save writes strings raw (Settings.cpp:333) while Load splits on the first '=' and takes the rest of the line (Settings.cpp:87-88), so one CR/LF inside Slot0_Name becomes a live top-level key on the next load. Attacker 1 is right that the smuggled key cannot be Enabled=1 (s.Enabled = false at Settings.cpp:250 runs after the parse loop) and attacker 2's example is right about what IS reachable: SystemWide=1, AdvancedModeUnlocked=1, AutoStartSlot=0. Sanitise at the write boundary, bound on read.

MUST: clamp every numeric slot key at parse, from the single shared bound, never from a retyped literal. Implementation trap none of the three passes or either attacker caught - ParameterRegistry::GetMeta is NOT callable during Load: ModuleMain.cpp:1584-1585 runs CurrentSettings = Settings::Load(AddonDir); before RegisterAllParameters();, and GetMeta asserts on an unregistered id and otherwise returns an empty ParamMeta whose maxI == minI, i.e. no clamp at all. So the bounds must move into constexpr constants in a header that both RegisterAllParameters() and Settings::Load read (RegisterAllParameters builds its ParamMeta from them). That is one definition with two readers and no initialisation order dependency, and it also retires the retyped literals in ImportPresetString that the comment at Settings.cpp:447-455 records the cost of. Alternative if Emi prefers less churn: move RegisterAllParameters() above the Load call - the registry points at CurrentSettings members, which whole-struct assignment does not invalidate - but that leaves two homes for the numbers, so the constants are the better answer.

MUST: run a post-parse validation pass over Slots[]. A slot whose Type failed strict parsing is reset to a default-constructed ProfileSlot with Used=false. A Name longer than 63 bytes is truncated; control characters are dropped.

APPLY, which is where the arming question actually lives: applying a slot is a deliberate act with exactly two callers - a user clicking a slot chip, and the AutoStartSlot path at ModuleMain.cpp:1596-1615, which is already vetoed by SafeModeTriggered and which sets Enabled = true itself. ApplySlot writes only the 13 payload fields, then must do three things the field write alone does not: (1) call GetHybridScanner().SetEnabled(slot.EnableHybridMode), because CurrentSettings.EnableHybridMode never reaches HybridScanner::mEnabled by itself - the only three writers are ModuleMain.cpp:1628, MainWindow.cpp:2869 and FeatureModule.cpp:56, and ScanFrame's first line is if (!mEnabled || !aSwapChain) return; (HybridScanner.cpp:487). This is why I overrule attacker 2 and keep EnableHybridMode in the payload: with it off, no Commander Tag pixel and no Lab pixel is ever replaced, so a restored Com-Tag profile would leave the panel reporting "Com-Tag Contrast: active, N of 9 colours shifted" while nothing on screen changes - rule 15 exactly. Pass 3's claim that CommanderTagMode alone starts the scanner is false; ModuleMain.cpp:1324 only decides whether ScanFrame is CALLED, and the Watchdog self-heal at ModuleMain.cpp:1016 calls Initialize(), which never touches mEnabled. (2) set CurrentSettings.AutoBrightness = false before writing GammaGain, so the restored gain is not overwritten on the next panel frame. (3) call UpdateTagEnhancerConflicts() and Recompute(force) once, under s_recomputeMutex, the way ResetFilterSettingsAndDisable does.

Two real defects found while verifying, both of which ApplySlot would otherwise expose. First: HybridScanner::SetEnabled(false) clears mProblems but does NOT release mOverlaySRV (HybridScanner.cpp:122-129), while GetOverlaySRV keeps returning it (HybridScanner.cpp:131-137) and the gate at ModuleMain.cpp:1324 is satisfied by CommanderTagMode != 0 alone - so the combination EnableHybridMode=0 with CommanderTagMode=1 keeps drawing a frozen last overlay via AddImage forever. Attacker 1 flagged this and the code confirms it; SetEnabled(false) should release the SRV. Second: the profile panel's unsaved-changes indicator is six function-local statics at MainWindow.cpp:2274-2296 (s_baseType/s_baseSev/s_baseMixed/s_baseMixedRg/s_baseMixedBy/s_baseGamma) - a hand-written mirror of exactly today's eight-field payload, re-synced at three slot sites and NOT at the Com-Tag quick-save. Widen ProfileSlot without rebuilding that baseline from the same struct and the panel reports "saved" while Eye Comfort, Com-Tag, tolerance and the scanner flag all differ. Fix: give ProfileSlot an operator==, replace the six statics with one static Settings::ProfileSlot s_baseline, and make isDirty = !(CaptureSlot(CurrentSettings) == s_baseline). Then the dirty check cannot drift from the payload, because it is the payload. Being function-local statics they also escape CLAUDE.md rule 13 across a Nexus disable/enable.

One more Apply-time interaction to settle deliberately rather than discover: FilterLab.cpp:912-919 turns Commander Tag off whenever any Lab control is touched in a frame (touchedThisFrame includes saveNeeded, which a chip click sets). With Lab excluded from the slot this cannot corrupt a restored profile at apply time, but a restored Com-Tag profile still dies silently the first time the user clicks anything in Filter Lab. Out of scope for the field list, worth a separate look.


---

## Migration bestehender ini-Dateien

Nothing to migrate, and no version key needed - the format is additive by construction. Verified against the parser: Slot keys are matched by name in a linear if/else (Settings.cpp:191-206) inside a positional prefix test (key.rfind("Slot",0)==0 && key.size()>=8, idx = key[4]-'0', key[5]=='_', prop = key.substr(6)); unknown keys fall through silently and absent keys never execute an assignment, so the member keeps its struct default. All seven new prop names are two chars or longer, so they satisfy the size>=8 test.

An existing settings.ini carrying only the six old payload keys therefore loads exactly as it does today - Used, Name, Type, Sev, Mixed, MixedRg, MixedBy, Gamma all still parse from the same key names, none of which is renamed - and the seven new members come back at their ProfileSlot defaults. Choose those defaults so that an old slot reads as "the profile as it was, plus no extra layers": EyeComfortModeEnabled=false, BlueFilter01=WarmTint01=SaturationReduction01=0.0f, CommanderTagMode=0, EnhancerTolerance=0.12f (the Settings default), EnableHybridMode=false. The first Save after upgrading rewrites all fifteen keys per slot; there is no migration code, no rename, and an older build reading a newer ini ignores the seven keys it does not know.

Two consequences to state rather than let people find. First, a real behaviour change: applying an upgraded old slot now turns Commander Tag and the scanner OFF, where before it left whatever was live untouched - because the slot is now authoritative for its whole payload. That is the correct semantics for "reiner IST-Zustand" but it is a change, and it is question 4 for Emi. Second, the member type change from double to float on Severity01/MixedRg01/MixedBy01 (matching the Settings fields and the header comment at Settings.h:15-19 that argues against double for exactly these values) is invisible to the file: Save already wrote them at default ostream precision and the parser reads text either way. Two members are also renamed in C++ only - MixedRg01 to MixedRgSeverity01 and MixedBy01 to MixedBySeverity01 - so that every payload member name is identical to its Settings counterpart. The ini keys stay _MixedRg and _MixedBy. That one-to-one naming is what makes the mechanical audit check below possible.

The single hard rule going forward, in place of a version integer: never reuse an existing Slot key name for a different meaning. Append a new name instead.


---

## Testplan

- STRUCTURAL, and the one that actually catches the reported bug class. New audit check in tools/audit_pro_review.py: extract the member names of struct ProfileSlot from core/Settings.h, extract the left-hand sides assigned in CaptureSlot() and the right-hand sides read in ApplySlot() from the new core/ProfileSlots.cpp, and assert the three sets are identical. A field added to the struct but forgotten in ApplySlot fails CI. This is precisely what would have caught MainWindow.cpp:2054 (Mixed hardcoded false) and the never-written MixedRg/MixedBy. Break it per rule 19 by deleting one assignment line from ApplySlot and watching the audit go red before believing it.

- STRUCTURAL, single-writer enforcement. Audit check: assert no translation unit other than core/ProfileSlots.cpp contains a write to a Slots[...] member. Today there are seven hand-written copies - MainWindow.cpp:638-652, 683-688, 2036-2055, 2336-2354, 2403-2408, 2472-2477, 2498-2499 and ModuleMain.cpp:1601-1606 - and their drift IS the bug (CLAUDE.md rule 10). Break it by re-adding the Com-Tag quick-save's hand-written block.

- STRUCTURAL, forces a decision on every future Settings field. Audit check: extract every member of struct Settings from Settings.h and assert each name appears either in ProfileSlot's member list or in a named exclusion list that lives in one place in the source (a constexpr array of string literals next to CaptureSlot, not a comment). Adding a Settings field then fails CI until someone classifies it. Break it by adding a dummy bool to Settings.

- PURE ROUND-TRIP, in plugins/nexus/tests/ (needs CaptureSlot/ApplySlot to take an explicit Settings& rather than touching the CurrentSettings global, so they link without the addon). Build a Settings with all 13 payload fields set to distinct non-default values, CaptureSlot into slot 1, reset the live fields to defaults, ApplySlot, assert every payload field equals the source. Break by deleting one line from ApplySlot.

- PURE ISOLATION, same test file. Snapshot every NON-payload Settings field before ApplySlot and assert every one is bit-identical afterwards - especially Enabled, the four Show* flags, AdvancedModeUnlocked, SystemWide, RenderBackend, AutoStartSlot and LabFilters. This is the check that catches an ApplySlot which quietly writes something it should not. Break by adding CurrentSettings.SystemWide = true to ApplySlot.

- HOSTILE INI, the check that pins what this task was actually afraid of. Write a settings.ini containing: every slot numeric key far out of range (Slot0_Sev=99, Slot0_Gamma=-5, Slot0_EnhancerTol=7, Slot0_BlueFilter=12, Slot0_CmdrMode=9), a Slot1_Type=Banana, a Slot2_Name carrying an embedded CR/LF followed by SystemWide=1 and AdvancedModeUnlocked=1, plus AutoStartSlot=0. Load it, then assert: Enabled is false; all four Show* flags are false; every slot numeric sits inside its shared bound; slot 1 is Used=false (strict type parse rejected it); slot 2's Name carries no control characters; and SystemWide and AdvancedModeUnlocked are still false, i.e. nothing was injected. Break it by removing one clamp, and separately by removing the write-boundary sanitiser.

- MIGRATION, same file. Load an ini whose slots carry only the six old payload keys and assert Type/Sev/Mixed/MixedRg/MixedBy/Gamma round-trip unchanged while the seven new members read back at their documented defaults (EyeComfort off, three sliders 0, CmdrMode 0, EnhancerTol 0.12, Hybrid off). Break by changing one default in the struct.

- DIRTY-BASELINE COVERAGE, in tests once operator== exists on ProfileSlot. Assert that for a Settings differing from a baseline in ONLY a newly added payload field (say EnhancerTolerance), CaptureSlot(s) == baseline is false. With the six s_base* statics at MainWindow.cpp:2274-2296 replaced by one static ProfileSlot compared via operator==, the panel's unsaved-changes indicator cannot drift from the payload. Break it by reverting to the hand-written six-field comparison and watching a tolerance change report 'saved'.

- LIVE, in core/SelfTest.cpp (the place for in-process state per CLAUDE.md rule 20). Assert the scanner's actual enable state matches CurrentSettings.EnableHybridMode - i.e. that some path called SetEnabled after the last write - and extend the existing slot-range check at SelfTest.cpp:288-305 to cover all 13 payload fields instead of the current three severities. Break it by removing the SetEnabled call from ApplySlot; the round-trip test will still pass, which is exactly why this one has to exist separately.

- MANUAL, on a screen, because nothing above proves the thing the tool is for: save a Com-Tag + Eye Comfort profile with Hybrid on, restart GW2, load the slot from the chip, and confirm the tags are actually recoloured - not just that the panel says they are. This is the same untested-since-the-backend-swap gap CLAUDE.md already lists as open, and rule 18 says report the build number and what was seen, not a state that was assumed.


---

## Fragen an Emi

- Auto-Brightness: I took it OUT of the slot and have ApplyFromSlot switch it off, so a slot restores one fixed gain. The alternative - storing the flag - means the panel shows a gain the slot never applies, because SyncAutoBrightnessGain overwrites GammaGain on the next panel frame. 'Das Bild, das du gesehen hast' is the gain; 'Auto-Brightness war an' is a behaviour, not a state. Einverstanden, oder soll ein Slot die Automatik zurueckholen?

- Should the unconditional neutral-start block at Settings.cpp:235-250 grow to force CommanderTagMode=0 and EnableHybridMode=0 as well, with audit check 6.4 extended to match? This is independent of slots - today a settings.ini with Com-Tag and Hybrid on already paints tags at character select with no click, because ModuleMain.cpp:1324 has no Enabled term and FilterLayers.h:66-70 says so outright. Widening slots just adds a second door to the same room. Extending it is consistent ('every launch starts neutral') but costs existing users one re-tick.

- Filter Lab stays out of v1 slots - the stack is a document, not a preference, its current shape is explicitly pre-rebuild, and 'zero Lab filters' cannot even be expressed today because two different seeders refill an empty vector with different content. Confirm: wenn du Lab-Stacks speichern willst, bekommen sie einen eigenen Speicher, keine drei namenlosen Kopien in den Farbprofilen.

- Applying an upgraded old six-key slot will now turn Commander Tag and the scanner OFF, because the slot is authoritative for its whole payload. Before, loading a slot left whatever was live alone. Is that the behaviour you want for existing saved profiles, or should an old slot leave Com-Tag untouched until it has been re-saved once?

- Should the Com-Tag quick-save button (MainWindow.cpp:2020-2055) survive at all? Its real defect is not the missing Mixed writes, it is deriving a storage address from a filter value (Protan->0, Deutan->1, Tritan->2) - a worldview in which mixed profiles do not exist. Ich wuerde die Ableitung loeschen, nicht flicken: entweder der Button benutzt dieselbe Save-to-slot-Logik wie die anderen drei Stellen, oder er verschwindet.

- Should the bank cursor persist? s_activeSlotIdx (UIState.h, ParamId::ActiveSlotIdx) is never written to the ini, so after a restart the highlighted chip is slot 1 unless AutoStart fixes it - 'auf welchem Profil bin ich gerade' ueberlebt die Sitzung nicht. Das ist Bank-Metadaten, kein Slot-Feld; nur soll es nicht per Auslassung entschieden werden.

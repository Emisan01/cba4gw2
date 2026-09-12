# CBA — Roadmap

Stand 2026-09-12. Zusammenführung aller offenen Punkte mit Emis Ideensammlung.
Reihenfolge ist bewusst: jede Phase setzt voraus, dass die davor sitzt.

Kein Release, bis Phase 3 steht — Emis Vorgabe: *„der aktuelle Stand geht so
nicht online."*

---

## Eine Begriffsklärung vorweg

**„Hybrid" bedeutet gerade zwei verschiedene Dinge.** Das muss aufgelöst
werden, bevor darüber geplant wird:

| Bedeutung | Was es ist | Wo |
|---|---|---|
| **Hybrid-Modus (heute, im Code)** | Die Overlay-Schicht: `HybridScanner` liest den Frame per DXGI zurück und malt Ersatzfarb-Marker über getroffene Pixel. | `EnableHybridMode` → `HybridScanner::SetEnabled` |
| **Hybrid (Emis Idee, Punkt 5)** | DWM **und** Shader gemeinsam: DWM für screen-weit, Shader für das Spielbild. | existiert noch nicht |

Das sind drei Schichten, nicht zwei Modi. Vorschlag: die Overlay-Schicht
bekommt einen eigenen Namen (z. B. „Ziel-Overlay"), und „Hybrid" wird für die
Kombination der beiden Malwege frei. **Zu klären, bevor Phase 2 beginnt.**

---

## Phase 0 — Blocker

Nichts davon ist neue Arbeit, aber nichts darüber zählt, solange es offen ist.

- [ ] **Commander-Tags im Shader-Modus visuell prüfen.** Sehen sie aus wie
      unter DWM? Genau das schützt die PostRender-Platzierung, und es hat noch
      nie jemand auf einem Bildschirm gesehen. Kostet zwei Minuten im Spiel.

---

## Phase 1 — Sortieren *(hier fangen wir an)*

Emis Kern: **„wir brauchen die Zusammenführung der passenden Regler zu
entsprechenden Modulen."**

- [ ] **Regler den Modulen zuordnen.** Bestandsaufnahme über alle vier Fenster:
      welcher Regler gehört fachlich zu welchem Modul, wo sitzt er heute, und
      wo wäre sein einziges Zuhause. Details klärt Emi noch.
      → Voraussetzung ist erfüllt: registry-gebundene Regler lassen sich ohne
      Verhaltensänderung verschieben (Regel 9).
- [ ] **UI grundsätzlich überarbeiten.** Emi: gefällt so nicht. Screenshots
      kommen — **nicht vorher raten.**
- [ ] **Commander-Tag-Leiste mit echten Symbolen.** Statt abstrakter Farbfelder
      die tatsächlichen Commander-Symbole, alle 9 Farben, als Symbolleiste.
      → Offen: woher die Symbole kommen. Eigene Grafik (wie `gen_icon.py`) ist
      der sichere Weg; Spiel-Assets sind keiner.
- [ ] **Überlappende Kreise prüfen — Logik *und* Darstellung.** Nicht nur ob
      die Mathematik stimmt, sondern ob das Bild zeigt, was es behauptet.
      Betrifft die Kontrast-Test-Farbfelder und die Vorher/Nachher-Paare.

---

## Phase 2 — Hybrid retten und richtig bauen

Emi: **Priorität, nicht fallen lassen.**

- [ ] **Begriffe trennen** (siehe oben). Ohne das plant man an zwei Dingen
      gleichzeitig vorbei.
- [ ] **DWM-Pfad bleibt.** Die Zeile *„Retire the DWM backend"* in CLAUDE.md
      ist damit überholt und wird gestrichen — DWM ist nicht Altlast, sondern
      die zweite Hälfte.
- [ ] **DWM + Shader kombinierbar machen.** Heute schließen sie sich aus
      (`ShouldScreenEffectBeActive` gibt false zurück, sobald der Shader läuft;
      SelfTest prüft sogar, dass nur einer malt).
      → **Offene Designfrage:** Was genau soll die Kombination bewirken? Beide
      dieselbe Matrix anzuwenden würde sie *quadrieren*. Plausibel wäre: Shader
      trägt die volle Korrektur im Spielbild, DWM trägt eine schwächere Fassung
      für alles außerhalb. Das ist eine Entscheidung, keine Ableitung.
- [ ] **Overlay-Schicht (heutiger Hybrid-Modus) fertigstellen.** Steht seit
      Beginn als „(Beta)" da.

---

## Phase 3 — Freigabefähig machen

- [ ] **Selbsttest grün auf einem echten Spielstand**, nicht nur im Leerlauf.
- [ ] **Release-Tag.** Letzter ist `v1.11.0`; `main` ist weit darüber hinaus,
      der Branch noch weiter. Nichts davon erreicht Spieler.
- [ ] **README gegen den tatsächlichen Stand prüfen** — die
      Vollbild-Voraussetzung ist raus, aber der Rest ist seit dem Umbau
      ungeprüft.

---

## Phase 4 — Filter Lab, das Glanzstück

Emis Ziel, bewusst zuletzt: *„das Filter Lab regeln wir später, wenn der Rest
sitzt."* Ausführlich in CLAUDE.md unter *Where Filter Lab is going*.

- [ ] **Modularer Layer-Stack** zur Wellenlängen-Elimination, Reihenfolge
      bestimmbar, Roman-Teleskop als Vorbild: jede Schicht trägt ein *Gewicht
      bei*, statt dass jede die nächste verengt.
- [ ] **Blocker zuerst:** `HybridScanner::AnalyzeBuffer` gruppiert Treffer nach
      *exakter* Gleichheit der Ersatzfarbe. Eine echte gewichtete Mischung
      macht fast jeden Pixel minimal anders und zerlegt jeden Cluster.
      Gruppierung nach Ziel-ID oder Farbähnlichkeit muss davor kommen.
- [ ] **AoE-Preset**, das heute angelegt wird und nie erscheint — gehört in
      diese Phase, nicht davor.

---

## Läuft nebenher

Kein eigener Meilenstein, aber nicht vergessen:

- [ ] `cba_session.lock` / Safe-Start ist ein Workaround, kein Design.
- [ ] `EAddonFlags::DisableHotloading` für Release-Builds — Emis „jetzt nicht".
- [ ] **Regional Hybrid Mode** (4 Quadranten, je eigene Matrix) — Vorschlag,
      größer als er aussieht. Fällt womöglich mit Phase 2 zusammen.
- [ ] **Sensor-Messbereich.** Misst heute den ganzen Frame, CBA-Oberfläche und
      Sensorfenster eingeschlossen. Ein setzbares Rechteck wäre die ehrliche
      Verfeinerung, sobald die Zahlen präzise statt orientierend sein sollen.
- [ ] **Weitere Logiken gegen die Messung verifizieren.** Auto-Helligkeit ist
      durch. Nächster Kandidat: der Commander-Tag-Enhancer — er behauptet „N
      von 9 verschoben", und mit Vorher/Nachher-Bild ließe sich messen, ob die
      Trennung wirklich zugenommen hat.

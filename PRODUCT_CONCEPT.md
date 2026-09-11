# Produktkonzept: CBA für die GW2-Community

Dieses Dokument verbindet die UX-Ebene mit der vorhandenen Technik. Es ist
kein Feature-Wunschzettel, sondern die Begründung dafür, *warum* welche
Umbauten in welcher Reihenfolge sinnvoll sind.

Ergänzt `CLAUDE.md` (Projektstand, Architektur), `AGENTS.md` (Code- und
UI-Regeln) und `COLOR_MATH.md` (Farbmathematik). Stand: 2026-09-11.

---

## 1. Der Kerngedanke

> **CBA verlangt vom Nutzer, eine Farbkorrektur mit genau der Wahrnehmung zu
> beurteilen, die korrigiert werden soll.**

Das ist die zentrale Designfalle des bisherigen Aufbaus, und fast jede
UX-Schwäche lässt sich darauf zurückführen. Ein Normalsichtiger kann eine
Farbeinstellung bewerten. Die Zielgruppe kann es per Definition nicht.

Jede Frage, die CBA heute stellt, verlangt ein Urteil, für das dem Nutzer das
Instrument fehlt:

| Frage im aktuellen UI | Warum der Nutzer sie nicht beantworten kann |
|---|---|
| „Protan, Deutan oder Tritan?" | Eine Diagnose, die die meisten nie gestellt bekommen haben |
| „Stärke 60%?" | Kein Referenzpunkt — mehr Korrektur sieht nicht „richtiger" aus |
| „Mixed RG / BY?" | Setzt Kenntnis der eigenen Achsen voraus |
| „Sieht das jetzt besser aus?" | Genau die Frage, die die Sehschwäche unbeantwortbar macht |

**Daraus folgt die Leitregel für alle weiteren Entscheidungen:**

> Jede Designentscheidung muss entweder **ein Urteil entfernen**, das der
> Nutzer nicht fällen kann, oder **externen Beleg liefern**, dass die
> Korrektur wirkt.

Alles andere in diesem Dokument ist Anwendung dieser einen Regel.

### 1.1 Das war schon immer der rote Faden

Rückblickend zielten die meisten bisherigen UI-Wünsche bereits auf genau
dieses Problem, nur ohne es zu benennen: die Kontrast-Swatches und ihre
mehrfachen Redesigns („man sieht den Effekt gar nicht wirklich" → größer →
überlappend → Glas-Look), die 9-Tag-Reihe im Nexus-Panel, die Statuszeile
„N von 9 Farben verschoben", der Handshake-Punkt. Das sind alles Antworten auf
dieselbe Frage: **„Woher weiß ich, dass es wirkt?"**

Diese Frage ist nicht ein UI-Detail unter vielen. Sie ist die Hauptaufgabe.

---

## 2. Die drei benannten Aufgaben

Das Produkt sollte sich um Aufgaben organisieren, die ein Spieler tatsächlich
hat — nicht um die Maschinerie, die sie löst. Alle drei laufen auf derselben
bereits existierenden Engine.

### A. „Ich verliere den Commander im Zerg"
Die Einstiegsfunktion. Automatischer Kontrast für die 9 GW2-Tag-Farben,
selektiv nur auf die tatsächlich verwechselbaren.
**Technik:** `UpdateTagEnhancerConflicts()` + `HybridScanner` — vollständig
vorhanden, seit 2026-09-11 auch mathematisch korrekt (siehe `COLOR_MATH.md`
Abschnitt 8).

### B. „Ich stehe im roten Kreis, ohne ihn zu sehen"
Die meistgenannte GW2-Beschwerde bei Farbsehschwäche überhaupt: rote
Gegner-AoE gegen freundliche Felder. Vermutlich größer als Aufgabe A.
**Technik:** Dieselbe Target→Replace-Engine. Existiert bereits — aber nur als
Handarbeit: Emis eigener Filter „AoE Red to Signal Cyan", hinter Advanced Mode
im Labor. Die mitgelieferten Defaults sind stattdessen abstrakte Farbtausche
(„Gruen zu Signal-Rot", `Settings.cpp`).
**Lücke ist reine Verpackung, keine Technik.**

### C. „Ich sehe generell Farben schlecht"
Die bildschirmweite CVD-Korrektur. Wissenschaftlich die stärkste Komponente,
aber als *Standardeinstieg* die schlechteste UX (siehe 3.4).
**Technik:** `Recompute()` + DWM — vorhanden und abgesichert.

### D. „Meine Augen brennen nach drei Stunden GW2" — der Bindungsfaktor

Nachgetragen 2026-09-11 aufgrund des bisher wertvollsten Nutzersignals: Eye
Comfort ist die Funktion, die Emi nach eigener Aussage bereits *süchtig* nach
dem Tool macht — im Alltag, beim normalen Spielen. Das ist kein Nebeneffekt,
das ist eine strategische Information.

Es trennt zwei Rollen, die vorher vermischt waren:

| Rolle | Funktion | Warum |
|---|---|---|
| **Einstieg** (warum installiert jemand?) | Commander-Tag-Kontrast | Konkretes, benennbares GW2-Problem |
| **Bindung** (warum bleibt es an?) | Eye Comfort | Wirkt in *jeder* Session, nicht nur im Zerg |

Drei Gründe, warum das mehr Gewicht verdient, als es bisher hatte:

1. **Es fällt als einziges nicht in die Designfalle.** Ob der Bildschirm zu
   blau oder zu hell ist, kann auch jemand mit Farbsehschwäche zuverlässig
   beurteilen. Kein Beleg nötig, kein Messgerät — der Nutzer *spürt* das
   Ergebnis sofort. Entsprechend niedrig ist die Einstiegshürde.
2. **Die Zielgruppe ist um ein Vielfaches größer.** Augenbelastung bei langen
   Sessions betrifft nicht nur Farbsehschwache, sondern praktisch jeden
   Abendspieler. Das kann Nutzer bringen, die gar keine CVD haben — und die
   CVD-Funktionen sind dann für die da, die sie brauchen.
3. **Es bestätigt eine längst getroffene Entscheidung.** Die Umbenennung von
   „Colorblind Assist" zu „Color Balance Assist" war ausdrücklich dafür
   gedacht, das Tool breiter zu positionieren (siehe `CLAUDE.md`). Dieses
   Signal ist der erste empirische Beleg, dass die Entscheidung richtig war.

**Konsequenz für die Basis-Ebene:** Eye Comfort bleibt sichtbar, auch wenn die
Basis ansonsten auf *eine* Funktion reduziert wird. Es ist die zweite
Funktion, die das verdient.

**Ausbaurichtung (noch nicht entschieden):** Denkbar sind Voreinstellungen
(„Abend", „Lange Session"), ein Tagesverlauf, oder kontextabhängige Anpassung
über die bereits vorhandenen Mumble-Daten (`GetCurrentGameContext()` liefert
Map, Maptyp, Kampfstatus). **Achtung:** Kontextautomatik ist genau die Sorte
versteckter Logik, die Abschnitt 7 verbietet — sie wäre nur zulässig, wenn sie
in der Pipeline-Ansicht sichtbar ist und abschaltbar bleibt.

---

## 3. UX-Bedarf ↔ technische Mechanik

Der eigentliche Punkt dieses Dokuments: Fast alles, was die UX braucht,
existiert technisch schon. Die Lücke ist Verbindung und Verpackung, nicht
Fähigkeit.

| UX-Bedarf | Vorhandene Mechanik | Zustand |
|---|---|---|
| Nicht nach Diagnose fragen | Vision Lab: Anomaloskop, AQ-Berechnung, schreibt `Type`/`Severity01` | **existiert, aber hinter Advanced Mode eingesperrt** |
| Beleg am echten Spielinhalt | Halte-Vergleich (Filter aussetzen solange Taste gedrückt) | Toggle `Alt+Shift+O` existiert, **Halten fehlt** |
| Beleg auf einen Blick | Kontrast-Swatches, 9-Tag-Reihe, „N von 9 verschoben" | existiert, **verstreut über drei Orte** |
| Chirurgisch statt invasiv | `HybridScanner`-Overlay statt globaler DWM-Tönung | beides da, **falsche Voreinstellung** |
| Ein Schritt bis zum Erfolg | — | **fehlt: kein geführter Einstieg** |
| Benannte Fälle statt Regler | `LabFilters` mit Priorität, Persistenz, Export-Codes | Engine da, **Defaults abstrakt** |
| Profil teilen | Export/Import als Textcode | existiert, **nicht mit der Messung verbunden** |

### 3.1 Nie nach der Diagnose fragen

„Protan (Rot-Fokus)" ist eine Taxonomie. Der Nutzer soll beantworten, was er
*sieht*, nicht was er *ist*:

> „Kannst du diese beiden Farben unterscheiden?" → ja / nein / unsicher

Aus mehreren solchen Antworten leitet das Tool Typ und Severity ab. Genau so
arbeitet ein Anomaloskop — und die Mechanik liegt fertig im Vision Lab, inkl.
echtem Anomalous Quotient.

**Der Fehler ist die Gatterung, nicht die Funktion.** Das klinisch fundierte
Werkzeug, das CBA vertrauenswürdig *und* selbsterklärend macht, ist aktuell nur
erreichbar, wenn der Nutzer einen Haken bei „Advanced Mode" setzt. Ein
Einsteiger landet stattdessen vor drei Rateknöpfen.

### 3.2 Verifikation gehört in den Moment des Scheiterns

Das Problem passiert im Zerg, nicht im Einstellungsfenster. Der heutige Ablauf
ist: Regler schieben → Fenster schließen → spielen → hoffen.

Was fehlt, ist ein **Halte-Vergleich**: Taste gedrückt = Filter aus, loslassen
= Filter zurück. Damit beantwortet sich „wirkt das?" in einer halben Sekunde am
realen Spielbild statt an einem abstrakten Farbfeld.

Ein Umschalter (wie das vorhandene `Alt+Shift+O`) leistet das *nicht*: Er
kostet zwei Tastendrücke und den Moment. Technisch ist der Unterschied klein —
`ProcessKeybind` und `TryClearAppliedEffect()`/`Recompute()` sind vorhanden;
nötig ist ein Keybind, der auf Loslassen reagiert.

### 3.3 Weniger Urteile bis zum ersten Erfolg

Bis ein Nutzer heute weiß, ob CBA ihm hilft, kann er über Typ, Stärke, Mixed
RG, Mixed BY, Gamma, Toleranz und drei Eye-Comfort-Achsen entscheiden — rund
zehn Entscheidungen vor dem ersten Erfolgserlebnis.

**Ziel: eine Entscheidung.** Tiefe danach, freiwillig. Der Advanced-Mode-Gate
ist dafür bereits das richtige Werkzeug — er steht nur an der falschen Stelle
(er versteckt die Diagnose statt nur die Regler).

### 3.4 Chirurgisch schlägt global

Der erklärte Anspruch war eine Automatik, „die aber super selektiv nur die
entsprechenden Farben filtert". Der Standardeinstieg ist heute das Gegenteil:
eine bildschirmweite DWM-Tönung, die *alles* verfärbt — UI, Karte, Chat,
Mauszeiger.

Das erzeugt zwei Effekte:
- **Gefühlt invasiv.** Alles verändert sich, auch was nicht sollte.
- **Nicht überprüfbar.** Wenn sich das gesamte Bild verschiebt, gibt es keinen
  unveränderten Bezugspunkt mehr, gegen den man vergleichen könnte.

Eine gezielte Korrektur wirkt chirurgisch und erzeugt Vertrauen; eine globale
Tönung erzeugt Unbehagen. Beides existiert bereits als getrennte Ebenen — es
geht ausschließlich um die Frage, was der Einstieg ist und was die Vertiefung.

---

## 4. Der Einstieg (erste 60 Sekunden)

Zielbild, abgeleitet aus den Regeln oben:

1. **Orientierung statt Optionen.** Eine Zeile, die das Problem benennt, nicht
   die Technik: „Commander-Tags und AoE-Kreise besser unterscheiden."
2. **Eine Frage, kein Formular.** „Welche Farben verwechselst du am ehesten?"
   mit visuellen Paaren statt der Protan/Deutan/Tritan-Taxonomie.
3. **Sofort sichtbares Ergebnis**, gezielt — nicht als globale Tönung.
4. **Beleg**: die 9 Tag-Farben als Original→Kontrast-Reihe (existiert bereits)
   plus Hinweis auf den Halte-Vergleich.
5. **Erst danach** der Weg in die Tiefe: Advanced Mode, Studio, Labor.

Der Nutzer trifft genau eine Entscheidung und sieht danach einen Beleg.

---

## 5. Umbaureihenfolge

Nach Hebelwirkung, unter Berücksichtigung dessen, was CBA tatsächlich
blockiert.

**Stufe 0 — Voraussetzung (erledigt 2026-09-11).**
Die Pipeline ist explizit (`core/FilterLayers.*`, sichtbare Stufen 1 und 2).
Das ist die Bedingung dafür, dass Ausbau nicht wieder zu „grip loss" führt —
siehe Abschnitt 7.

**Stufe 1 — Diagnose entsperren.**
Den geführten Einstieg bauen; Vision Labs Messung vor das Advanced-Gate holen.
Entfernt das Urteil, das der Nutzer am wenigsten fällen kann.

**Stufe 2 — Halte-Vergleich.**
Kleinster Eingriff mit der höchsten Vertrauenswirkung. Beantwortet „wirkt es?"
am echten Spielinhalt.

**Stufe 3 — Aufgabe B verpacken.**
AoE-Kreise als benannten, mitgelieferten Fall statt als Laborexperiment. Reine
Verpackung vorhandener Engine.

**Stufe 4 — README als Spielereinstieg.**
Aktuell beginnt sie mit HPE-Matrizen und Viénot-Zitaten. Die Wissenschaft ist
ein echtes Alleinstellungsmerkmal und bleibt — aber nicht als Eröffnung.
Zuerst Vorher/Nachher, dann Technik. (Titel sagt außerdem noch „v1.0".)

**Stufe 5 — Nexus Addon Library.**
Der größte Verteilungshebel: Entdeckung läuft praktisch vollständig über den
Library-Tab in Nexus. Solange der einzige Weg „lade eine fremde .dll von
GitHub" ist, findet die Community das Tool nicht — und sicherheitsbewusste
Spieler installieren es nicht. Dieser Schritt liegt bei Emi (Einreichung bei
Raidcore), nicht im Code.

---

## 6. Die geschlossene Schleife (mittelfristig)

Was CBA von üblichen CVD-Tools unterscheidet, ist nicht die Korrektur — die
haben andere auch. Es ist, dass hier **messen → korrigieren → prüfen → teilen**
vollständig vorhanden ist:

- **Messen**: Vision Lab, echter Anomalous Quotient
- **Korrigieren**: wissenschaftlich fundierte Pipeline
- **Prüfen**: Swatches, Tag-Reihe, Halte-Vergleich
- **Teilen**: Profil-Export als Textcode

Verbunden sind diese vier heute nicht. Genau diese Verbindung ist die
Vertrauensgeschichte, die eine Community trägt: Ein Spieler misst sich einmal,
bekommt ein Profil, kann es belegen — und den Code einem Freund mit derselben
Schwäche geben.

---

## 7. Bewusst nicht

Der historische Bruchpunkt dieses Projekts ist dokumentiert: *„jedes neue
Feature erzwingt eine UI-Reorg."* Deshalb gehören folgende Regeln zum Konzept:

- **Alles Neue tritt als Layer in die bestehende Matrix ein, nicht als neues
  Fenster.** Fünf Fenster sind bereits das Maximum des Erträglichen.
- **Ein Zuhause pro Einstellung.** Jede weitere Kopie eines Reglers ist ein
  künftiger Drift-Bug.
- **Keine versteckte Automatik.** Sichtbar machen schlägt clever regeln. Die
  Filter-Pipeline-Ansicht ist die Umsetzung dieses Prinzips.
- **Kein Modus unterdrückt einen anderen** ohne ausdrücklichen Nutzerwunsch.
- **Kein Neuaufbau.** Die Technik trägt; die Lücke liegt in Verpackung und
  Einstieg.

---

## 8. Offene Entscheidungen

Punkte, die bewusst nicht einseitig entschieden wurden:

- **Globale Korrektur als Standardeinstieg?** Abschnitt 3.4 argumentiert
  dagegen. Das ist jedoch eine Produktentscheidung mit spürbarer Wirkung für
  bestehende Nutzer — Emis Aufruf.
- **`GetBrightnessRetention()` sampelt 8 von 9 Tag-Farben** (Weiß
  übersprungen). Liest sich wie ein Off-by-one, hat aber eine verteidigbare
  Lesart. Eine Änderung verschiebt die Helligkeit für alle sichtbar. Im Code
  markiert, siehe `CLAUDE.md`.
- **Wellenlängen-Elimination** als eigentliche Filter-Lab-Idee: Bänder dämpfen
  statt RGB tauschen. Passt wissenschaftlich zur vorhandenen LMS-Basis, die
  Spektralgraphen existieren bereits — aber eigener, größerer Umbau.

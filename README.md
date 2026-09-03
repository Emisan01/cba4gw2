# Colorblind Assist – Prototyp

Kleines Windows-Tool, das systemweit eine Daltonisierungs-Matrix über die
Windows Magnification API (`MagSetFullscreenColorEffect`) anwendet – derselbe
Mechanismus, den auch die Bordmittel-Farbfilter unter
**Einstellungen > Eingabehilfen > Farbe und hoher Kontrast** nutzen, nur mit
frei wählbarem Typ (Protan/Deutan/Tritan/Mixed) und stufenlosem
Intensitätsregler statt fester Ein/Aus-Presets.

## Build

Voraussetzung: .NET 8 SDK + Windows.

```
dotnet build
dotnet run
```

For a portable Windows x64 executable that includes the .NET runtime:

```
dotnet publish --profile WinX64
```

The modern build targets Windows 10 and 11. The Windows Magnification API itself
is available from Windows 8 onward, but .NET 8 is not supported on Windows 8.1.
Supporting Windows 8/8.1 requires a separate legacy build and should be tested on
the actual operating systems before distribution.

Öffnet sich als normales Fenster, legt sich zusätzlich als Tray-Icon ab.
Minimieren schickt es in den Tray, Doppelklick auf das Tray-Icon holt es
zurück. **Strg+Alt+C** schaltet den Effekt global ein/aus, auch wenn das
Fenster nicht im Fokus ist.

## Funktionsweise

- `Magnification.cs` – P/Invoke-Wrapper um `magnification.dll`
- `ColorMatrix.cs` – Matrix-Mathematik nach dem in daltonize.js/Vischeck
  verbreiteten Fidaner-Ansatz: Farbschwäche simulieren, Differenz zum
  Original berechnen, Differenz auf die verbleibenden Kanäle umverteilen.
  Kollabiert zu einer einzigen 3×3-Matrix, linear zwischen Identität (0%)
  und voller Korrektur (100%) interpoliert.
- `SettingsForm.cs` – UI, ruft bei jeder Änderung `ApplyCurrentSettings()`
  auf, die die aktuelle Matrix berechnet und über den Controller setzt.

## Bekannte Einschränkungen (bitte vor dem Einsatz lesen)

1. **Exklusiver Vollbildmodus wird nicht erfasst.** `MagSetFullscreenColorEffect`
   arbeitet auf DWM-Compositor-Ebene. Läuft ein Spiel (z. B. GW2) im echten
   exklusiven Vollbild statt "Vollbild (Fenster)"/Borderless, umgeht es den
   Compositor – der Effekt greift dann nicht. Für Spiele: Borderless/
   Fenstermodus verwenden.
2. **Kollidiert mit den Windows-eigenen Farbfiltern.** Es kann jeweils nur
   ein systemweiter Vollbild-Farbeffekt aktiv sein. Sind die
   Bordmittel-Farbfilter gleichzeitig aktiv, überschreiben sie sich
   gegenseitig – vor dem Testen die Windows-eigenen Filter deaktivieren.
3. **Mixed-Modus nutzt aktuell Deutan als Rot-Grün-Basis**, nicht Protan.
   Für die meiste Rot-Grün-Schwäche ist das eine brauchbare Näherung, aber
   keine exakte Unterscheidung – eine spätere Version könnte hier zusätzlich
   Protan/Deutan getrennt wählbar machen.
4. **Ungetestet.** Ich konnte diesen Code in dieser Umgebung nicht unter
   Windows kompilieren – bitte in Visual Studio / mit `dotnet build`
   gegenprüfen, bevor du dich darauf verlässt. API-Namen und Struct-Layout
   habe ich nach bestem Wissen aus `magnification.h` übertragen, aber ohne
   Testlauf gebe ich dafür keine Garantie.
5. **Kein Admin-Manifest hinterlegt.** Falls `MagSetFullscreenColorEffect`
   mit Zugriffsfehler fehlschlägt, testweise als Administrator ausführen.

# Mathematische & Wissenschaftliche Dokumentation: Color Balance Assist (cba4gw2)

Diese Dokumentation beschreibt die vollständige mathematische Herleitung, die wissenschaftlichen Referenzen und die softwaretechnische Umsetzung der Farblogik in **Color Balance Assist (cba4gw2) v1.0**.

---

## 1. Übersicht & Zielsetzung

Color Balance Assist führt eine hardwarebeschleunigte **Echtzeit-Farbbalance-Optimierung** auf dem gesamten Bildschirminhalt von Guild Wars 2 durch.
Ziel ist es nicht, die Farben beliebig zu verfremden, sondern **verlorene Kontraste messbar in für den jeweiligen Nutzertyp sichtbare Farbkanäle umzuverteilen**, während neutrale Farbtöne (Weiß, Grau, Schwarz) unverändert bleiben.

Die Pipeline basiert auf drei wissenschaftlichen Säulen:
1. **Farbraum-Transformation:** Hunt-Pointer-Estévez (HPE) nach Viénot et al. (1999)
2. **Dichromasie-Simulation:** Physiologische Projektion nach Viénot, Brettel & Mollon (1999)
3. **Fehler-Kompensation (Daltonisierung):** Fehlermatrix-Umverteilung nach Fidaner, Lin & Özgüven (2005)

---

## 2. Der sRGB $\to$ LMS Farbraum (Hunt-Pointer-Estévez)

Das menschliche Farbsehen basiert auf drei Zapfentypen in der Netzhaut:
* **L** (Long wavelength / Rot-sensitiv)
* **M** (Medium wavelength / Grün-sensitiv)
* **S** (Short wavelength / Blau-sensitiv)

### Warum nicht Reinhard et al. (2001)?
Frühere Vorab-Versionen nutzten eine Matrix aus Erik Reinhard et al. (2001, *„Color Transfer between Images“*). Diese Matrix (`0.3811, 0.5783, 0.0402`) diente jedoch ausschließlich der statistischen Dekorrelation von Fotos im $l\alpha\beta$-Raum. 
Wird darin ein Zapfenkanal (z. B. $M$) genullt, entfällt der subtraktive Term ($-3{,}5873 \cdot M$) in der Rücktransformation: Reines Weiß $(1, 1, 1)$ explodiert auf $R' = 4{,}59$ (459 %), während Grün auf $-1{,}38$ abstürzt. Dies war die mathematische Ursache des historischen "Rot-Screen-Bugs".

### Die HPE-Transformation (Viénot 1999)
Die korrekte, wissenschaftliche Standardmatrix zur Umrechnung von linearem sRGB in den physiologischen LMS-Zapfenraum lautet:

$$\begin{pmatrix} L \\ M \\ S \end{pmatrix} = \mathbf{M}_{RGB \to LMS} \cdot \begin{pmatrix} R \\ G \\ B \end{pmatrix}$$

$$\mathbf{M}_{RGB \to LMS} = \begin{pmatrix} 
17{,}8824 & 43{,}5161 & 4{,}11935 \\ 
3{,}45565 & 27{,}1554 & 3{,}86714 \\ 
0{,}0299566 & 0{,}184309 & 1{,}46709 
\end{pmatrix}$$

### Die inverse Transformation ($LMS \to RGB$)
Die exakte analytische Inverse $\mathbf{M}_{LMS \to RGB} = (\mathbf{M}_{RGB \to LMS})^{-1}$ lautet:

$$\mathbf{M}_{LMS \to RGB} = \begin{pmatrix} 
 0{,}0809444479 & -0{,}1305044092 &  0{,}1167210664 \\ 
-0{,}0102485335 &  0{,}0540193266 & -0{,}1136147082 \\ 
-0{,}0003652969 & -0{,}0041216147 &  0{,}6935114049 
\end{pmatrix}$$

---

## 3. Physiologische Dichromasie-Simulation (Viénot, Brettel & Mollon 1999)

Ein Mensch mit individueller Farbwahrnehmung sieht bei weißem Licht nicht schwarz oder rot; das Gehirn verarbeitet das verbleibende Signal entlang der Neutralachse.
Echte Dichromasie wird daher **nicht durch Nullsetzen eines Zapfens**, sondern durch **Projektion des fehlenden Zapfens auf die Ebene der verbleibenden Zapfen** modelliert:

### A. Deuteranopie ($M$-Zapfen / Grün-Fokus)
Der $M$-Zapfen wird als Linearkombination aus $L$ und $S$ geschätzt:
$$M' = 0{,}494207 \cdot L + 1{,}24827 \cdot S$$

$$\mathbf{CVD}_{Deutan} = \begin{pmatrix} 
1{,}0 & 0{,}0 & 0{,}0 \\ 
0{,}494207 & 0{,}0 & 1{,}24827 \\ 
0{,}0 & 0{,}0 & 1{,}0 
\end{pmatrix}$$

### B. Protanopie ($L$-Zapfen / Rot-Fokus)
Der $L$-Zapfen wird als Linearkombination aus $M$ und $S$ geschätzt:
$$L' = 2{,}02344 \cdot M - 2{,}52581 \cdot S$$

$$\mathbf{CVD}_{Protan} = \begin{pmatrix} 
0{,}0 & 2{,}02344 & -2{,}52581 \\ 
0{,}0 & 1{,}0 & 0{,}0 \\ 
0{,}0 & 0{,}0 & 1{,}0 
\end{pmatrix}$$

### C. Tritanopie ($S$-Zapfen / Blau-Fokus)
Der $S$-Zapfen wird als Linearkombination aus $L$ und $M$ geschätzt:
$$S' = -0{,}395913 \cdot L + 0{,}801109 \cdot M$$

$$\mathbf{CVD}_{Tritan} = \begin{pmatrix} 
1{,}0 & 0{,}0 & 0{,}0 \\ 
0{,}0 & 1{,}0 & 0{,}0 \\ 
-0{,}395913 & 0{,}801109 & 0{,}0 
\end{pmatrix}$$

### Die zusammengesetzte Simulationsmatrix im RGB-Raum
$$\mathbf{Sim} = \mathbf{M}_{LMS \to RGB} \cdot \mathbf{CVD} \cdot \mathbf{M}_{RGB \to LMS}$$

### Mathematischer Beweis der Neutralachsen-Invarianz:
Für reines Weiß $\vec{v}_{weiss} = (1{,}0, 1{,}0, 1{,}0)^T$ gilt:
$$\mathbf{Sim}_{Deutan} \cdot \begin{pmatrix} 1{,}0 \\ 1{,}0 \\ 1{,}0 \end{pmatrix} = \begin{pmatrix} 1{,}000 \\ 1{,}000 \\ 1{,}000 \end{pmatrix}$$
$$\mathbf{Sim}_{Protan} \cdot \begin{pmatrix} 1{,}0 \\ 1{,}0 \\ 1{,}0 \end{pmatrix} = \begin{pmatrix} 1{,}000 \\ 1{,}000 \\ 1{,}000 \end{pmatrix}$$
$$\mathbf{Sim}_{Tritan} \cdot \begin{pmatrix} 1{,}0 \\ 1{,}0 \\ 1{,}0 \end{pmatrix} = \begin{pmatrix} 1{,}000 \\ 1{,}000 \\ 1{,}000 \end{pmatrix}$$

**Resultat:** Weiß bleibt unter allen drei Profilen exakt $(1, 1, 1)$.

---

## 4. Daltonisierungs-Kompensation (Fidaner et al. 2005)

### Schritt 1: Berechnung des Wahrnehmungsfehlers
Für jedes Pixel wird die Differenz zwischen Originalfarbe und simulierter Wahrnehmung berechnet:
$$\vec{E} = \vec{I}_{orig} - \mathbf{Sim} \cdot \vec{I}_{orig} = (\mathbf{I} - \mathbf{Sim}) \cdot \vec{I}_{orig}$$
Für Weiß gilt: $\vec{E}_{weiss} = (0, 0, 0)^T$. Es entsteht keinerlei Störsignal.

### Schritt 2: Fehlershift auf sichtbare Kanäle
Der verlorene Kontrast $\vec{E}$ wird über eine typspezifische Shift-Matrix $\mathbf{Shift}$ in Kanäle umverteilt, die der Nutzer wahrnehmen kann:

* **Protanopia ($Shift_{Protan}$):** Roter Fehler wird zu 70 % auf Grün und Blau verschoben:
  $$\mathbf{Shift}_{Protan} = \begin{pmatrix} 0{,}0 & 0{,}0 & 0{,}0 \\ 0{,}7 & 0{,}0 & 0{,}0 \\ 0{,}7 & 0{,}0 & 0{,}0 \end{pmatrix}$$

* **Deuteranopia ($Shift_{Deutan}$):** Grüner Fehler wird zu 70 % auf Rot und Blau verschoben:
  $$\mathbf{Shift}_{Deutan} = \begin{pmatrix} 0{,}0 & 0{,}7 & 0{,}0 \\ 0{,}0 & 0{,}0 & 0{,}0 \\ 0{,}0 & 0{,}7 & 0{,}0 \end{pmatrix}$$

* **Tritanopia ($Shift_{Tritan}$):** Blauer Fehler wird zu 70 % auf Rot und Grün verschoben:
  $$\mathbf{Shift}_{Tritan} = \begin{pmatrix} 0{,}0 & 0{,}0 & 0{,}7 \\ 0{,}0 & 0{,}0 & 0{,}7 \\ 0{,}0 & 0{,}0 & 0{,}0 \end{pmatrix}$$

### Schritt 3: Gesamtkorrekturmatrix $\mathbf{M}_{full}$
$$\vec{I}_{korr} = \vec{I}_{orig} + \mathbf{Shift} \cdot \vec{E}$$
$$\vec{I}_{korr} = \vec{I}_{orig} + \mathbf{Shift} \cdot (\mathbf{I} - \mathbf{Sim}) \cdot \vec{I}_{orig}$$
$$\mathbf{M}_{full} = \mathbf{I} + \mathbf{Shift} - \mathbf{Shift} \cdot \mathbf{Sim}$$

### Schritt 4: Stärkeregelung (Severity Interpolation)
Mit dem Schieberegler $s \in [0{,}0, 1{,}0]$ wird linear zwischen der Einheitsmatrix $\mathbf{I}$ und der Vollkorrektur $\mathbf{M}_{full}$ interpoliert:
$$\mathbf{M}(s) = (1 - s) \cdot \mathbf{I} + s \cdot \mathbf{M}_{full}$$

* Bei $s \le 0{,}0001$ greift ein deterministischer **Fast-Path**: Es wird exakt die Einheitsmatrix $\mathbf{I}$ zurückgegeben (100 % neutral, null Rechenlast).

---

## 5. Gemischte Farbanpassung (Mixed Mode)

Für Nutzer mit kombinierter Rot-Grün- und Blau-Gelb-Farbbalance werden zwei unabhängige Achsen kombiniert:
$$\mathbf{M}_{mixed} = \mathbf{M}_{Tritan}(s_{BY}) \cdot \mathbf{M}_{Deutan}(s_{RG})$$

---

## 6. Hardware-Ausgabe via Windows Magnification API

Die resultierende $3 \times 3$ Matrix wird in die $5 \times 5$ Datenstruktur `MAGCOLOREFFECT` der Windows Desktop Window Manager (DWM) API überführt:

$$\mathbf{T}_{5 \times 5} = \begin{pmatrix} 
m_{00} & m_{10} & m_{20} & 0 & 0 \\ 
m_{01} & m_{11} & m_{21} & 0 & 0 \\ 
m_{02} & m_{12} & m_{22} & 0 & 0 \\ 
0 & 0 & 0 & 1 & 0 \\ 
0 & 0 & 0 & 0 & 1 
\end{pmatrix}$$

*(Hinweis: Die Magnification API erwartet column-major bzw. transponierte Notation für Farbvektoren als Spalten. Zeilen 3 und 4 bleiben Identität, damit Alpha und Helligkeits-Bias unverändert durchlaufen).*

---

## 7. Literaturverzeichnis

1. **Viénot, F., Brettel, H., & Mollon, J. D. (1999).**  
   *Digital video colourmaps for checking the legibility of displays by dichromats.*  
   Color Research & Application, 24(4), 243–252.
2. **Brettel, H., Viénot, F., & Mollon, J. D. (1997).**  
   *Computerized simulation of color appearance for dichromats.*  
   Journal of the Optical Society of America A, 14(10), 2647–2655.
3. **Fidaner, I. B., Lin, N. H., & Özgüven, R. (2005).**  
   *Analysis of Color Blindness and Computer-Aided Daltonization for Natural Images.*  
   Stanford University Technical Report.
4. **W3C Web Accessibility Initiative (WAI). (2018).**  
   *Web Content Accessibility Guidelines (WCAG) 2.1 – Contrast (Minimum) Success Criterion 1.4.3.*

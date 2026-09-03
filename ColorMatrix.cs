namespace ColorblindAssist;

public enum DeficiencyType
{
    Protan,
    Deutan,
    Tritan
}

// Berechnet Daltonisierungs-Matrizen nach dem in daltonize.js / Vischeck
// verbreiteten Ansatz (Fidaner, Lin, Ozguven 2005): eine lineare Simulation
// der jeweiligen Farbschwaeche, der Fehler zwischen Original und Simulation,
// und eine Umverteilung dieses Fehlers auf die noch intakten Kanaele.
//
// Wichtig: das ist eine praktikable Naeherung, keine klinisch kalibrierte
// Loesung. Fuer eine erste, nutzbare Version reicht das; fuer eine spaetere
// Version waere ein Abgleich mit echten Anomaloskop-/Cambridge-Colour-Test-
// Werten sinnvoll.
public static class ColorMatrix
{
    // Vereinfachte RGB-Simulationsmatrizen pro Typ (voller Schweregrad = Anopie).
    private static readonly double[,] SimProtan =
    {
        { 0.56667, 0.43333, 0.00000 },
        { 0.55833, 0.44167, 0.00000 },
        { 0.00000, 0.24167, 0.75833 }
    };

    private static readonly double[,] SimDeutan =
    {
        { 0.62500, 0.37500, 0.00000 },
        { 0.70000, 0.30000, 0.00000 },
        { 0.00000, 0.30000, 0.70000 }
    };

    private static readonly double[,] SimTritan =
    {
        { 0.95000, 0.05000, 0.00000 },
        { 0.00000, 0.43333, 0.56667 },
        { 0.00000, 0.47500, 0.52500 }
    };

    // Verteilt den "verlorenen" Farbanteil auf die verbleibenden Kanaele um.
    private static readonly double[,] ErrorRedistribution =
    {
        { 0.0, 0.0, 0.0 },
        { 0.7, 1.0, 0.0 },
        { 0.7, 0.0, 1.0 }
    };

    private static readonly double[,] Identity3 =
    {
        { 1, 0, 0 },
        { 0, 1, 0 },
        { 0, 0, 1 }
    };

    private static double[,] SimulationMatrix(DeficiencyType type) => type switch
    {
        DeficiencyType.Protan => SimProtan,
        DeficiencyType.Deutan => SimDeutan,
        DeficiencyType.Tritan => SimTritan,
        _ => Identity3
    };

    private static double[,] Multiply(double[,] a, double[,] b)
    {
        var result = new double[3, 3];
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
            {
                double sum = 0;
                for (int k = 0; k < 3; k++)
                    sum += a[i, k] * b[k, j];
                result[i, j] = sum;
            }
        return result;
    }

    private static double[,] Add(double[,] a, double[,] b)
    {
        var result = new double[3, 3];
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                result[i, j] = a[i, j] + b[i, j];
        return result;
    }

    private static double[,] Lerp(double[,] a, double[,] b, double t)
    {
        var result = new double[3, 3];
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                result[i, j] = a[i, j] + (b[i, j] - a[i, j]) * t;
        return result;
    }

    // Kombinierte Daltonisierungsmatrix fuer einen Typ bei gegebenem
    // Schweregrad (0.0 = keine Korrektur, 1.0 = volle Korrektur fuer Anopie).
    public static double[,] CorrectionMatrix(DeficiencyType type, double severity01)
    {
        var sim = SimulationMatrix(type);
        // M = I + Err - Err * Sim
        var errTimesSim = Multiply(ErrorRedistribution, sim);
        var full = Add(Add(Identity3, ErrorRedistribution), Negate(errTimesSim));
        return Lerp(Identity3, full, Clamp01(severity01));
    }

    // Mixed-Modus: zwei unabhaengige Achsen (Rot-Gruen ueber Deutan als
    // Basistyp, Blau-Gelb ueber Tritan) werden nacheinander angewendet.
    // Eine spaetere Version koennte Protan statt Deutan als RG-Basis
    // anbieten, wenn das fuer den Einzelfall praeziser ist.
    public static double[,] MixedCorrectionMatrix(double rgSeverity01, double bySeverity01)
    {
        var rg = CorrectionMatrix(DeficiencyType.Deutan, rgSeverity01);
        var by = CorrectionMatrix(DeficiencyType.Tritan, bySeverity01);
        return Multiply(by, rg);
    }

    private static double[,] Negate(double[,] a)
    {
        var result = new double[3, 3];
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                result[i, j] = -a[i, j];
        return result;
    }

    private static double Clamp01(double v) => v < 0 ? 0 : v > 1 ? 1 : v;

    public static MagColorEffect ToMagColorEffect(double[,] m3x3)
    {
        return new MagColorEffect
        {
            M00 = (float)m3x3[0, 0], M01 = (float)m3x3[0, 1], M02 = (float)m3x3[0, 2], M03 = 0, M04 = 0,
            M10 = (float)m3x3[1, 0], M11 = (float)m3x3[1, 1], M12 = (float)m3x3[1, 2], M13 = 0, M14 = 0,
            M20 = (float)m3x3[2, 0], M21 = (float)m3x3[2, 1], M22 = (float)m3x3[2, 2], M23 = 0, M24 = 0,
            M30 = 0, M31 = 0, M32 = 0, M33 = 1, M34 = 0,
            M40 = 0, M41 = 0, M42 = 0, M43 = 0, M44 = 1
        };
    }
}

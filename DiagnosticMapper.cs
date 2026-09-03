namespace ColorblindAssist;

internal enum HrrLevel
{
    None,
    Mild,
    Moderate,
    Severe,
    Complete
}

internal static class DiagnosticMapper
{
    // AQ_MAX_DEUTAN is a heuristic upper bound, not a clinical standard.
    private const double AqMaxDeutan = 4.0;

    public static double? ResolveSeverity(int typeIndex, double? aq, HrrLevel hrr)
    {
        double? fromAq = typeIndex switch
        {
            0 when aq is <= 0.7 => Clamp((0.7 - aq.Value) / 0.7),
            0 when aq is not null => 0.0,
            1 when aq is >= 1.4 => Clamp((aq.Value - 1.4) / (AqMaxDeutan - 1.4)),
            1 when aq is not null => 0.0,
            _ => null
        };

        double? fromHrr = hrr switch
        {
            HrrLevel.Mild => 0.25,
            HrrLevel.Moderate => 0.55,
            HrrLevel.Severe => 0.85,
            HrrLevel.Complete => 1.0,
            _ => null
        };

        return fromAq ?? fromHrr;
    }

    private static double Clamp(double value) => Math.Max(0, Math.Min(1, value));
}

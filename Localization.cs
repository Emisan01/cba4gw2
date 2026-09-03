using System.Text.Json;

namespace ColorblindAssist;

internal enum AppLanguage
{
    English,
    German
}

internal static class Localization
{
    private static readonly string SettingsPath = Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
        "ColorblindAssist",
        "settings.json");

    public static AppLanguage Current { get; private set; } = AppLanguage.English;

    public static void Load()
    {
        try
        {
            if (File.Exists(SettingsPath))
            {
                var settings = JsonSerializer.Deserialize<LanguageSettings>(File.ReadAllText(SettingsPath));
                if (Enum.TryParse(settings?.Language, out AppLanguage savedLanguage))
                {
                    Current = savedLanguage;
                }
            }
        }
        catch
        {
            Current = AppLanguage.English;
        }
    }

    public static void SetLanguage(AppLanguage language)
    {
        Current = language;

        try
        {
            Directory.CreateDirectory(Path.GetDirectoryName(SettingsPath)!);
            File.WriteAllText(SettingsPath, JsonSerializer.Serialize(new LanguageSettings { Language = language.ToString() }));
        }
        catch
        {
            // A missing preference should never prevent the app from starting.
        }
    }

    public static string LanguageName(AppLanguage language) => language switch
    {
        AppLanguage.German => "Deutsch",
        _ => "English"
    };

    public static string Enabled => Current == AppLanguage.German ? "Aktiv" : "Enabled";
    public static string HotkeyHint => Current == AppLanguage.German
        ? "Strg+Alt+C schaltet global um"
        : "Ctrl+Alt+C toggles the effect globally";
    public static string Type => Current == AppLanguage.German ? "Typ" : "Type";
    public static string Intensity(int value) => Current == AppLanguage.German ? $"Intensität: {value}%" : $"Intensity: {value}%";
    public static string RedGreen(int value) => Current == AppLanguage.German ? $"Rot-Grün-Achse: {value}%" : $"Red-green axis: {value}%";
    public static string BlueYellow(int value) => Current == AppLanguage.German ? $"Blau-Gelb-Achse: {value}%" : $"Blue-yellow axis: {value}%";
    public static string Reset => Current == AppLanguage.German ? "Zurücksetzen" : "Reset";
    public static string Open => Current == AppLanguage.German ? "Öffnen" : "Open";
    public static string Exit => Current == AppLanguage.German ? "Beenden" : "Exit";
    public static string Language => Current == AppLanguage.German ? "Sprache" : "Language";
    public static string SaveSettings => Current == AppLanguage.German ? "Einstellungen speichern" : "Save settings";
    public static string StartWithWindows => Current == AppLanguage.German ? "Mit Windows starten" : "Start with Windows";
    public static string SettingsSaved => Current == AppLanguage.German ? "Einstellungen gespeichert" : "Settings saved";
    public static string FilterApplyFailed => Current == AppLanguage.German
        ? "Der Windows-Farbfilter konnte nicht angewendet werden."
        : "The Windows color filter could not be applied.";
    public static string OverviewTitle => Current == AppLanguage.German ? "Dein Farbprofil" : "Your color profile";
    public static string OverviewText => Current == AppLanguage.German
        ? "Passe die Korrektur an, bis Farben für dich leichter unterscheidbar sind."
        : "Adjust the correction until colors are easier for you to distinguish.";
    public static string OverviewTip => Current == AppLanguage.German
        ? "Tipp: Kleine Änderungen können einen großen Unterschied machen."
        : "Tip: Small adjustments can make a big difference.";
    public static string DiagnosticValueKnown => Current == AppLanguage.German ? "Diagnosewert vorhanden" : "Diagnostic value known";
    public static string AqValue => Current == AppLanguage.German ? "Anomaloskop AQ:" : "Anomaloscope AQ:";
    public static string HrrLevel => Current == AppLanguage.German ? "HRR-Einstufung:" : "HRR level:";
    public static string DiagnosticHint => Current == AppLanguage.German
        ? "Näherungswert, keine klinisch validierte Umrechnung."
        : "Approximation, not a clinically validated conversion.";
    public static string[] HrrLevels => Current == AppLanguage.German
        ? new[] { "Keine Angabe", "Leicht", "Mittel", "Stark", "Vollständig" }
        : new[] { "Not provided", "Mild", "Moderate", "Severe", "Complete" };
    public static string ApproximationHint => Current == AppLanguage.German
        ? "Näherungswert aus dem Befund. Danach visuell feinjustieren."
        : "Approximate starting point from a test result. Fine-tune visually afterwards.";

    private sealed class LanguageSettings
    {
        public string Language { get; set; } = AppLanguage.English.ToString();
    }
}

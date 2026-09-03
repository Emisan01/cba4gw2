using Microsoft.Win32;
using System.Text.Json;

namespace ColorblindAssist;

internal sealed class AppPreferences
{
    private static readonly string SettingsPath = Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
        "ColorblindAssist",
        "preferences.json");

    public bool Enabled { get; set; } = true;
    public int TypeIndex { get; set; } = 2;
    public int SingleIntensity { get; set; } = 50;
    public int RedGreenIntensity { get; set; } = 50;
    public int BlueYellowIntensity { get; set; } = 50;
    public bool StartWithWindow { get; set; }
    public bool DiagnosticValueKnown { get; set; }
    public double? DiagnosticAq { get; set; }
    public int DiagnosticHrrLevel { get; set; }

    public static AppPreferences Load()
    {
        try
        {
            if (File.Exists(SettingsPath))
            {
                return JsonSerializer.Deserialize<AppPreferences>(File.ReadAllText(SettingsPath)) ?? new AppPreferences();
            }
        }
        catch
        {
            // Corrupt preferences should fall back to safe defaults.
        }

        return new AppPreferences();
    }

    public void Save()
    {
        try
        {
            Directory.CreateDirectory(Path.GetDirectoryName(SettingsPath)!);
            File.WriteAllText(SettingsPath, JsonSerializer.Serialize(this, new JsonSerializerOptions { WriteIndented = true }));
        }
        catch
        {
            // Preferences are optional and should never block the UI.
        }

        try
        {
            using var startupKey = Registry.CurrentUser.OpenSubKey(
                "Software\\Microsoft\\Windows\\CurrentVersion\\Run", writable: true);
            if (startupKey is null) return;

            const string valueName = "ColorblindAssist";
            if (StartWithWindow)
            {
                var launcherPath = Path.Combine(AppContext.BaseDirectory, "Start-ColorblindAssist.cmd");
                var command = File.Exists(launcherPath)
                    ? $"\"{Environment.GetEnvironmentVariable("ComSpec") ?? "cmd.exe"}\" /c \"\"{launcherPath}\"\""
                    : $"\"{Application.ExecutablePath}\"";
                startupKey.SetValue(valueName, command);
            }
            else
            {
                startupKey.DeleteValue(valueName, throwOnMissingValue: false);
            }
        }
        catch
        {
            // Startup registration may be unavailable on restricted systems.
        }
    }
}

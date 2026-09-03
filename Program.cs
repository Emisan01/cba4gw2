namespace ColorblindAssist;

internal static class Program
{
    [STAThread]
    private static void Main()
    {
        using var instanceLock = new Mutex(true, "ColorblindAssist.SingleInstance", out var isFirstInstance);
        if (!isFirstInstance) return;

        ApplicationConfiguration.Initialize();
        Application.SetUnhandledExceptionMode(UnhandledExceptionMode.CatchException);
        Application.ThreadException += (_, args) =>
            MessageBox.Show(args.Exception.Message, "Colorblind Assist", MessageBoxButtons.OK, MessageBoxIcon.Error);

        try
        {
            Application.Run(new SettingsForm());
        }
        catch (DllNotFoundException)
        {
            MessageBox.Show(
                "Windows Magnification API is unavailable on this system.",
                "Colorblind Assist",
                MessageBoxButtons.OK,
                MessageBoxIcon.Error);
        }
    }
}

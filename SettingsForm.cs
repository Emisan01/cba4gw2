using System.Runtime.InteropServices;

namespace ColorblindAssist;

public sealed class SettingsForm : Form
{
    private readonly ColorEffectController _controller = new();
    private readonly NotifyIcon _trayIcon;

    private CheckBox _enabledCheck = null!;
    private RadioButton _rbProtan = null!;
    private RadioButton _rbDeutan = null!;
    private RadioButton _rbTritan = null!;
    private RadioButton _rbMixed = null!;
    private TrackBar _singleSlider = null!;
    private Label _singleLabel = null!;
    private Panel _singlePanel = null!;
    private Panel _mixedPanel = null!;
    private TrackBar _rgSlider = null!;
    private Label _rgLabel = null!;
    private TrackBar _bySlider = null!;
    private Label _byLabel = null!;
    private ComboBox _languageCombo = null!;
    private Label _hotkeyHint = null!;
    private GroupBox _typeGroup = null!;
    private Button _resetButton = null!;
    private CheckBox _startWithWindowCheck = null!;
    private Button _saveButton = null!;
    private Label _saveStatus = null!;
    private Label _overviewTitle = null!;
    private Label _overviewText = null!;
    private Label _overviewTip = null!;
    private AppPreferences _preferences = null!;
    private ColorCurveView _curveView = null!;
    private GroupBox _diagnosticGroup = null!;
    private CheckBox _diagnosticKnownCheck = null!;
    private Label _aqLabel = null!;
    private TextBox _aqTextBox = null!;
    private Label _hrrLabel = null!;
    private ComboBox _hrrCombo = null!;
    private Label _diagnosticHint = null!;
    private ToolStripMenuItem _openMenuItem = null!;
    private ToolStripMenuItem _exitMenuItem = null!;

    private const int HOTKEY_ID = 0xC01B;
    private const uint MOD_CONTROL = 0x0002;
    private const uint MOD_ALT = 0x0001;
    private const uint VK_C = 0x43;

    [DllImport("user32.dll")]
    private static extern bool RegisterHotKey(IntPtr hWnd, int id, uint fsModifiers, uint vk);

    [DllImport("user32.dll")]
    private static extern bool UnregisterHotKey(IntPtr hWnd, int id);

    public SettingsForm()
    {
        Localization.Load();
        _preferences = AppPreferences.Load();
        Text = "Colorblind Assist";
        Width = 800;
        Height = 650;
        FormBorderStyle = FormBorderStyle.FixedDialog;
        MaximizeBox = false;
        StartPosition = FormStartPosition.CenterScreen;

        BuildUi();

        _trayIcon = new NotifyIcon
        {
            Icon = SystemIcons.Application,
            Text = "Colorblind Assist",
            Visible = true,
            ContextMenuStrip = BuildTrayMenu()
        };
        _trayIcon.DoubleClick += (_, _) => ShowFromTray();

        Load += (_, _) =>
        {
            if (!_controller.Initialize())
            {
                MessageBox.Show(
                    "The Windows color effect could not be initialized. The application will remain available, but no filter can be applied.",
                    "Colorblind Assist",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Warning);
                return;
            }

            ApplyCurrentSettings();
        };
        FormClosing += OnFormClosing;
        Resize += (_, _) =>
        {
            if (WindowState == FormWindowState.Minimized)
            {
                Hide();
            }
        };
    }

    protected override void OnHandleCreated(EventArgs e)
    {
        base.OnHandleCreated(e);
        RegisterHotKey(Handle, HOTKEY_ID, MOD_CONTROL | MOD_ALT, VK_C);
    }

    protected override void WndProc(ref Message m)
    {
        const int WM_HOTKEY = 0x0312;
        if (m.Msg == WM_HOTKEY && m.WParam.ToInt32() == HOTKEY_ID)
        {
            _enabledCheck.Checked = !_enabledCheck.Checked;
        }
        base.WndProc(ref m);
    }

    private ContextMenuStrip BuildTrayMenu()
    {
        var menu = new ContextMenuStrip();
        _openMenuItem = new ToolStripMenuItem(Localization.Open);
        _openMenuItem.Click += (_, _) => ShowFromTray();
        menu.Items.Add(_openMenuItem);
        menu.Items.Add(new ToolStripSeparator());
        _exitMenuItem = new ToolStripMenuItem(Localization.Exit);
        _exitMenuItem.Click += (_, _) => Close();
        menu.Items.Add(_exitMenuItem);
        return menu;
    }

    private void ShowFromTray()
    {
        Show();
        WindowState = FormWindowState.Normal;
        Activate();
    }

    private void OnFormClosing(object? sender, FormClosingEventArgs e)
    {
        _trayIcon.Visible = false;
        UnregisterHotKey(Handle, HOTKEY_ID);
        _controller.Dispose();
    }

    private void BuildUi()
    {
        var outer = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            ColumnCount = 2,
            Padding = new Padding(16),
            ColumnStyles = { new ColumnStyle(SizeType.Absolute, 410), new ColumnStyle(SizeType.Percent, 100) }
        };
        Controls.Add(outer);

        var root = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            ColumnCount = 1,
            Padding = new Padding(0, 0, 18, 0)
        };
        outer.Controls.Add(root, 0, 0);
        outer.Controls.Add(BuildOverviewPanel(), 1, 0);

        _enabledCheck = new CheckBox { Text = Localization.Enabled, Checked = _preferences.Enabled, AutoSize = true };
        _enabledCheck.CheckedChanged += (_, _) => ApplyCurrentSettings();
        root.Controls.Add(_enabledCheck);

        _hotkeyHint = new Label
        {
            Text = Localization.HotkeyHint,
            AutoSize = true,
            ForeColor = SystemColors.GrayText,
            Margin = new Padding(0, 0, 0, 12)
        };
        root.Controls.Add(_hotkeyHint);

        var languageRow = new FlowLayoutPanel { AutoSize = true, Margin = new Padding(0, 0, 0, 12) };
        languageRow.Controls.Add(new Label { Text = Localization.Language + ":", AutoSize = true, Margin = new Padding(0, 6, 8, 0) });
        _languageCombo = new ComboBox { DropDownStyle = ComboBoxStyle.DropDownList, Width = 120 };
        _languageCombo.Items.AddRange(Enum.GetValues<AppLanguage>().Select(Localization.LanguageName).ToArray());
        _languageCombo.SelectedIndex = (int)Localization.Current;
        _languageCombo.SelectedIndexChanged += (_, _) =>
        {
            Localization.SetLanguage((AppLanguage)_languageCombo.SelectedIndex);
            ApplyLanguage();
        };
        languageRow.Controls.Add(_languageCombo);
        root.Controls.Add(languageRow);

        _typeGroup = new GroupBox { Text = Localization.Type, AutoSize = true, Width = 340 };
        var typeFlow = new FlowLayoutPanel { AutoSize = true, Padding = new Padding(8) };
        _rbProtan = new RadioButton { Text = "Protan", AutoSize = true };
        _rbDeutan = new RadioButton { Text = "Deutan", AutoSize = true };
        _rbTritan = new RadioButton { Text = "Tritan", AutoSize = true, Checked = true };
        _rbMixed = new RadioButton { Text = "Mixed", AutoSize = true };
        foreach (var rb in new[] { _rbProtan, _rbDeutan, _rbTritan, _rbMixed })
        {
            rb.Margin = new Padding(0, 0, 16, 4);
            rb.CheckedChanged += OnTypeChanged;
            typeFlow.Controls.Add(rb);
        }
        _typeGroup.Controls.Add(typeFlow);
        root.Controls.Add(_typeGroup);

        _diagnosticGroup = new GroupBox
        {
            Width = 390,
            AutoSize = true,
            Padding = new Padding(10),
            Margin = new Padding(0, 10, 0, 0)
        };
        var diagnosticLayout = new TableLayoutPanel { AutoSize = true, ColumnCount = 2, Dock = DockStyle.Fill };
        _diagnosticKnownCheck = new CheckBox
        {
            Text = Localization.DiagnosticValueKnown,
            Checked = _preferences.DiagnosticValueKnown,
            AutoSize = true,
            Margin = new Padding(0, 0, 0, 4)
        };
        diagnosticLayout.Controls.Add(_diagnosticKnownCheck, 0, 0);
        diagnosticLayout.SetColumnSpan(_diagnosticKnownCheck, 2);
        _aqLabel = new Label { Text = Localization.AqValue, AutoSize = true, Anchor = AnchorStyles.Left, Margin = new Padding(0, 5, 8, 4) };
        _aqTextBox = new TextBox { Width = 100, Text = _preferences.DiagnosticAq?.ToString(System.Globalization.CultureInfo.InvariantCulture) ?? "" };
        _hrrLabel = new Label { Text = Localization.HrrLevel, AutoSize = true, Anchor = AnchorStyles.Left, Margin = new Padding(0, 5, 8, 4) };
        _hrrCombo = new ComboBox { DropDownStyle = ComboBoxStyle.DropDownList, Width = 150 };
        _hrrCombo.Items.AddRange(Localization.HrrLevels);
        _hrrCombo.SelectedIndex = Math.Clamp(_preferences.DiagnosticHrrLevel, 0, 4);
        _diagnosticHint = new Label { Text = Localization.DiagnosticHint, AutoSize = true, ForeColor = SystemColors.GrayText, MaximumSize = new Size(350, 0), Margin = new Padding(0, 6, 0, 2) };
        diagnosticLayout.Controls.Add(_aqLabel, 0, 1);
        diagnosticLayout.Controls.Add(_aqTextBox, 1, 1);
        diagnosticLayout.Controls.Add(_hrrLabel, 0, 2);
        diagnosticLayout.Controls.Add(_hrrCombo, 1, 2);
        diagnosticLayout.Controls.Add(_diagnosticHint, 0, 3);
        diagnosticLayout.SetColumnSpan(_diagnosticHint, 2);
        _diagnosticGroup.Controls.Add(diagnosticLayout);
        root.Controls.Add(_diagnosticGroup);
        _diagnosticKnownCheck.CheckedChanged += (_, _) => { UpdateDiagnosticInputs(); ApplyCurrentSettings(); };
        _aqTextBox.TextChanged += (_, _) => ApplyCurrentSettings();
        _hrrCombo.SelectedIndexChanged += (_, _) => ApplyCurrentSettings();
        UpdateDiagnosticInputs();

        // Einzel-Intensitaetsregler
        _singlePanel = new Panel { AutoSize = true, Width = 340, Margin = new Padding(0, 12, 0, 0) };
        _singleLabel = new Label { Text = Localization.Intensity(_preferences.SingleIntensity), AutoSize = true };
        _singleSlider = new TrackBar { Minimum = 0, Maximum = 100, Value = _preferences.SingleIntensity, Width = 370, TickFrequency = 10 };
        _singleSlider.ValueChanged += (_, _) =>
        {
            _singleLabel.Text = Localization.Intensity(_singleSlider.Value);
            ApplyCurrentSettings();
        };
        _singlePanel.Controls.Add(_singleLabel);
        _singleLabel.Top = 0;
        _singleSlider.Top = 20;
        _singlePanel.Controls.Add(_singleSlider);
        _singlePanel.Height = 60;
        root.Controls.Add(_singlePanel);

        // Mixed-Modus: zwei getrennte Regler
        _mixedPanel = new Panel { AutoSize = true, Width = 340, Visible = false, Margin = new Padding(0, 12, 0, 0) };
        _rgLabel = new Label { Text = Localization.RedGreen(_preferences.RedGreenIntensity), AutoSize = true, Top = 0 };
        _rgSlider = new TrackBar { Minimum = 0, Maximum = 100, Value = _preferences.RedGreenIntensity, Width = 370, Top = 20 };
        _rgSlider.ValueChanged += (_, _) =>
        {
            _rgLabel.Text = Localization.RedGreen(_rgSlider.Value);
            ApplyCurrentSettings();
        };
        _byLabel = new Label { Text = Localization.BlueYellow(_preferences.BlueYellowIntensity), AutoSize = true, Top = 70 };
        _bySlider = new TrackBar { Minimum = 0, Maximum = 100, Value = _preferences.BlueYellowIntensity, Width = 370, Top = 90 };
        _bySlider.ValueChanged += (_, _) =>
        {
            _byLabel.Text = Localization.BlueYellow(_bySlider.Value);
            ApplyCurrentSettings();
        };
        _mixedPanel.Controls.Add(_rgLabel);
        _mixedPanel.Controls.Add(_rgSlider);
        _mixedPanel.Controls.Add(_byLabel);
        _mixedPanel.Controls.Add(_bySlider);
        _mixedPanel.Height = 130;
        root.Controls.Add(_mixedPanel);

        _startWithWindowCheck = new CheckBox { Text = Localization.StartWithWindows, Checked = _preferences.StartWithWindow, AutoSize = true, Margin = new Padding(0, 14, 0, 0) };
        root.Controls.Add(_startWithWindowCheck);

        _saveButton = new Button { Text = Localization.SaveSettings, AutoSize = true, Margin = new Padding(0, 10, 0, 0) };
        _saveButton.Click += (_, _) => SaveCurrentSettings();
        root.Controls.Add(_saveButton);

        _saveStatus = new Label { AutoSize = true, ForeColor = SystemColors.GrayText, Margin = new Padding(4, 8, 0, 0) };
        root.Controls.Add(_saveStatus);

        _resetButton = new Button { Text = Localization.Reset, AutoSize = true, Margin = new Padding(0, 10, 0, 0) };
        _resetButton.Click += (_, _) =>
        {
            _rbTritan.Checked = true;
            _singleSlider.Value = 0;
            _rgSlider.Value = 0;
            _bySlider.Value = 0;
            _enabledCheck.Checked = false;
        };
        root.Controls.Add(_resetButton);

        if (_preferences.TypeIndex is >= 0 and <= 3)
        {
            new[] { _rbProtan, _rbDeutan, _rbTritan, _rbMixed }[_preferences.TypeIndex].Checked = true;
        }
    }

    private Control BuildOverviewPanel()
    {
        var panel = new Panel { Dock = DockStyle.Fill, BackColor = Color.FromArgb(31, 48, 65), Padding = new Padding(24) };
        _overviewTitle = new Label { Text = Localization.OverviewTitle, AutoSize = true, Font = new Font(Font.FontFamily, 16, FontStyle.Bold), ForeColor = Color.FromArgb(31, 48, 65) };
        _overviewTitle.ForeColor = Color.White;
        _overviewText = new Label { Text = Localization.OverviewText, AutoSize = true, MaximumSize = new Size(260, 0), Top = 42, ForeColor = Color.FromArgb(205, 216, 225) };
        var graphHost = new Panel
        {
            Left = 24,
            Top = 122,
            Width = 300,
            Height = 300,
            Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right,
            Padding = new Padding(0, 0, 18, 0)
        };
        _curveView = new ColorCurveView { Dock = DockStyle.Fill };
        graphHost.Controls.Add(_curveView);
        _overviewTip = new Label { Text = Localization.OverviewTip, AutoSize = true, MaximumSize = new Size(260, 0), Top = 440, ForeColor = Color.FromArgb(170, 188, 201) };
        panel.Controls.Add(_overviewTitle);
        panel.Controls.Add(_overviewText);
        panel.Controls.Add(graphHost);
        panel.Controls.Add(_overviewTip);
        return panel;
    }

    private void SaveCurrentSettings()
    {
        _preferences.Enabled = _enabledCheck.Checked;
        _preferences.TypeIndex = _rbProtan.Checked ? 0 : _rbDeutan.Checked ? 1 : _rbTritan.Checked ? 2 : 3;
        _preferences.SingleIntensity = _singleSlider.Value;
        _preferences.RedGreenIntensity = _rgSlider.Value;
        _preferences.BlueYellowIntensity = _bySlider.Value;
        _preferences.StartWithWindow = _startWithWindowCheck.Checked;
        _preferences.DiagnosticValueKnown = _diagnosticKnownCheck.Checked;
        _preferences.DiagnosticAq = TryParseDiagnosticAq(_aqTextBox.Text, out var aq) ? aq : null;
        _preferences.DiagnosticHrrLevel = Math.Max(0, _hrrCombo.SelectedIndex);
        _preferences.Save();
        _saveStatus.Text = Localization.SettingsSaved;
    }

    private static bool TryParseDiagnosticAq(string text, out double value)
    {
        var normalized = text.Trim().Replace(',', '.');
        return double.TryParse(normalized, System.Globalization.NumberStyles.Float, System.Globalization.CultureInfo.InvariantCulture, out value);
    }

    private void ApplyLanguage()
    {
        _enabledCheck.Text = Localization.Enabled;
        _hotkeyHint.Text = Localization.HotkeyHint;
        _typeGroup.Text = Localization.Type;
        _singleLabel.Text = Localization.Intensity(_singleSlider.Value);
        _rgLabel.Text = Localization.RedGreen(_rgSlider.Value);
        _byLabel.Text = Localization.BlueYellow(_bySlider.Value);
        _resetButton.Text = Localization.Reset;
        _startWithWindowCheck.Text = Localization.StartWithWindows;
        _saveButton.Text = Localization.SaveSettings;
        _overviewTitle.Text = Localization.OverviewTitle;
        _overviewText.Text = Localization.OverviewText;
        _overviewTip.Text = Localization.OverviewTip;
        _diagnosticKnownCheck.Text = Localization.DiagnosticValueKnown;
        _aqLabel.Text = Localization.AqValue;
        _hrrLabel.Text = Localization.HrrLevel;
        _diagnosticHint.Text = Localization.DiagnosticHint;
        var hrrIndex = _hrrCombo.SelectedIndex;
        _hrrCombo.Items.Clear();
        _hrrCombo.Items.AddRange(Localization.HrrLevels);
        _hrrCombo.SelectedIndex = Math.Clamp(hrrIndex, 0, 4);
        _openMenuItem.Text = Localization.Open;
        _exitMenuItem.Text = Localization.Exit;
        _trayIcon.Text = "Colorblind Assist";

        if (_languageCombo.Parent?.Controls[0] is Label languageLabel)
        {
            languageLabel.Text = Localization.Language + ":";
        }
    }

    private void OnTypeChanged(object? sender, EventArgs e)
    {
        if (sender is RadioButton { Checked: false }) return;
        bool mixed = _rbMixed.Checked;
        _singlePanel.Visible = !mixed;
        _mixedPanel.Visible = mixed;
        UpdateDiagnosticInputs();
        ApplyCurrentSettings();
    }

    private void UpdateDiagnosticInputs()
    {
        if (_diagnosticKnownCheck is null) return;
        var typeIndex = _rbProtan.Checked ? 0 : _rbDeutan.Checked ? 1 : _rbTritan.Checked ? 2 : 3;
        var enabled = _diagnosticKnownCheck.Checked;
        _aqLabel.Visible = enabled && (typeIndex == 0 || typeIndex == 1);
        _aqTextBox.Visible = _aqLabel.Visible;
        _hrrLabel.Visible = enabled;
        _hrrCombo.Visible = enabled;
        _diagnosticHint.Visible = enabled;
        _aqTextBox.Enabled = _aqLabel.Visible;
        _hrrCombo.Enabled = enabled;
    }

    private void ApplyCurrentSettings()
    {
        if (!_enabledCheck.Checked)
        {
            _controller.Clear();
            _curveView.SetMatrix(new double[,] { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } });
            return;
        }

        double[,] matrix;
        var typeIndex = _rbProtan.Checked ? 0 : _rbDeutan.Checked ? 1 : _rbTritan.Checked ? 2 : 3;
        double? diagnosticSeverity = null;
        if (_diagnosticKnownCheck is not null && _diagnosticKnownCheck.Checked)
        {
            double? aq = TryParseDiagnosticAq(_aqTextBox.Text, out var parsedAq)
                ? parsedAq
                : null;
            diagnosticSeverity = DiagnosticMapper.ResolveSeverity(typeIndex, aq, (HrrLevel)Math.Max(0, _hrrCombo.SelectedIndex));
        }
        if (_rbMixed.Checked)
        {
            matrix = diagnosticSeverity is double severity
                ? ColorMatrix.MixedCorrectionMatrix(severity, severity)
                : ColorMatrix.MixedCorrectionMatrix(_rgSlider.Value / 100.0, _bySlider.Value / 100.0);
        }
        else
        {
            var type = _rbProtan.Checked ? DeficiencyType.Protan
                     : _rbDeutan.Checked ? DeficiencyType.Deutan
                     : DeficiencyType.Tritan;
            matrix = ColorMatrix.CorrectionMatrix(type, diagnosticSeverity ?? _singleSlider.Value / 100.0);
        }

        var effect = ColorMatrix.ToMagColorEffect(matrix);
        _curveView.SetMatrix(matrix);
        if (!_controller.Apply(effect))
        {
            _saveStatus.Text = Localization.FilterApplyFailed;
        }
    }
}

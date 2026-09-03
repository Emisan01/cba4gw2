using System.Runtime.InteropServices;

namespace ColorblindAssist;

// 5x5-Farbtransformationsmatrix, wie sie die Magnification API erwartet.
// Layout muss exakt dem nativen MAGCOLOREFFECT-Struct aus magnification.h entsprechen.
[StructLayout(LayoutKind.Sequential)]
public struct MagColorEffect
{
    public float M00, M01, M02, M03, M04;
    public float M10, M11, M12, M13, M14;
    public float M20, M21, M22, M23, M24;
    public float M30, M31, M32, M33, M34;
    public float M40, M41, M42, M43, M44;

    public static MagColorEffect Identity() => new()
    {
        M00 = 1f,
        M11 = 1f,
        M22 = 1f,
        M33 = 1f,
        M44 = 1f
    };
}

// Dünner Wrapper um magnification.dll. Nur die drei Funktionen, die wir
// tatsächlich brauchen, um systemweit eine Farbmatrix zu setzen bzw. zu loeschen.
internal static class NativeMagnification
{
    [DllImport("Magnification.dll", SetLastError = true)]
    public static extern bool MagInitialize();

    [DllImport("Magnification.dll", SetLastError = true)]
    public static extern bool MagUninitialize();

    // Setzt den Vollbild-Farbeffekt fuer den gesamten Desktop (Windows 8+).
    // Das ist derselbe zugrunde liegende Mechanismus, den auch die
    // Bordmittel-Farbfilter unter Einstellungen > Eingabehilfen verwenden.
    [DllImport("Magnification.dll", SetLastError = true)]
    public static extern bool MagSetFullscreenColorEffect(ref MagColorEffect effect);

    [DllImport("Magnification.dll", SetLastError = true)]
    public static extern bool MagGetFullscreenColorEffect(out MagColorEffect effect);
}

// Hoeherwertiger Controller: kapselt Init/Uninit-Lebenszyklus und stellt
// sicher, dass wir beim Beenden immer auf Identity zuruecksetzen, statt den
// Bildschirm verfaerbt zu hinterlassen.
public sealed class ColorEffectController : IDisposable
{
    private bool _initialized;

    public bool Initialize()
    {
        _initialized = NativeMagnification.MagInitialize();
        return _initialized;
    }

    public bool Apply(MagColorEffect effect)
    {
        if (!_initialized) return false;
        return NativeMagnification.MagSetFullscreenColorEffect(ref effect);
    }

    public bool Clear()
    {
        if (!_initialized) return false;
        var identity = MagColorEffect.Identity();
        return NativeMagnification.MagSetFullscreenColorEffect(ref identity);
    }

    public void Dispose()
    {
        if (_initialized)
        {
            Clear();
            NativeMagnification.MagUninitialize();
            _initialized = false;
        }
    }
}

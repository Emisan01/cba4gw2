using System.Drawing.Drawing2D;

namespace ColorblindAssist;

internal sealed class ColorCurveView : Panel
{
    private double[,] _matrix = Identity();

    public ColorCurveView()
    {
        DoubleBuffered = true;
        BackColor = Color.FromArgb(31, 48, 65);
        MinimumSize = new Size(250, 250);
        Resize += (_, _) => Invalidate();
    }

    public void SetMatrix(double[,] matrix)
    {
        _matrix = matrix;
        Invalidate();
    }

    protected override void OnPaint(PaintEventArgs e)
    {
        base.OnPaint(e);
        e.Graphics.SmoothingMode = SmoothingMode.AntiAlias;

        var plot = new Rectangle(42, 20, Math.Max(80, ClientSize.Width - 62), Math.Max(100, ClientSize.Height - 58));
        using var gridPen = new Pen(Color.FromArgb(65, 83, 99));
        using var axisPen = new Pen(Color.FromArgb(150, 166, 180));
        using var baselinePen = new Pen(Color.FromArgb(225, 235, 242), 1.6f) { DashStyle = DashStyle.Dash };
        using var redPen = new Pen(Color.FromArgb(255, 108, 108), 2.4f);
        using var greenPen = new Pen(Color.FromArgb(91, 220, 151), 2.4f);
        using var bluePen = new Pen(Color.FromArgb(102, 173, 255), 2.4f);
        using var labelBrush = new SolidBrush(Color.FromArgb(205, 216, 225));
        using var smallFont = new Font(Font.FontFamily, 8f);

        for (var i = 0; i <= 4; i++)
        {
            var x = plot.Left + plot.Width * i / 4;
            var y = plot.Bottom - plot.Height * i / 4;
            e.Graphics.DrawLine(gridPen, x, plot.Top, x, plot.Bottom);
            e.Graphics.DrawLine(gridPen, plot.Left, y, plot.Right, y);
        }

        e.Graphics.DrawLine(axisPen, plot.Left, plot.Bottom, plot.Right, plot.Bottom);
        e.Graphics.DrawLine(axisPen, plot.Left, plot.Top, plot.Left, plot.Bottom);
        e.Graphics.DrawString("input", smallFont, labelBrush, plot.Right - 32, plot.Bottom + 8);
        e.Graphics.DrawString("output", smallFont, labelBrush, 3, plot.Top - 14);

        DrawBaseline(e.Graphics, plot, baselinePen);
        DrawCurve(e.Graphics, plot, 0, redPen);
        DrawCurve(e.Graphics, plot, 1, greenPen);
        DrawCurve(e.Graphics, plot, 2, bluePen);

        var legendY = ClientSize.Height - 22;
        e.Graphics.DrawString("R", smallFont, redPen.Brush, plot.Left, legendY);
        e.Graphics.DrawString("G", smallFont, greenPen.Brush, plot.Left + 24, legendY);
        e.Graphics.DrawString("B", smallFont, bluePen.Brush, plot.Left + 48, legendY);
    }

    private void DrawCurve(Graphics graphics, Rectangle plot, int row, Pen pen)
    {
        using var path = new GraphicsPath();
        const int steps = 32;
        // Sum the transformed channel weights so the identity matrix produces
        // the same y=x line for R, G, and B.
        var outputWeight = _matrix[0, row] + _matrix[1, row] + _matrix[2, row];
        PointF previous = default;
        for (var i = 0; i <= steps; i++)
        {
            var input = i / (double)steps;
            var output = input * outputWeight;
            output = Math.Clamp(output, 0, 1);
            var x = plot.Left + (float)(input * plot.Width);
            var y = plot.Bottom - (float)(output * plot.Height);
            var current = new PointF(x, y);
            if (i == 0)
            {
                path.StartFigure();
            }
            else
            {
                path.AddLine(previous, current);
            }
            previous = current;
        }
        graphics.DrawPath(pen, path);
    }

    private static void DrawBaseline(Graphics graphics, Rectangle plot, Pen pen)
    {
        graphics.DrawLine(pen, plot.Left, plot.Bottom, plot.Right, plot.Top);
    }

    private static double[,] Identity() => new double[,]
    {
        { 1d, 0d, 0d },
        { 0d, 1d, 0d },
        { 0d, 0d, 1d }
    };
}

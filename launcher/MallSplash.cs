// Launch screen: splash.png floating over the desktop with per-pixel transparency (a layered
// window), plus a status line drawn under it. play.ps1 compiles this once into ..\logs.
using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Text;
using System.Runtime.InteropServices;
using System.Windows.Forms;

public class MallSplash : Form
{
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    struct BLEND { public byte Op, Flags, Alpha, Format; }
    [DllImport("user32.dll")] static extern bool UpdateLayeredWindow(IntPtr hwnd, IntPtr hdcDst, ref Point dst, ref Size size,
        IntPtr hdcSrc, ref Point src, int key, ref BLEND blend, int flags);
    [DllImport("user32.dll")] static extern IntPtr GetDC(IntPtr hwnd);
    [DllImport("user32.dll")] static extern int ReleaseDC(IntPtr hwnd, IntPtr dc);
    [DllImport("gdi32.dll")] static extern IntPtr CreateCompatibleDC(IntPtr dc);
    [DllImport("gdi32.dll")] static extern bool DeleteDC(IntPtr dc);
    [DllImport("gdi32.dll")] static extern IntPtr SelectObject(IntPtr dc, IntPtr obj);
    [DllImport("gdi32.dll")] static extern bool DeleteObject(IntPtr obj);

    readonly Bitmap art;

    public MallSplash(string png)
    {
        using (Bitmap file = new Bitmap(png)) art = new Bitmap(file);   // a copy: don't lock splash.png (the updater replaces it)
        FormBorderStyle = FormBorderStyle.None;
        ShowInTaskbar = false;
        TopMost = true;
        StartPosition = FormStartPosition.Manual;
        Rectangle s = Screen.PrimaryScreen.WorkingArea;
        Location = new Point(s.X + (s.Width - art.Width) / 2, s.Y + (s.Height - art.Height) / 2);
        Size = art.Size;
    }

    protected override CreateParams CreateParams
    {
        get { CreateParams cp = base.CreateParams; cp.ExStyle |= 0x80000 | 0x80; return cp; }   // LAYERED | TOOLWINDOW
    }

    public void SetStatus(string text, Color color)
    {
        using (Bitmap bmp = new Bitmap(art))
        {
            using (Graphics g = Graphics.FromImage(bmp))
            using (GraphicsPath path = new GraphicsPath())
            using (StringFormat sf = new StringFormat { Alignment = StringAlignment.Center })
            using (Pen pen = new Pen(Color.FromArgb(29, 36, 48), 5) { LineJoin = LineJoin.Round })
            using (SolidBrush brush = new SolidBrush(color))
            {
                g.SmoothingMode = SmoothingMode.AntiAlias;
                g.TextRenderingHint = TextRenderingHint.AntiAlias;
                path.AddString(text, new FontFamily("Segoe UI"), (int)FontStyle.Bold, 24,
                               new RectangleF(0, art.Height - 44, art.Width, 40), sf);
                g.DrawPath(pen, path);
                g.FillPath(brush, path);
            }
            Apply(bmp);
        }
    }

    void Apply(Bitmap bmp)
    {
        IntPtr screen = GetDC(IntPtr.Zero), mem = CreateCompatibleDC(screen);
        IntPtr hb = bmp.GetHbitmap(Color.FromArgb(0)), old = SelectObject(mem, hb);
        Point dst = Location, src = Point.Empty;
        Size size = bmp.Size;
        BLEND blend = new BLEND { Op = 0, Flags = 0, Alpha = 255, Format = 1 };   // per-pixel alpha
        UpdateLayeredWindow(Handle, screen, ref dst, ref size, mem, ref src, 0, ref blend, 2);
        SelectObject(mem, old); DeleteObject(hb); DeleteDC(mem); ReleaseDC(IntPtr.Zero, screen);
    }
}

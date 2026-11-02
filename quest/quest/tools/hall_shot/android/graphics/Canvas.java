package android.graphics;
import java.awt.*;
import java.awt.geom.*;
import java.util.ArrayDeque;
public class Canvas {
    final Graphics2D g;
    final ArrayDeque<Object[]> stack = new ArrayDeque<>();
    public Canvas(Bitmap b) {
        g = b.img.createGraphics();
        g.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
        g.setRenderingHint(RenderingHints.KEY_TEXT_ANTIALIASING, RenderingHints.VALUE_TEXT_ANTIALIAS_ON);
        g.setRenderingHint(RenderingHints.KEY_FRACTIONALMETRICS, RenderingHints.VALUE_FRACTIONALMETRICS_ON);
        g.setRenderingHint(RenderingHints.KEY_INTERPOLATION, RenderingHints.VALUE_INTERPOLATION_BILINEAR);
        g.setRenderingHint(RenderingHints.KEY_STROKE_CONTROL, RenderingHints.VALUE_STROKE_PURE);
    }
    public int save() { stack.push(new Object[] { g.getTransform(), g.getClip() }); return stack.size(); }
    public void restore() { Object[] s = stack.pop(); g.setTransform((AffineTransform) s[0]); g.setClip((Shape) s[1]); }
    public void translate(float x, float y) { g.translate(x, y); }
    public void scale(float x, float y) { g.scale(x, y); }
    public void rotate(float deg) { g.rotate(Math.toRadians(deg)); }
    public boolean clipPath(Path p) { g.clip(p.p); return true; }
    void paint(Paint p) {
        if (p.shader != null) g.setPaint(p.shader.awt()); else g.setColor(new java.awt.Color(p.color, true));
        g.setStroke(new BasicStroke(p.stroke));
    }
    void shape(Shape s, Paint p) { paint(p); if (p.style == Paint.Style.STROKE) g.draw(s); else g.fill(s); }
    public void drawRect(RectF r, Paint p) { drawRect(r.left, r.top, r.right, r.bottom, p); }
    public void drawRect(float l, float t, float r, float b, Paint p) { shape(new Rectangle2D.Float(l, t, r - l, b - t), p); }
    public void drawRoundRect(RectF r, float rx, float ry, Paint p) { shape(new RoundRectangle2D.Float(r.left, r.top, r.width(), r.height(), rx * 2, ry * 2), p); }
    public void drawOval(RectF r, Paint p) { shape(new Ellipse2D.Float(r.left, r.top, r.width(), r.height()), p); }
    public void drawPath(Path path, Paint p) { shape(path.p, p); }
    public void drawLine(float x0, float y0, float x1, float y1, Paint p) { paint(p); g.draw(new Line2D.Float(x0, y0, x1, y1)); }
    public void drawText(String s, float x, float y, Paint p) { paint(p); g.setFont(p.font()); g.drawString(s, x, y); }
    public void drawBitmap(Bitmap b, Rect src, RectF dst, Paint p) {
        int sx = src == null ? 0 : src.left, sy = src == null ? 0 : src.top;
        int sw = src == null ? b.getWidth() : src.right - src.left, sh = src == null ? b.getHeight() : src.bottom - src.top;
        AffineTransform t = g.getTransform();
        g.translate(dst.left, dst.top); g.scale(dst.width() / sw, dst.height() / sh);
        g.drawImage(b.img, 0, 0, sw, sh, sx, sy, sx + sw, sy + sh, null);
        g.setTransform(t);
    }
}

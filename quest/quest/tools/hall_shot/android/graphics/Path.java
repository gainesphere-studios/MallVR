package android.graphics;
public class Path {
    public enum Direction { CW }
    final java.awt.geom.Path2D.Float p = new java.awt.geom.Path2D.Float();
    public void moveTo(float x, float y) { p.moveTo(x, y); }
    public void lineTo(float x, float y) { p.lineTo(x, y); }
    public void close() { p.closePath(); }
    public void addOval(RectF r, Direction d) { p.append(new java.awt.geom.Ellipse2D.Float(r.left, r.top, r.width(), r.height()), false); }
}

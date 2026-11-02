package android.graphics;
public class LinearGradient extends Shader {
    final float x0, y0, x1, y1; final int a, b;
    public LinearGradient(float x0, float y0, float x1, float y1, int a, int b, TileMode m) { this.x0 = x0; this.y0 = y0; this.x1 = x1; this.y1 = y1; this.a = a; this.b = b; }
    java.awt.Paint awt() { return new java.awt.GradientPaint(x0, y0, c(a), x1, y1, c(b)); }
}

package android.graphics;
public class RadialGradient extends Shader {
    final float cx, cy, r; final int[] cols; final float[] pos;
    public RadialGradient(float cx, float cy, float r, int[] cols, float[] pos, TileMode m) { this.cx = cx; this.cy = cy; this.r = r; this.cols = cols; this.pos = pos; }
    java.awt.Paint awt() {
        java.awt.Color[] cs = new java.awt.Color[cols.length];
        for (int i = 0; i < cs.length; i++) cs[i] = c(cols[i]);
        return new java.awt.RadialGradientPaint(cx, cy, r, pos, cs);
    }
}

package android.graphics;
public class Paint {
    public static final int ANTI_ALIAS_FLAG = 1, FILTER_BITMAP_FLAG = 2;
    public enum Style { FILL, STROKE }
    Style style = Style.FILL; int color = 0xFF000000; float stroke = 0, size = 12; Typeface face = Typeface.SANS_SERIF; Shader shader;
    java.awt.Font font;
    public Paint() { }
    public Paint(int flags) { }
    public void setStyle(Style s) { style = s; }
    public void setColor(int c) { color = c; }
    public void setStrokeWidth(float w) { stroke = w; }
    public void setTextSize(float s) { size = s; font = null; }
    public void setTypeface(Typeface t) { face = t; font = null; }
    public void setSubpixelText(boolean b) { }
    public Shader setShader(Shader s) { shader = s; return s; }
    java.awt.Font font() {
        if (font == null) font = new java.awt.Font(face.family, face.style == Typeface.BOLD ? java.awt.Font.BOLD : java.awt.Font.PLAIN, 100).deriveFont(size);
        return font;
    }
    static final java.awt.font.FontRenderContext FRC = new java.awt.font.FontRenderContext(null, true, true);
    public float measureText(String s) { return (float) font().getStringBounds(s, FRC).getWidth(); }
}

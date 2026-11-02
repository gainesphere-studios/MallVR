package android.graphics;
public abstract class Shader {
    public enum TileMode { CLAMP }
    abstract java.awt.Paint awt();
    static java.awt.Color c(int v) { return new java.awt.Color(v, true); }
}

package android.graphics;
import java.awt.image.BufferedImage;
public class BitmapFactory {
    public static class Options { public boolean inMutable; public Bitmap.Config inPreferredConfig; }
    public static Bitmap decodeStream(java.io.InputStream in) { return decodeStream(in, null, null); }
    public static Bitmap decodeStream(java.io.InputStream in, Rect pad, Options o) {
        try {
            BufferedImage s = javax.imageio.ImageIO.read(in);
            if (s == null) return null;
            Bitmap b = Bitmap.createBitmap(s.getWidth(), s.getHeight(), Bitmap.Config.ARGB_8888);
            b.img.getGraphics().drawImage(s, 0, 0, null);
            return b;
        } catch (java.io.IOException e) { return null; }
    }
}

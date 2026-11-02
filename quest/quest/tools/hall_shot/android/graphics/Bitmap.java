package android.graphics;
import java.awt.image.BufferedImage;
public class Bitmap {
    public enum Config { ARGB_8888 }
    final BufferedImage img;
    Bitmap(BufferedImage i) { img = i; }
    public static Bitmap createBitmap(int w, int h, Config c) { return new Bitmap(new BufferedImage(w, h, BufferedImage.TYPE_INT_ARGB)); }
    public int getWidth() { return img.getWidth(); }
    public int getHeight() { return img.getHeight(); }
    public boolean isMutable() { return true; }
    public Config getConfig() { return Config.ARGB_8888; }
    public Bitmap copy(Config c, boolean m) { Bitmap b = createBitmap(getWidth(), getHeight(), c); b.img.getGraphics().drawImage(img, 0, 0, null); return b; }
    public void getPixels(int[] px, int off, int stride, int x, int y, int w, int h) { img.getRGB(x, y, w, h, px, off, stride); }
    public void setPixels(int[] px, int off, int stride, int x, int y, int w, int h) { img.setRGB(x, y, w, h, px, off, stride); }
    public void setHasAlpha(boolean a) { }
    public boolean compress(Object fmt, int q, java.io.OutputStream o) { try { return javax.imageio.ImageIO.write(img, "png", o); } catch (java.io.IOException e) { return false; } }
}

import android.graphics.*;
import com.gainesphere.mallvr.HallShotAccess;

/* hall_shot: draw the Quest launcher's Cyberspace Hall (HallPainter) to a PNG on the PC.
     java HallShot <hall dir> <out.png> <width> <height> [wanted] [machine] [nofiles] [store] [hover]   (see hall_shot.sh) */
public class HallShot {
    public static void main(String[] a) throws Exception {
        java.util.Set<String> opt = new java.util.HashSet<>(java.util.Arrays.asList(a).subList(4, a.length));
        Bitmap b = Bitmap.createBitmap(Integer.parseInt(a[2]), Integer.parseInt(a[3]), Bitmap.Config.ARGB_8888);
        HallShotAccess.draw(new java.io.File(a[0]), new Canvas(b), b.getWidth(), b.getHeight(), opt);
        try (java.io.FileOutputStream o = new java.io.FileOutputStream(a[1])) { b.compress(null, 100, o); }
    }
}

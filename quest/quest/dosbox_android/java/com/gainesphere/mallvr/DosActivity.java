package com.gainesphere.mallvr;

import android.os.Bundle;
import org.libsdl.app.SDLActivity;
import java.io.File;

/* MallVR Quest proof 2: stock DOSBox-X in a 2D window. C: = this app's own folder on the headset
   (Android/data/com.gainesphere.mallvr.dos/files, reachable over USB / SideQuest, no permission needed).
   HOME = app files dir, so DOSBox-X finds a writable place for its config. */
public class DosActivity extends SDLActivity {
    @Override protected void onCreate(Bundle b) {
        try {
            android.system.Os.setenv("HOME", getFilesDir().getAbsolutePath(), true);
            android.system.Os.setenv("SDL_HINT_ANDROID_TRAP_BACK_BUTTON", "1", true);
        } catch (Exception e) { }
        super.onCreate(b);
    }
    @Override protected String[] getArguments() {
        File c = getExternalFilesDir(null);
        if (c != null) c.mkdirs();
        String dir = c != null ? c.getAbsolutePath() : getFilesDir().getAbsolutePath();
        return new String[] { "-defaultconf", "-nopromptfolder", "-set", "cpu core=normal",
            "-c", "mount c \"" + dir + "\"", "-c", "c:", "-c", "ver", "-c", "dir" };
    }
}

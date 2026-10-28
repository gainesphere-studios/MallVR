package com.gainesphere.mallvr;

import android.app.Activity;
import android.graphics.Typeface;
import android.os.Bundle;
import android.widget.*;

/* ABOUT / LICENCES (plan section 9): credits, the GPL source offer, and every bundled library's licence text (assets/licenses). */
public class AboutActivity extends Activity {
    @Override protected void onCreate(Bundle b) {
        super.onCreate(b);
        LinearLayout root = LauncherActivity.col(this); root.setBackgroundColor(LauncherActivity.BG);
        int p = (int) (24 * getResources().getDisplayMetrics().density); root.setPadding(p, p / 2, p, p / 2);
        TextView t = LauncherActivity.text(this, "ABOUT MALLVR " + MallVR.version(this), 28, LauncherActivity.GOLD); t.setTypeface(Typeface.DEFAULT_BOLD); root.addView(t);
        StringBuilder s = new StringBuilder();
        s.append("MallVR plays the 1990s Virtuality arcade VR games (SU2000 / SU3000) on a modified DOSBox-X. A Gainesphere project: gainesphere.com/pages/mallvr\n\n")
         .append("THANKS\nJeremy Shartzer for Buggy Ball. ErrorDan for the 3000-series games (Total Recoil, Zero Hour, Zeneca), newer versions of older games and the Zone Hunter music. ")
         .append("nyxxic and Faulty for the original SU2000 dump, and Simon Marston for the SU2000 drive that started it all. LeonG-ZA for his SU2000 emulator work (su2000 card code, GPL-2.0).\n\n")
         .append("MallVR ships no game files: you bring your own (GET GAMES downloads the community's archive.org copies).\n\n")
         .append("FREE SOFTWARE\nThis app is a modified DOSBox-X, so the whole app is under the GNU General Public License (version 2 or later). You may copy, change and share it under that licence. ")
         .append("Its complete source code (our changes, the Android project and build scripts) is MallVR-Quest-").append(MallVR.version(this))
         .append("-source.zip, posted next to this APK on gainesphere.com/pages/mallvr-download. Can't find it? Ask in the MallVR Discord and we'll send it.\n\n")
         .append("Inside: DOSBox-X (GPL-2.0+), SDL2 (zlib), gl4es (MIT), the Khronos OpenXR loader (Apache-2.0), Apache Commons Compress (Apache-2.0), XZ for Java (public domain). ")
         .append("Their licence texts follow.\n");
        try {
            for (String f : getAssets().list("licenses"))
                s.append("\n\n==================== ").append(f).append(" ====================\n\n").append(MallVR.readAll(getAssets().open("licenses/" + f)));
        } catch (Exception e) { s.append("\n(licence texts missing: ").append(e).append(")"); }
        TextView body = LauncherActivity.text(this, s.toString(), 14, LauncherActivity.CREAM); body.setTextIsSelectable(true);
        ScrollView sv = new ScrollView(this); sv.addView(body);
        root.addView(sv, new LinearLayout.LayoutParams(-1, 0, 1f));
        setContentView(root);
    }
}

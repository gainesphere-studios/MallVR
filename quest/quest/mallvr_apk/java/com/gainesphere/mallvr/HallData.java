package com.gainesphere.mallvr;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import java.io.*;
import java.util.*;
import java.util.concurrent.ConcurrentHashMap;

/* The Cyberspace Hall's files (the PC's HallData in MallHall.cs): pod turnarounds, the mark, the pillbug, the exhibit text, the news.
   Read from the APK's assets/hall (copied from package/engine/hall by build_mallvr_apk.sh). Plain android.graphics + java.io only,
   so quest/tools/hall_shot can draw the hall on the PC. */
final class HallData {
    static final String NEWS_URL = "https://downloads.gainesphere.com/mallvr/news.txt";
    interface Source { InputStream open(String name) throws IOException; }

    final Source src;
    final File cache;                                 // news_cache.txt: the server's last news
    final Map<String, Map<String, String>> info = new HashMap<>();
    final Map<String, Bitmap> pods = new ConcurrentHashMap<>();
    Bitmap logo, bug;
    volatile String newsTitle = "", newsText = "", newsLink = "";

    HallData(Source src, File cache) {
        this.src = src; this.cache = cache;
        logo = image("logo.png"); bug = image("pillbug.png");
        parse(read("hall_info.txt"));
        // news: the copy saved last time (or the one shipped); fetchNews() then asks the server
        String c = "";
        try { if (cache != null && cache.isFile()) c = readAll(new FileInputStream(cache)); } catch (IOException e) { }
        setNews(c.contains("title=") ? c : read("news.txt"));
    }

    String get(String section, String key) {
        Map<String, String> s = info.get(section);
        String v = s == null ? null : s.get(key);
        return v == null ? "" : v;
    }

    /* the pod's turnaround once loadPods() has it, else null (the hall shows the pod's name meanwhile) */
    Bitmap pod(String code) { return pods.get(code); }

    /* on a worker thread, like the PC's Preload while the intro plays */
    void loadPods(String[] codes) {
        for (String c : codes) {
            if (pods.containsKey(c)) continue;
            try {
                BitmapFactory.Options o = new BitmapFactory.Options();
                o.inMutable = true; o.inPreferredConfig = Bitmap.Config.ARGB_8888;
                InputStream in = src.open("pod_" + c + ".jpg");
                Bitmap b = BitmapFactory.decodeStream(in, null, o);
                in.close();
                if (b == null) continue;
                if (!b.isMutable() || b.getConfig() != Bitmap.Config.ARGB_8888) b = b.copy(Bitmap.Config.ARGB_8888, true);
                feather(b, b.getWidth() / 10, b.getHeight() / 6);
                pods.put(c, b);
            } catch (Throwable e) { }
        }
    }

    /* 60 frames, 10 x 6, white studio; each frame's edge fades out so it melts into the spotlight (the PC's Feather) */
    static void feather(Bitmap b, int tw, int th) {
        float cx = tw / 2f, cy = th * 0.52f, r1 = tw * 0.5f, r0 = tw * 0.36f;
        int[] a = new int[tw * th];
        for (int y = 0; y < th; y++)
            for (int x = 0; x < tw; x++) {
                float dx = x - cx, dy = (y - cy) * 1.05f, d = (float) Math.sqrt(dx * dx + dy * dy);
                a[y * tw + x] = (int) (255 * Math.max(0, Math.min(1, (r1 - d) / (r1 - r0))));
            }
        int w = b.getWidth(); int[] row = new int[w];
        for (int y = 0; y < b.getHeight(); y++) {
            b.getPixels(row, 0, w, 0, y, w, 1);
            int ay = (y % th) * tw;
            for (int x = 0; x < w; x++) row[x] = (row[x] & 0xFFFFFF) | (a[ay + x % tw] << 24);
            b.setPixels(row, 0, w, 0, y, w, 1);
        }
        b.setHasAlpha(true);                          // a JPEG decodes as opaque: without this the feather is ignored
    }

    /* the server's news (worker thread). Saved for next time; offline = the saved news stays. */
    void fetchNews() {
        try {
            java.net.HttpURLConnection c = (java.net.HttpURLConnection) new java.net.URL(NEWS_URL + "?v="
                + new java.text.SimpleDateFormat("yyyyMMddHH", Locale.US).format(new Date())).openConnection();
            c.setConnectTimeout(5000); c.setReadTimeout(5000); c.setRequestProperty("User-Agent", "MallVR");
            if (c.getResponseCode() != 200) { c.disconnect(); return; }
            String s = readAll(c.getInputStream());
            c.disconnect();
            if (!s.contains("title=") || s.length() >= 4000) return;
            setNews(s);
            if (cache != null) { Writer w = new OutputStreamWriter(new FileOutputStream(cache), "UTF-8"); w.write(s); w.close(); }
        } catch (Exception e) { }
    }

    void setNews(String s) {
        Map<String, String> kv = new HashMap<>();
        for (String line : s.split("\n")) {
            int e = line.indexOf('=');
            if (e <= 0 || line.trim().startsWith("#")) continue;
            kv.put(line.substring(0, e).trim().toLowerCase(Locale.US), line.substring(e + 1).trim());
        }
        String t = kv.get("title"), x = kv.get("text"), l = kv.get("link");
        newsTitle = t == null ? "" : t.toUpperCase(Locale.US);
        newsText = x == null ? "" : x;
        newsLink = l == null ? "" : l;
    }

    void parse(String s) {
        Map<String, String> cur = null;
        for (String raw : s.split("\n")) {
            String line = raw.trim();
            if (line.isEmpty() || line.startsWith("#")) continue;
            if (line.startsWith("[") && line.endsWith("]")) { cur = new HashMap<>(); info.put(line.substring(1, line.length() - 1).trim(), cur); continue; }
            int e = line.indexOf('=');
            if (cur != null && e > 0) cur.put(line.substring(0, e).trim(), line.substring(e + 1).trim());
        }
    }

    Bitmap image(String name) {
        try { InputStream in = src.open(name); Bitmap b = BitmapFactory.decodeStream(in); in.close(); return b; } catch (Exception e) { return null; }
    }

    String read(String name) { try { return readAll(src.open(name)); } catch (Exception e) { return ""; } }

    static String readAll(InputStream in) throws IOException {
        ByteArrayOutputStream o = new ByteArrayOutputStream(); byte[] b = new byte[8192]; int n;
        while ((n = in.read(b)) > 0) o.write(b, 0, n);
        in.close();
        return o.toString("UTF-8").replace("\r", "");
    }
}

import com.gainesphere.mallvr.MallOnline;
import java.io.File;
import java.util.*;

/* Crossplay test without a headset: the Quest's online code (quest/mallvr_apk/.../MallOnline.java) runs on the PC's JVM as a "QUEST"
   player, against a PC launcher on the same list (online/list_mock.py, or the live list.php). When the group starts it runs a real
   pod with play.ps1 of a MallVR install (window mode), the way the Quest app starts its ":game" process, and relays its frames.
   A group switch (the host's SWITCH GAME) closes that pod and starts the next game, as OnlineActivity does.
     java -cp <classes> OnlineTest <list url> host|join <GAME> <MallVR install dir> [name] [nolan]
   nolan = no LAN shouts (forces the list's ADDR path, as from another house). Env: TEST_SECS (default 90), TEST_HAVE="DAC GHOST"
   (games this player has; default = GAME), TEST_BAD_SHA=1 (claim other game files: the host must refuse). */
public class OnlineTest {
    static Process game;

    public static void main(String[] a) throws Exception {
        String url = a[0], mode = a[1], first = a[2], dir = a[3], name = a.length > 4 ? a[4] : "QUESTBOT";
        boolean nolan = a.length > 5 && a[5].equals("nolan");
        MallOnline n = new MallOnline();
        n.listUrl = url; n.name = name; n.version = "0.2-quest"; n.key = MallOnline.newKey(); n.platform = "QUEST";
        n.lanFind = !nolan;
        Map<String, String> sha = new HashMap<>();
        sha.put("DAC", "5151a6d58b95a74fb86cb0dbd53a36d67eef7ce7"); sha.put("DN2", "110d8f51aa1fc9b41d197d6cb2fb35bc0e488bf2");
        sha.put("GHOST", "ce0a36969d418cd591c0e7f32f9e3a35c6748adc");
        if (System.getenv("TEST_BAD_SHA") != null) sha.put(first, "0000000000000000000000000000000000000000");
        n.sha1For.putAll(sha);
        for (String g : System.getenv().getOrDefault("TEST_HAVE", first).split(" ")) { n.have.add(g); n.maxFor.put(g, 2); }
        n.start();
        Thread.sleep(3500);
        if (mode.equals("host")) n.host(first, 2);
        if (mode.equals("quick")) { n.quickGames.addAll(n.have); n.startQuick(first.equals("ANY") ? "" : first); }   // QUICK MATCH (ANY = any game)
        boolean joined = false;
        long until = System.currentTimeMillis() + Long.parseLong(System.getenv().getOrDefault("TEST_SECS", "90")) * 1000;
        while (System.currentTimeMillis() < until) {
            if (mode.equals("join") && n.status.equals("LOBBY") && !joined)
                for (MallOnline.Entry e : n.everyone()) if (e.isHost() && e.game.equals(first) && !e.name.equals(name)) { System.out.println("joining " + e.name + " (" + e.plat + ", tok " + e.tok + ", lan " + e.fromLan + ")"); joined = true; n.join(e); break; }
            if (mode.equals("host") && n.status.equals("HOST") && n.roster().size() >= 2 && game == null) { System.out.println("START: " + n.roster() + " " + n.rosterPlat()); n.startMatch(); }
            if (n.goPod > 0 && game == null) run(n, n.game, n.goPod, dir);
            if (!n.switchGame.isEmpty()) {
                String g = n.switchGame; int pod = n.switchPod;
                System.out.println("SWITCH to " + g + " as pod " + pod);
                Process p = game; game = null;
                if (p != null) { kill(p); p.waitFor(); }
                n.switchTaken(); n.endGame();
                run(n, g, pod, dir);
            }
            StringBuilder b = new StringBuilder();
            for (MallOnline.Entry e : n.everyone()) b.append(e.name).append('/').append(e.status).append('/').append(e.plat).append(e.isHost() ? "/tok=" + e.tok : "").append("  ");
            System.out.println("[" + n.status + " " + n.game + "] note=" + n.note + " | " + b);
            Thread.sleep(2000);
        }
        if (game != null) kill(game);
        n.stop();
        System.exit(0);
    }

    static void run(MallOnline n, String g, int pod, String dir) throws Exception {
        System.out.println("game: " + g + " pod " + pod + ", roster " + n.roster() + " " + n.rosterPlat());
        n.beginGame(pod);
        ProcessBuilder pb = new ProcessBuilder("powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", new File(dir, "engine\\play.ps1").getPath(),
            g, "-Pod", "" + pod, "-Peers", "127.0.0.1:" + MallOnline.localPort(pod), "-Desktop");
        pb.inheritIO();
        Process p = pb.start();
        game = p;
        new Thread(() -> { try { p.waitFor(); } catch (InterruptedException e) { } if (game == p) { game = null; System.out.println("game ended"); n.afterGame(); } }).start();
    }

    static void kill(Process p) {
        try { new ProcessBuilder("taskkill", "/PID", "" + p.pid(), "/T", "/F").start().waitFor(); } catch (Exception e) { }
    }
}

package com.gainesphere.mallvr;

import java.io.*;
import java.net.*;
import java.nio.charset.StandardCharsets;
import java.security.SecureRandom;
import java.util.*;

/* MallVR online play on the Quest: package/engine/MallOnline.cs (the PC launcher's online code) in Java, packet for packet, so Quest
   and PC players are in the same list, the same groups and the same linked games (Quest plan section 7, crossplay).
   Plain Java (no Android classes), so it is tested on the PC's JVM against a real PC launcher before any APK goes out
   (quest/tools/OnlineTest.java). The Android parts (Wi-Fi multicast lock, keeping the process alive) are in OnlineService.java.
   Everything below matches MallOnline.cs; read that file's header for the packets:
     J join / W welcome / F refused / K keepalive / L roster / T platforms / G go / X leave / D data / P punch,
     groups: H have / N next? / A answer / S switch / E between games / C game picked / Q R ping / Y started.
   The list (list.php) keeps addresses private: rows carry a host token (tok); a joiner beats with call=<tok> and gets an ADDR line,
   the host gets CALLER lines. The game (this engine, in the ":game" process) sends its frames to 127.0.0.1:(27110 + pod) and gets
   the others' frames on 127.0.0.1:(27100 + pod), exactly as on the PC. */
public class MallOnline {
    public static class Entry {
        public String name = "", status = "", game = "", addr = "", lan = "", to = "", ver = "", plat = "PC", tok = "";
        public int n, max, port, pubPort;
        public boolean same, fromLan;
        public long seen = System.currentTimeMillis();
        public boolean isHost() { return status.equals("HOST") && (port > 0 || !tok.isEmpty()); }
        public InetSocketAddress target() {
            boolean near = (same || fromLan) && !lan.isEmpty();
            String a = near ? lan : addr;
            int p = near ? port : (pubPort > 0 ? pubPort : port);
            if (a.isEmpty() || p <= 0) return null;
            try { return new InetSocketAddress(InetAddress.getByName(a), p); } catch (Exception e) { return null; }
        }
    }

    public static final int LAN_PORT = 27209, NET_PROTO = 2;
    public static final int HOST_PORT = envPort("MALLVR_HOST_PORT", 27200);
    static int envPort(String v, int d) { try { int p = Integer.parseInt(System.getenv(v)); return p > 1024 && p < 65536 ? p : d; } catch (Exception e) { return d; } }
    public volatile String listUrl = "", name = "PLAYER", version = "", key = "";
    public final String id;
    public String platform = "QUEST";
    public volatile String status = "LOBBY", note = "", routerNote = "";
    public volatile boolean listOk;
    public volatile String game = "";
    public volatile int max = 2;
    public final Map<String, Integer> maxFor = new HashMap<>();           // game code -> players (games.json maxPlayers)
    public final Map<String, String> sha1For = new HashMap<>();           // game code -> SHA1 of the EXE this headset runs
    int maxOf(String g) { synchronized (maxFor) { Integer m = maxFor.get(g); return m != null ? m : 2; } }
    String sha1Of(String g) { synchronized (sha1For) { String h = sha1For.get(g); return h != null ? h : ""; } }
    public volatile int goPod;
    public volatile boolean inGame;
    public String lanIp = "";
    public volatile boolean quick;
    public volatile String quickGame = "";
    public final List<String> quickGames = new ArrayList<>();
    boolean quickWants(String g) { return !quickGame.isEmpty() ? g.equals(quickGame) : quickGames.contains(g); }
    static final Random quickRnd = new Random();
    public volatile int countdown;
    final Map<String, Long> badHosts = new HashMap<>();
    String joinKey = "";

    final Object lk = new Object();
    final List<Entry> web = new ArrayList<>(), lan = new ArrayList<>();
    final List<String> roster = new ArrayList<>(), rosterPlat = new ArrayList<>();

    static class Member { InetSocketAddress ep; String name, id = "", joinGame = "", plat = "PC"; int pod; long last; List<String> have; int answer = -1; double rtt, arrive = -1; }
    public final List<String> have = new ArrayList<>();
    final List<Member> members = new ArrayList<>();

    // groups (read by the launcher screen)
    public volatile String switchGame = "";
    public volatile int switchPod, promptSeq;
    public volatile String promptGame = "", promptHost = "";
    public volatile long promptUntil;
    public volatile boolean proposing;
    public volatile long proposeUntil;
    int seq, seenSeq, answered, seenSwitch, goSeq;
    String proposeGame = "";
    long proposeAt;
    public String switchNote = "", joinVia = "";
    public double joinSecs;
    long joinAt, startAt, hostedAt;

    volatile DatagramSocket ext, loc, lanRx;
    volatile InetSocketAddress hostEp;
    volatile long hostLast, joinSent;
    volatile boolean welcomed;
    volatile int myPod;
    int lagMs;
    volatile boolean running;
    volatile boolean upnpMapped;
    volatile int pubPort;
    String joinName = "", callTok = "";
    final Map<String, Entry> addrs = new HashMap<>();
    final List<Entry> callers = new ArrayList<>();

    public MallOnline() {
        id = newKey().substring(0, 24);
        try { lagMs = Integer.parseInt(System.getenv("MALLVR_LAG_MS")); } catch (Exception e) { lagMs = 0; }
        lanIp = findLanIp();
    }

    // ---- start / stop ---------------------------------------------------------------------------------
    public boolean lanFind = true;                    // false (tests): no LAN shouts heard or sent, so joins go through the list
    public void start() {
        if (running) return;
        running = true;
        if (lanFind) try {
            DatagramSocket s = new DatagramSocket(null);
            s.setReuseAddress(true); s.setBroadcast(true); s.bind(new InetSocketAddress(LAN_PORT));
            lanRx = s;
            spawn(this::lanLoop);
        } catch (Exception e) { lanRx = null; }
        spawn(this::beatLoop); spawn(this::tickLoop); spawn(this::delayLoop); spawn(this::quickLoop);
    }

    public void stop() {
        screenClosed("quit");
        leave();
        running = false;
        if (!listUrl.isEmpty()) try { get("op=bye&id=" + id); } catch (Exception e) { }
        close(lanRx); lanRx = null;
        if (upnpMapped) { upnp(false); upnpMapped = false; }
    }

    public static String newKey() {
        byte[] b = new byte[16]; new SecureRandom().nextBytes(b);
        StringBuilder s = new StringBuilder(); for (byte x : b) s.append(String.format("%02x", x)); return s.toString();
    }
    static void spawn(Runnable r) { Thread t = new Thread(r, "MallOnline"); t.setDaemon(true); t.start(); }
    static void close(DatagramSocket s) { if (s != null) try { s.close(); } catch (Exception e) { } }

    // ---- what the screen shows --------------------------------------------------------------------------
    public List<Entry> everyone() {
        synchronized (lk) {
            List<Entry> all = new ArrayList<>(web);
            for (Entry l : lan) {
                Entry w = null;
                for (Entry x : all) if (x.name.equals(l.name) && x.game.equals(l.game) && x.isHost()) { w = x; break; }
                if (w != null) { w.fromLan = true; w.lan = l.lan; if (w.port == 0) w.port = l.port; }
                else all.add(l);
            }
            return all;
        }
    }
    public List<String> roster() { synchronized (lk) { return new ArrayList<>(roster); } }
    public List<String> rosterPlat() {
        synchronized (lk) { List<String> r = new ArrayList<>(); for (int i = 0; i < roster.size(); i++) r.add(i < rosterPlat.size() ? rosterPlat.get(i) : "PC"); return r; }
    }

    // ---- host -------------------------------------------------------------------------------------------
    public boolean host(String g, int mx) {
        leave();
        try { ext = new DatagramSocket(HOST_PORT); }
        catch (Exception e) { note = "PORT " + HOST_PORT + " BUSY: ALREADY HOSTING?"; close(ext); ext = null; return false; }
        pubPort = 0; stun(ext);
        game = g; max = Math.max(2, Math.min(4, mx));
        synchronized (lk) { members.clear(); roster.clear(); rosterPlat.clear(); roster.add(name); rosterPlat.add(platform); }
        status = "HOST"; note = "WAITING FOR PLAYERS TO JOIN"; hostedAt = now();
        sessHosted = true; if (waitFrom == 0) waitFrom = now();
        DatagramSocket u = ext; spawn(() -> extLoop(u));
        if ("1".equals(System.getenv("MALLVR_NO_UPNP"))) routerNote = "ROUTER PORT NOT TRIED (MALLVR_NO_UPNP)";
        else if (!upnpMapped) {
            routerNote = "OPENING YOUR ROUTER PORT...";
            spawn(() -> { upnpMapped = upnp(true); routerNote = upnpMapped ? "ROUTER PORT OPEN: ANYONE CAN JOIN" : "ROUTER SAID NO TO OPENING A PORT: PLAYERS WILL PUNCH THROUGH INSTEAD"; });
        }
        beat();
        return true;
    }

    public boolean startMatch() {
        if (!status.equals("HOST")) return false;
        synchronized (lk) {
            if (members.isEmpty()) { note = "WAIT FOR ANOTHER PLAYER TO JOIN"; return false; }
            for (int i = 0; i < members.size(); i++) members.get(i).pod = i + 2;
            String s = switchPacket(++seq, game);
            startAt = now(); for (Member m : members) m.arrive = -1;
            for (int r = 0; r < 3; r++) for (Member m : members) send(ext, m.have != null ? s : "G\t" + m.pod, m.ep);
        }
        goPod = 1;
        return true;
    }

    String switchPacket(int sq, String g) {
        StringBuilder b = new StringBuilder("S\t" + sq + "\t" + g);
        for (Member m : members) b.append('\t').append(!m.id.isEmpty() ? m.id : m.name).append('=').append(m.pod);
        return b.toString();
    }

    // ---- groups -----------------------------------------------------------------------------------------
    public boolean isHost() { return hostEp == null && (status.equals("HOST") || (status.equals("PLAYING") && ext != null)); }
    public boolean inGroup() {
        if (ext == null) return false;
        if (hostEp != null) return welcomed;
        synchronized (lk) { return !members.isEmpty(); }
    }

    public List<String> commonGames() {
        List<String> r = new ArrayList<>();
        synchronized (lk) {
            int n = members.size() + 1;
            for (String g : have) {
                if (maxOf(g) < n) continue;
                boolean all = true;
                for (Member m : members) if (m.have == null || !m.have.contains(g)) { all = false; break; }
                if (all) r.add(g);
            }
        }
        return r;
    }

    public void setGame(String g) {
        if (!status.equals("HOST") || g.equals(game)) return;
        game = g; max = Math.max(max, maxOf(g));
        synchronized (lk) {
            for (Iterator<Member> it = members.iterator(); it.hasNext(); ) { Member m = it.next(); if (m.have == null && !m.joinGame.equals(g)) { send(ext, "F\tTHE GROUP NEEDS A NEWER MALLVR", m.ep); it.remove(); } }
            for (Member m : members) send(ext, "C\t" + g, m.ep);
        }
        sendRoster();
        beat();
    }

    public boolean propose(String g) {
        if (!isHost() || proposing) return false;
        synchronized (lk) {
            if (members.isEmpty()) return false;
            proposeGame = g; seq++;
            for (Member m : members) m.answer = -1;
        }
        proposeAt = now(); proposeUntil = proposeAt + 10000;
        proposing = true;
        sendProposal();
        return true;
    }
    void sendProposal() { synchronized (lk) { for (Member m : members) if (m.answer < 0) send(ext, "N\t" + seq + "\t" + proposeGame + "\t" + name, m.ep); } }
    public void cancelPropose() { proposing = false; }

    void commit() {
        proposing = false;
        int go = 0, stay = 0, none = 0, old = 0;
        synchronized (lk) {
            List<Member> keep = new ArrayList<>();
            for (Member m : members) {
                if (m.have == null) { old++; send(ext, "F\tTHE GROUP NEEDS A NEWER MALLVR", m.ep); continue; }
                if (m.answer == 0) { stay++; continue; }
                if (m.answer == 1) go++; else none++;
                keep.add(m);
            }
            members.clear(); members.addAll(keep);
            for (int i = 0; i < members.size(); i++) members.get(i).pod = i + 2;
            String s = switchPacket(seq, proposeGame);
            startAt = now(); for (Member m : members) m.arrive = -1;
            for (int r = 0; r < 3; r++) for (Member m : members) send(ext, s, m.ep);
        }
        switchNote = "go=" + go + " stay=" + stay + " none=" + none + " old=" + old + " secs=" + (now() - proposeAt) / 1000;
        game = proposeGame; max = Math.max(maxOf(game), 2);
        sendRoster();
        if (members.isEmpty()) { note = "NOBODY CAME ALONG"; switchNote += " empty=1"; }
        switchPod = 1; switchGame = game;
    }

    public void answer(int sq, boolean go) {
        if (hostEp == null) return;
        answered = sq;
        if (sq == promptSeq) promptSeq = 0;
        for (int r = 0; r < 3; r++) send(ext, "A\t" + sq + "\t" + (go ? 1 : 0), hostEp);
        if (!go) { leave(); note = "YOU STAYED OUT: BACK IN THE LOBBY"; }
    }
    public void switchTaken() { switchGame = ""; switchPod = 0; }

    // ---- join -------------------------------------------------------------------------------------------
    public void join(Entry e) {
        leave();
        if (e.addr.isEmpty() && !e.fromLan && !e.tok.isEmpty()) {     // the list keeps addresses private: ask for this host's
            callTok = e.tok; joinName = e.name; game = e.game; status = "JOINED";
            beat();
            Entry a; synchronized (lk) { a = addrs.get(e.tok); }
            if (a != null) { e.addr = a.addr; e.port = a.port; e.pubPort = a.pubPort; e.same = a.same; if (!a.lan.isEmpty()) e.lan = a.lan; }
        }
        InetSocketAddress t = e.target();
        joinPlat = e.plat;
        if (t == null) { callTok = ""; status = "LOBBY"; note = "THAT HOST HAS NO ADDRESS"; fail("no_address"); return; }
        try { ext = new DatagramSocket(0); }
        catch (Exception x) { note = "COULD NOT OPEN A NETWORK PORT"; close(ext); ext = null; fail("no_port"); return; }
        pubPort = 0; stun(ext);
        hostEp = t; welcomed = false; game = e.game; max = e.max; joinName = e.name;
        joinVia = (e.same || e.fromLan) && !e.lan.isEmpty() ? "lan" : "internet"; joinAt = now(); joinSecs = 0;
        joinKey = hostKey(e); callTok = e.tok;
        sessJoined = true; if (waitFrom == 0) waitFrom = now();
        hostLast = now(); joinSent = 0;
        synchronized (lk) { roster.clear(); rosterPlat.clear(); }
        status = "JOINED"; note = "CALLING " + e.name + "...";
        DatagramSocket u = ext; spawn(() -> extLoop(u));
        sendJoin();
        beat();
    }

    void sendJoin() { joinSent = now(); send(ext, "J\t" + name + "\t" + game + "\t" + NET_PROTO + "\t" + version + "\t" + id + "\t" + sha1Of(game) + "\t" + platform, hostEp); }

    public void leave() {
        DatagramSocket u = ext;
        if (u != null) {
            if (status.equals("HOST")) { synchronized (lk) { for (Member m : members) send(u, "X", m.ep); } }
            else if (hostEp != null) send(u, "X", hostEp);
        }
        endGame();
        close(ext); ext = null;
        synchronized (lk) { members.clear(); roster.clear(); rosterPlat.clear(); }
        hostEp = null; goPod = 0; callTok = "";
        proposing = false; promptSeq = 0;
        status = "LOBBY";
    }

    // ---- the game ---------------------------------------------------------------------------------------
    public static int localPort(int pod) { return 27110 + pod; }

    // play notes (MallOnline.cs Arrivals / LagNote): host, seconds from START until each member's game sent its first frame; ping
    public String arrivals() {
        StringBuilder b = new StringBuilder();
        synchronized (lk) { for (Member m : members) b.append(b.length() > 0 ? " " : "").append(m.name.replace(' ', '_')).append('=').append(m.arrive < 0 ? "none" : String.format(Locale.US, "%.1f", m.arrive)); }
        return b.toString();
    }
    public String lagNote() {
        StringBuilder b = new StringBuilder();
        synchronized (lk) { for (Member m : members) b.append(b.length() > 0 ? " " : "").append(m.name.replace(' ', '_')).append('=').append((int) m.rtt); }
        return b.toString();
    }

    public void beginGame(int pod) {
        myPod = pod;
        try { loc = new DatagramSocket(new InetSocketAddress(InetAddress.getByName("127.0.0.1"), localPort(pod))); }
        catch (Exception e) { note = "LOCAL PORT " + localPort(pod) + " BUSY"; close(loc); loc = null; }
        inGame = true; goPod = 0;
        status = "PLAYING";
        sessGames++; if (waitSecs < 0 && waitFrom != 0) waitSecs = (now() - waitFrom) / 1000.0;
        if (hostEp != null && goSeq > 0) for (int r = 0; r < 3; r++) send(ext, "Y\t" + goSeq, hostEp);
        DatagramSocket l = loc;
        if (l != null) spawn(() -> locLoop(l));
        beat();
    }

    public void endGame() { inGame = false; close(loc); loc = null; }

    public void afterGame() {
        quick = false; countdown = 0; proposing = false; promptSeq = 0;
        endGame();
        if (ext == null) { leave(); status = "LOBBY"; }
        else if (hostEp == null) { status = "HOST"; synchronized (lk) { for (Member m : members) send(ext, "E\t" + seq, m.ep); } sendRoster(); }
        else if (welcomed) status = "GROUP";
        else leave();
        note = inGroup() ? "STILL TOGETHER: " + (isHost() ? "START OR PICK ANOTHER GAME" : "WAITING FOR THE HOST") : "";
        beat();
    }

    // ---- loops ------------------------------------------------------------------------------------------
    void locLoop(DatagramSocket u) {
        byte[] buf = new byte[2048];
        while (running && u == loc) {
            DatagramPacket pk = new DatagramPacket(buf, buf.length);
            try { u.receive(pk); } catch (Exception e) { break; }
            int n = pk.getLength();
            if (!pk.getAddress().isLoopbackAddress() || n == 0 || n > 1600) continue;
            byte[] p = new byte[n + 2];
            p[0] = 'D'; p[1] = (byte) myPod;
            System.arraycopy(pk.getData(), pk.getOffset(), p, 2, n);
            DatagramSocket x = ext;
            if (x == null) continue;
            if (status.equals("PLAYING") && hostEp == null) { synchronized (lk) { for (Member m : members) sendRaw(x, p, m.ep); } }
            else if (hostEp != null) sendRaw(x, p, hostEp);
        }
    }

    void extLoop(DatagramSocket u) {
        byte[] buf = new byte[2048];
        while (running && u == ext) {
            DatagramPacket pk = new DatagramPacket(buf, buf.length);
            try { u.receive(pk); } catch (SocketTimeoutException e) { continue; } catch (Exception e) { break; }
            int n = pk.getLength();
            if (n == 0) continue;
            byte[] p = Arrays.copyOfRange(pk.getData(), pk.getOffset(), pk.getOffset() + n);
            if (n >= 20 && p[0] == 1 && p[1] == 1) { stunReply(p); continue; }
            char k = (char) (p[0] & 0xFF);
            InetSocketAddress from = (InetSocketAddress) pk.getSocketAddress();
            InetSocketAddress h = hostEp;
            if (h == null) hostPacket(u, k, p, from);
            else if (from.equals(h)) joinerPacket(k, p);
        }
    }

    Member find(InetSocketAddress ep) { for (Member m : members) if (m.ep.equals(ep)) return m; return null; }

    void hostPacket(DatagramSocket u, char k, byte[] p, InetSocketAddress from) {
        Member m;
        synchronized (lk) { m = find(from); }
        if (k == 'D') {
            if (m == null || p.length < 3) return;
            m.last = now();
            deliver(p, 2);
            synchronized (lk) { for (Member o : members) if (o != m) sendRaw(u, p, o.ep); }
            return;
        }
        String[] f = new String(p, StandardCharsets.US_ASCII).split("\t", -1);
        if (k == 'J') {
            String nm = f.length > 1 ? clean(f[1], 12) : "PLAYER";
            if (m == null) {
                if (!status.equals("HOST")) { send(u, "F\tTHAT GAME ALREADY STARTED", from); return; }
                if (f.length > 2 && !f[2].equals(game)) { send(u, "F\tTHAT HOST CHANGED GAMES", from); return; }
                if (f.length > 3) {
                    try { int proto = Integer.parseInt(f[3]); if (proto != NET_PROTO) { send(u, "F\tNEEDS MALLVR " + (proto < NET_PROTO ? version : "UPDATE: THE HOST HAS AN OLDER ONE"), from); return; } }
                    catch (NumberFormatException e) { }
                }
                String mine = sha1Of(game), theirs = f.length > 6 ? hexId(f[6]) : "";
                if (!mine.isEmpty() && theirs.length() == 40 && !theirs.equals(mine)) { send(u, "F\tDIFFERENT GAME FILES: " + game + " IS ANOTHER VERSION", from); return; }
                synchronized (lk) {
                    if (members.size() + 1 >= max) { send(u, "F\tTHAT GAME IS FULL", from); return; }
                    m = new Member(); m.ep = from; m.name = nm; m.joinGame = game; m.id = f.length > 5 ? hexId(f[5]) : "";
                    m.plat = f.length > 7 && !clean(f[7], 8).isEmpty() ? clean(f[7], 8) : "PC";
                    members.add(m);
                }
                note = nm + " JOINED";
            }
            m.last = now();
            send(u, "W", from);
            sendRoster();
        }
        else if (k == 'K') { if (m != null) m.last = now(); }
        else if (k == 'H' && m != null) {
            List<String> h = new ArrayList<>();
            for (int i = 1; i < f.length; i++) { String c = clean(f[i], 10); if (!c.isEmpty()) h.add(c); }
            m.have = h; m.last = now();
        }
        else if (k == 'A' && m != null) {
            try { int sq = Integer.parseInt(f[1]), a = Integer.parseInt(f[2]); if (sq == seq && proposing) { m.answer = a == 0 ? 0 : 1; if (a == 0) note = m.name + " STAYED OUT"; } }
            catch (Exception e) { }
        }
        else if (k == 'R' && m != null) {
            try { long t0 = Long.parseLong(f[1]); double rtt = now() - t0; if (rtt >= 0 && rtt < 5000) m.rtt = m.rtt == 0 ? rtt : m.rtt * 0.8 + rtt * 0.2; } catch (Exception e) { }
        }
        else if (k == 'Y' && m != null) {
            try { int sq = Integer.parseInt(f[1]); if (sq == seq && m.arrive < 0) m.arrive = (now() - startAt) / 1000.0; } catch (Exception e) { }
        }
        else if (k == 'X' && m != null) {
            synchronized (lk) { members.remove(m); }
            note = m.name + " LEFT";
            sendRoster();
        }
    }

    void joinerPacket(char k, byte[] p) {
        hostLast = now();
        if (k == 'D') { if (p.length > 2) deliver(p, 2); return; }
        String[] f = new String(p, StandardCharsets.US_ASCII).split("\t", -1);
        if (k == 'W') {
            if (!welcomed) { welcomed = true; joinSecs = (now() - joinAt) / 1000.0; note = "IN THE GAME: WAITING FOR THE HOST TO START"; }
            sendHave();
        }
        else if (k == 'Q') { if (f.length > 1) send(ext, "R\t" + f[1], hostEp); }
        else if (k == 'N') {
            try {
                int sq = Integer.parseInt(f[1]);
                if (f.length > 2 && sq > seenSeq) {
                    seenSeq = sq;
                    promptGame = clean(f[2], 10); promptHost = f.length > 3 ? clean(f[3], 12) : "THE HOST";
                    promptUntil = now() + 10000; promptSeq = sq;
                }
            } catch (Exception e) { }
        }
        else if (k == 'S') {
            if (f.length < 3) return;
            int sq, pod = 0;
            try { sq = Integer.parseInt(f[1]); } catch (Exception e) { return; }
            for (int i = 3; i < f.length; i++) {
                String[] kv = f[i].split("=");
                if (kv.length == 2 && (kv[0].equals(id) || kv[0].equals(name))) try { pod = Integer.parseInt(kv[1]); } catch (Exception e) { }
            }
            if (sq <= seenSwitch) return;
            seenSwitch = sq; seenSeq = Math.max(seenSeq, sq); promptSeq = 0;
            String g = clean(f[2], 10);
            if (pod < 2 || pod > 4) { if (answered != sq) { leave(); note = "THE GROUP WENT TO " + g + " WITHOUT YOU"; } return; }
            game = g; goSeq = sq;
            if (inGame) { switchPod = pod; switchGame = g; }
            else if (goPod == 0) goPod = pod;
        }
        else if (k == 'C') { if (f.length > 1) { game = clean(f[1], 10); note = "THE HOST PICKED " + game; } }
        else if (k == 'E') { if (!inGame) note = "STILL TOGETHER: WAITING FOR THE HOST"; }
        else if (k == 'F') { markBad(); String why = f.length > 1 ? f[1] : "REFUSED"; fail("refused: " + why); leave(); note = why; }
        else if (k == 'L') { synchronized (lk) { roster.clear(); for (int i = 1; i < f.length; i++) roster.add(clean(f[i], 12)); } }
        else if (k == 'T') { synchronized (lk) { rosterPlat.clear(); for (int i = 1; i < f.length; i++) rosterPlat.add(clean(f[i], 8)); } }
        else if (k == 'G') { try { int pod = Integer.parseInt(f[1]); if (pod >= 2 && pod <= 4 && goPod == 0 && !inGame) goPod = pod; } catch (Exception e) { } }
        else if (k == 'X') { if (!inGame) { leave(); note = "THE HOST CLOSED THE GAME"; } }
    }

    void sendHave() {
        StringBuilder b = new StringBuilder("H");
        synchronized (lk) { for (String g : have) b.append('\t').append(g); }
        send(ext, b.toString(), hostEp);
    }

    void sendRoster() {
        StringBuilder sb = new StringBuilder("L\t" + name), tb = new StringBuilder("T\t" + platform);
        synchronized (lk) {
            roster.clear(); roster.add(name); rosterPlat.clear(); rosterPlat.add(platform);
            for (Member m : members) { sb.append('\t').append(m.name); roster.add(m.name); tb.append('\t').append(m.plat); rosterPlat.add(m.plat); }
            for (Member m : members) { send(ext, sb.toString(), m.ep); send(ext, tb.toString(), m.ep); }
        }
    }

    void tickLoop() {
        int n = 0;
        while (running) {
            sleep(1000);
            n++;
            DatagramSocket u = ext;
            if (u == null) continue;
            InetSocketAddress h = hostEp;
            if (h == null) {                              // host
                boolean changed;
                synchronized (lk) {
                    for (Member m : members) send(u, "K", m.ep);
                    int before = members.size();
                    long lim = inGame ? 30000 : 10000, t = now();
                    members.removeIf(m -> t - m.last > lim);
                    changed = members.size() != before;
                }
                if (changed) { note = "A PLAYER DROPPED OUT"; fail("member_dropped"); sendRoster(); }
                else if (n % 3 == 0 && status.equals("HOST")) sendRoster();
                if (n % 2 == 0) synchronized (lk) { for (Member m : members) send(u, "Q\t" + now(), m.ep); }
                if (proposing) {
                    boolean all = true;
                    synchronized (lk) { for (Member m : members) if (m.answer < 0 && m.have != null) all = false; }
                    if (all || now() >= proposeUntil) commit(); else sendProposal();
                }
                if (status.equals("HOST") && n % 2 == 0 && lanFind) shout(u);
                if (status.equals("HOST")) punch(u);
            } else {                                      // joiner
                send(u, "K", h);
                if (!welcomed && now() - joinSent >= 1000) sendJoin();
                long quiet = now() - hostLast;
                if (!welcomed && quiet > 25000) { markBad(); fail("no_answer"); leave(); note = "NO ANSWER: HOST ROUTER MAY BE CLOSED"; }
                else if (welcomed && !inGame && quiet > 15000) { fail("lost_host"); leave(); note = "LOST THE HOST"; }
            }
        }
    }

    // ---- QUICK MATCH ------------------------------------------------------------------------------------
    public void startQuick(String g) { quickGame = g; countdown = 0; quick = true; note = "LOOKING FOR A GAME..."; quickPresses++; if (waitFrom == 0) waitFrom = now(); }
    public void stopQuick() { quick = false; countdown = 0; if (!inGame) leave(); note = ""; }

    static String hostKey(Entry e) { return e.name + "|" + e.addr + "|" + e.lan + "|" + e.port + "|" + e.tok; }
    void markBad() { if (!joinKey.isEmpty()) synchronized (badHosts) { badHosts.put(joinKey, now() + 120000); } }
    boolean isBad(Entry e) { synchronized (badHosts) { Long u = badHosts.get(hostKey(e)); return u != null && u > now(); } }

    void quickLoop() {
        while (running) {
            sleep(1000);
            if (!quick || inGame || goPod > 0) { countdown = 0; continue; }
            String st = status;
            List<Entry> open = new ArrayList<>();
            for (Entry e : everyone()) if (e.isHost() && quickWants(e.game) && !e.name.equals(name) && (e.max == 0 || e.n < e.max) && !isBad(e)) open.add(e);
            if (st.equals("LOBBY")) {
                if (!open.isEmpty()) { open.sort((a, b) -> Integer.compare(b.n, a.n)); join(open.get(0)); }
                else if (!quickGame.isEmpty() || !quickGames.isEmpty()) {
                    String g = !quickGame.isEmpty() ? quickGame : quickGames.get(quickRnd.nextInt(quickGames.size()));
                    if (host(g, maxOf(g))) note = "WAITING FOR ANOTHER PLAYER...";
                }
            } else if (st.equals("HOST")) {
                int count; synchronized (lk) { count = members.size() + 1; }
                if (count == 1) {
                    countdown = 0;
                    Entry other = null;
                    for (Entry e : open) if (e.n <= 1 && (e.name + e.lan).compareTo(name + lanIp) < 0) { other = e; break; }
                    // still alone after 15 s: the other lone host may not have our game, so it never comes to us: go to its game
                    if (other == null && now() - hostedAt > 15000) for (Entry e : open) if (e.n <= 1) { other = e; break; }
                    if (other != null) join(other);
                } else if (count >= max) {
                    if (countdown == 0) countdown = 4;
                    countdown--;
                    note = countdown > 0 ? "FOUND A PLAYER! STARTING IN " + countdown : "STARTING!";
                    if (countdown <= 0) { countdown = 0; startMatch(); }
                } else countdown = 0;
            } else if (st.equals("JOINED") && welcomed) note = "FOUND A GAME! STARTING SOON...";
        }
    }

    void shout(DatagramSocket u) {
        int count; synchronized (lk) { count = members.size() + 1; }
        byte[] b = ("MALLVR\t" + id + "\t" + name + "\t" + game + "\t" + count + "\t" + max + "\t" + HOST_PORT).getBytes(StandardCharsets.US_ASCII);
        try { u.setBroadcast(true); u.send(new DatagramPacket(b, b.length, InetAddress.getByName("255.255.255.255"), LAN_PORT)); } catch (Exception e) { }
    }

    void lanLoop() {
        byte[] buf = new byte[512];
        while (running && lanRx != null) {
            DatagramPacket pk = new DatagramPacket(buf, buf.length);
            try { lanRx.receive(pk); } catch (SocketTimeoutException e) { continue; } catch (Exception e) { break; }
            String[] f = new String(pk.getData(), pk.getOffset(), pk.getLength(), StandardCharsets.US_ASCII).split("\t", -1);
            if (f.length < 7 || !f[0].equals("MALLVR") || f[1].equals(id)) continue;
            Entry e = new Entry(); e.name = clean(f[2], 12); e.status = "HOST"; e.game = clean(f[3], 8); e.lan = pk.getAddress().getHostAddress(); e.fromLan = true;
            try { e.n = Integer.parseInt(f[4]); e.max = Integer.parseInt(f[5]); e.port = Integer.parseInt(f[6]); } catch (Exception x) { continue; }
            synchronized (lk) { lan.removeIf(x -> x.lan.equals(e.lan) && x.port == e.port); lan.add(e); }
        }
    }

    void beatLoop() {
        int n = 0;
        while (running) {
            if (n++ % 3 == 0) beat();
            synchronized (lk) { long t = System.currentTimeMillis(); lan.removeIf(x -> t - x.seen > 6000); }
            sleep(1000);
        }
    }

    public void beat() {
        if (listUrl.isEmpty()) return;
        int count = 1;
        String st = status;
        if (st.equals("HOST")) synchronized (lk) { count = members.size() + 1; }
        String q = "op=beat&id=" + id + "&name=" + enc(name) + "&status=" + st + "&game=" + (st.equals("LOBBY") ? "" : game) + "&n=" + count + "&max=" + max
            + "&port=" + (st.equals("HOST") ? HOST_PORT : 0) + "&lan=" + lanIp + "&ver=" + enc(version) + "&key=" + key
            + "&pp=" + pubPort + "&to=" + (st.equals("JOINED") ? enc(joinName) : "") + "&plat=" + platform + "&call=" + (st.equals("JOINED") ? callTok : "");
        try {
            String body = get(q);
            List<Entry> got = new ArrayList<>(), calls = new ArrayList<>();
            Map<String, Entry> ad = new HashMap<>();
            for (String line : body.split("\n")) {
                String[] f = line.replace("\r", "").split("\t", -1);
                if (f.length >= 7 && f[0].equals("ADDR")) {
                    Entry a = new Entry(); a.tok = f[1]; a.addr = f[2]; a.lan = f[4]; a.same = f[5].equals("1");
                    a.port = toInt(f[3]); a.pubPort = toInt(f[6]); ad.put(a.tok, a); continue;
                }
                if (f.length >= 4 && f[0].equals("CALLER")) {
                    Entry c = new Entry(); c.name = f[1]; c.status = "JOINED"; c.addr = f[2]; c.to = name; c.pubPort = toInt(f[3]); calls.add(c); continue;
                }
                if (f.length < 9) continue;
                Entry e = new Entry(); e.name = f[0]; e.status = f[1]; e.game = f[2]; e.addr = f[5]; e.lan = f[7]; e.same = f[8].equals("1");
                e.n = toInt(f[3]); e.max = toInt(f[4]); e.port = toInt(f[6]);
                if (f.length >= 11) { e.pubPort = toInt(f[9]); e.to = f[10]; }
                if (f.length >= 13) { e.ver = f[11]; e.plat = !f[12].isEmpty() ? f[12] : "PC"; }
                if (f.length >= 14) e.tok = f[13];
                got.add(e);
            }
            synchronized (lk) { web.clear(); web.addAll(got); callers.clear(); callers.addAll(calls); addrs.putAll(ad); }
            int others = 0, open = 0;
            for (Entry x : got) if (!x.name.equals(name)) { others++; if (x.isHost()) open++; }
            seenMax = Math.max(seenMax, others); openMax = Math.max(openMax, open);
            listOk = true;
        } catch (Exception e) {
            if (listOk || !listFailNoted) { listFailNoted = true; event("list_fail", "why", e.getClass().getSimpleName() + ": " + shorten(e.getMessage(), 80), "plat", platform, "ver", version); }
            listOk = false;
        }
    }

    String get(String query) throws IOException {
        HttpURLConnection c = (HttpURLConnection) new URL(listUrl + (listUrl.contains("?") ? "&" : "?") + query).openConnection();
        c.setConnectTimeout(5000); c.setReadTimeout(5000); c.setRequestProperty("User-Agent", "MallVR/" + version);
        try (InputStream in = c.getInputStream()) {
            ByteArrayOutputStream b = new ByteArrayOutputStream(); byte[] x = new byte[8192]; int n;
            while ((n = in.read(x)) > 0) b.write(x, 0, n);
            return b.toString("US-ASCII");
        } finally { c.disconnect(); }
    }

    // ---- frames to the local pod, optionally late (MALLVR_LAG_MS) ----------------------------------------
    final ArrayDeque<Object[]> late = new ArrayDeque<>();

    void deliver(byte[] p, int off) {
        DatagramSocket u = loc;
        if (u == null || myPod <= 0) return;
        byte[] f = Arrays.copyOfRange(p, off, p.length);
        if (lagMs > 0) { synchronized (late) { late.add(new Object[] { now() + lagMs, f }); } return; }
        sendRaw(u, f, new InetSocketAddress(InetAddress.getLoopbackAddress(), 27100 + myPod));
    }

    void delayLoop() {
        while (running) {
            sleep(lagMs > 0 ? 1 : 200);
            while (true) {
                byte[] f = null;
                synchronized (late) { Object[] h = late.peek(); if (h != null && (Long) h[0] <= now()) f = (byte[]) late.poll()[1]; }
                if (f == null) break;
                DatagramSocket u = loc;
                if (u != null) sendRaw(u, f, new InetSocketAddress(InetAddress.getLoopbackAddress(), 27100 + myPod));
            }
        }
    }

    // ---- hole punching (MallOnline.cs has the why) -------------------------------------------------------
    static final String[] STUN = { "stun.l.google.com:19302", "stun1.l.google.com:19302", "stun.cloudflare.com:3478" };
    void stun(DatagramSocket u) {
        if (u == null) return;
        spawn(() -> {
            byte[] req = new byte[20];
            req[1] = 1; req[4] = 0x21; req[5] = 0x12; req[6] = (byte) 0xA4; req[7] = 0x42;
            byte[] tid = new byte[12]; new SecureRandom().nextBytes(tid); System.arraycopy(tid, 0, req, 8, 12);
            for (int tries = 0; tries < 3 && pubPort == 0 && u == ext; tries++) {
                for (String sv : STUN) {
                    try {
                        String[] hp = sv.split(":");
                        for (InetAddress a : InetAddress.getAllByName(hp[0])) if (a instanceof Inet4Address) { sendRaw(u, req, new InetSocketAddress(a, Integer.parseInt(hp[1]))); break; }
                    } catch (Exception e) { }
                }
                sleep(1500);
            }
        });
    }

    void stunReply(byte[] p) {
        for (int o = 20; o + 4 <= p.length; ) {
            int t = (p[o] & 0xFF) << 8 | (p[o + 1] & 0xFF), len = (p[o + 2] & 0xFF) << 8 | (p[o + 3] & 0xFF);
            if (o + 4 + len > p.length) break;
            if ((t == 0x0020 || t == 0x0001) && len >= 8 && p[o + 5] == 1) {
                int port = (p[o + 6] & 0xFF) << 8 | (p[o + 7] & 0xFF);
                if (t == 0x0020) port ^= 0x2112;
                if (port > 0 && pubPort != port) { pubPort = port; beat(); }
                return;
            }
            o += 4 + ((len + 3) & ~3);
        }
    }

    void punch(DatagramSocket u) {
        List<Entry> c = new ArrayList<>();
        synchronized (lk) {
            for (Entry e : web) if (e.status.equals("JOINED") && e.to.equals(name) && e.pubPort > 0 && !e.addr.isEmpty()) c.add(e);
            c.addAll(callers);
        }
        for (Entry e : c) {
            InetSocketAddress ep;
            try { ep = new InetSocketAddress(InetAddress.getByName(e.addr), e.pubPort); } catch (Exception x) { continue; }
            boolean joined; synchronized (lk) { joined = find(ep) != null; }
            if (!joined) send(u, "P", ep);
        }
    }

    // ---- UPnP: ask the router to open HOST_PORT (SSDP + one SOAP call; MallOnline.cs uses Windows' helper) ----
    String upnpControl = "", upnpService = "";
    boolean upnp(boolean add) {
        try {
            if (upnpControl.isEmpty() && !upnpFind()) return false;
            String body = add
                ? "<NewRemoteHost></NewRemoteHost><NewExternalPort>" + HOST_PORT + "</NewExternalPort><NewProtocol>UDP</NewProtocol><NewInternalPort>" + HOST_PORT
                  + "</NewInternalPort><NewInternalClient>" + lanIp + "</NewInternalClient><NewEnabled>1</NewEnabled><NewPortMappingDescription>MallVR online</NewPortMappingDescription><NewLeaseDuration>0</NewLeaseDuration>"
                : "<NewRemoteHost></NewRemoteHost><NewExternalPort>" + HOST_PORT + "</NewExternalPort><NewProtocol>UDP</NewProtocol>";
            String act = add ? "AddPortMapping" : "DeletePortMapping";
            String env = "<?xml version=\"1.0\"?><s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><s:Body><u:"
                + act + " xmlns:u=\"" + upnpService + "\">" + body + "</u:" + act + "></s:Body></s:Envelope>";
            HttpURLConnection c = (HttpURLConnection) new URL(upnpControl).openConnection();
            c.setConnectTimeout(3000); c.setReadTimeout(3000); c.setDoOutput(true); c.setRequestMethod("POST");
            c.setRequestProperty("Content-Type", "text/xml; charset=\"utf-8\""); c.setRequestProperty("SOAPAction", "\"" + upnpService + "#" + act + "\"");
            c.getOutputStream().write(env.getBytes(StandardCharsets.UTF_8));
            int code = c.getResponseCode(); c.disconnect();
            return code == 200;
        } catch (Exception e) { return false; }
    }

    boolean upnpFind() throws IOException {
        String ms = "M-SEARCH * HTTP/1.1\r\nHOST: 239.255.255.250:1900\r\nMAN: \"ssdp:discover\"\r\nMX: 2\r\nST: urn:schemas-upnp-org:device:InternetGatewayDevice:1\r\n\r\n";
        String loc = null;
        try (DatagramSocket s = new DatagramSocket()) {
            s.setSoTimeout(3000);
            byte[] b = ms.getBytes(StandardCharsets.US_ASCII);
            s.send(new DatagramPacket(b, b.length, InetAddress.getByName("239.255.255.250"), 1900));
            byte[] r = new byte[2048];
            DatagramPacket pk = new DatagramPacket(r, r.length);
            s.receive(pk);
            for (String l : new String(r, 0, pk.getLength(), StandardCharsets.US_ASCII).split("\r\n"))
                if (l.toLowerCase(Locale.US).startsWith("location:")) loc = l.substring(9).trim();
        }
        if (loc == null) return false;
        HttpURLConnection c = (HttpURLConnection) new URL(loc).openConnection();
        c.setConnectTimeout(3000); c.setReadTimeout(3000);
        String xml;
        try (InputStream in = c.getInputStream()) { ByteArrayOutputStream o = new ByteArrayOutputStream(); byte[] x = new byte[8192]; int n; while ((n = in.read(x)) > 0) o.write(x, 0, n); xml = o.toString("UTF-8"); }
        for (String svc : new String[] { "urn:schemas-upnp-org:service:WANIPConnection:1", "urn:schemas-upnp-org:service:WANIPConnection:2", "urn:schemas-upnp-org:service:WANPPPConnection:1" }) {
            int i = xml.indexOf(svc); if (i < 0) continue;
            int a = xml.indexOf("<controlURL>", i), z = xml.indexOf("</controlURL>", a);
            if (a < 0 || z < 0) continue;
            upnpControl = new URL(new URL(loc), xml.substring(a + 12, z).trim()).toString(); upnpService = svc;
            return true;
        }
        return false;
    }

    // ---- play-note events (MallOnline.cs Fail / ScreenOpened / ScreenClosed); the Quest screen drains them into PlayNotes ---
    public final List<String[]> events = new ArrayList<>();   // {ev, k, v, k, v...}
    String joinPlat = "";
    boolean listFailNoted, sessHosted, sessJoined;
    long sessOpen, waitFrom;
    int seenMax, openMax, quickPresses, sessGames;
    double waitSecs = -1;

    void event(String ev, String... kv) { String[] e = new String[kv.length + 1]; e[0] = ev; System.arraycopy(kv, 0, e, 1, kv.length); synchronized (events) { events.add(e); } }

    void fail(String why) {
        event("online_fail", "why", shorten(why, 60), "role", hostEp == null ? "host" : "member", "game", game, "via", joinVia,
            "host_plat", hostEp == null ? platform : joinPlat, "stun", pubPort > 0 ? "1" : "0", "router", upnpMapped ? "open" : "closed",
            "list", listOk ? "ok" : "down", "in_game", inGame ? "1" : "0", "plat", platform, "ver", version);
    }

    public void screenOpened() { sessOpen = now(); waitFrom = 0; waitSecs = -1; seenMax = openMax = quickPresses = sessGames = 0; sessHosted = sessJoined = false; }

    public void screenClosed(String how) {
        if (sessOpen == 0) return;
        long secs = (now() - sessOpen) / 1000;
        double waited = waitSecs >= 0 ? waitSecs : waitFrom != 0 ? (now() - waitFrom) / 1000.0 : -1;
        event("online_session", "secs", "" + secs, "seen_max", "" + seenMax, "open_max", "" + openMax, "quick", "" + quickPresses,
            "hosted", sessHosted ? "1" : "0", "joined", sessJoined ? "1" : "0", "games", "" + sessGames, "wait_secs", waited < 0 ? "" : "" + (int) waited,
            "matched", waitSecs >= 0 ? "1" : "0", "ended", how, "plat", platform, "ver", version);
        sessOpen = 0;
    }

    static String shorten(String s, int n) { s = s == null ? "" : s.replace('\t', ' ').replace('\r', ' ').replace('\n', ' '); return s.length() > n ? s.substring(0, n) : s; }

    // ---- helpers ----------------------------------------------------------------------------------------
    static void send(DatagramSocket u, String s, InetSocketAddress to) { sendRaw(u, s.getBytes(StandardCharsets.US_ASCII), to); }
    static void sendRaw(DatagramSocket u, byte[] b, InetSocketAddress to) {
        if (u == null || to == null) return;
        try { u.send(new DatagramPacket(b, b.length, to)); } catch (Exception e) { }
    }
    static final long T0 = System.nanoTime();
    static long now() { return (System.nanoTime() - T0) / 1000000L; }
    static void sleep(int ms) { try { Thread.sleep(ms); } catch (InterruptedException e) { } }
    static int toInt(String s) { try { return Integer.parseInt(s.trim()); } catch (Exception e) { return 0; } }
    static String enc(String s) { try { return URLEncoder.encode(s, "UTF-8").replace("+", "%20"); } catch (Exception e) { return ""; } }
    static String hexId(String s) {
        StringBuilder b = new StringBuilder();
        for (char c : (s == null ? "" : s).toLowerCase(Locale.US).toCharArray()) if (((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')) && b.length() < 40) b.append(c);
        return b.toString();
    }
    public static String clean(String s, int n) {
        StringBuilder b = new StringBuilder();
        for (char c : (s == null ? "" : s).toUpperCase(Locale.US).toCharArray()) if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ' ' || c == '-' || c == '_' || c == '.') b.append(c);
        String r = b.toString().trim();
        return r.length() > n ? r.substring(0, n) : r;
    }
    static String findLanIp() {
        try (DatagramSocket s = new DatagramSocket()) { s.connect(InetAddress.getByName("8.8.8.8"), 53); return s.getLocalAddress().getHostAddress(); }
        catch (Exception e) { return ""; }
    }
}

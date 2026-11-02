"""The hall's computer voice (the launcher's in-game AI): Kokoro-82M (Apache-2.0, local, H:/Games/Virtuality/video/tts) + a smooth
lady-robot chain (Bobby 10-31: like the computer in The Lawnmower Man's VR game, "almost like a lady transformer voice but smooth").
Writes package/engine/hall/voice/<line>.ogg; alarm_* lines start with a two-tone klaxon. Read by the Quest launcher (HallVoice.java).
    python quest/tools/make_hall_voice.py                    every line, voice VOICE
    python quest/tools/make_hall_voice.py --candidates       one alarm line in each candidate voice -> voice/_candidates/
"""
import os, sys, subprocess, hashlib
import soundfile as sf

TTS = r"H:\Games\Virtuality\video\tts"
HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "..", "..", "package", "engine", "hall", "voice")
VOICE, SPEED = "bf_emma", 0.94   # Bobby 10-31 picked bf_emma from the candidates
CANDIDATES = ["af_heart", "af_sky", "af_nicole", "af_bella", "bf_emma", "bf_isabella"]

# Bobby 10-31: "the music voice box effect like in those songs ... distorted digital ... synth voice box": a channel vocoder
# (vocode(): the voice's spectrum played on a synth chord, like the vocoder hooks of the menu songs), some dry voice under it so
# the words stay clear, then digital grit (drive + bitcrush), chorus and a short hall.
CHORD = [220.0, 261.63, 329.63, 440.0]       # A minor, A3-A4: a lady's range
DRY, NOISE, DRIVE = 0.32, 0.07, 2.2
FX = ("[0:a]aresample=44100,acrusher=bits=9:mix=0.3:mode=log,chorus=0.7:0.9:25|40:0.3|0.25:0.3|0.45:2|1.2,"
      "aecho=0.8:0.6:9|17:0.22|0.15,aecho=0.8:0.75:120|200:0.2|0.1,"
      "highpass=f=90,lowpass=f=10000,equalizer=f=2800:t=q:w=1:g=3,loudnorm=I=-15:TP=-1.5")


def vocode(x, sr):
    """the voice's band envelope (24 log bands, per 1024-sample frame) on a saw-wave chord + a little noise (for s, t, k)"""
    import numpy as np
    n, hop = 1024, 256
    t = np.arange(len(x) + n) / sr
    car = sum(2 * ((t * f) % 1.0) - 1 for f in CHORD) / len(CHORD) + NOISE * np.random.default_rng(7).standard_normal(len(t))
    win = np.hanning(n)
    edges = np.unique(np.round(np.geomspace(2, n // 2, 25)).astype(int))
    def env(m):                                # band averages, back on every bin
        e = np.empty_like(m)
        for a, b in zip(edges[:-1], edges[1:]): e[..., a:b] = m[..., a:b].mean(axis=-1, keepdims=True)
        e[..., :edges[0]] = e[..., edges[0]:edges[0] + 1]; e[..., edges[-1]:] = e[..., edges[-1] - 1:edges[-1]]
        return e
    xp = np.concatenate([x, np.zeros(n)])
    out = np.zeros(len(xp) + n)
    for i in range(0, len(x), hop):
        V = np.fft.rfft(xp[i:i + n] * win); C = np.fft.rfft(car[i:i + n] * win)
        Y = C * env(np.abs(V)) / (env(np.abs(C)) + 1e-6)
        out[i:i + n] += np.fft.irfft(Y) * win
    out = out[:len(x)]
    out = out / (np.abs(out).max() + 1e-9) * 0.9 + DRY * x / (np.abs(x).max() + 1e-9)
    return np.tanh(DRIVE * out) / np.tanh(DRIVE)
# two-tone klaxon, three bursts, then a beat of quiet before the voice
KLAXON = ("aevalsrc='0.35*sgn(sin(2*PI*(if(lt(mod(t,0.5),0.25),880,660))*t))*lt(mod(t,0.5),0.45)*lt(t,1.5)'"
          ":s=44100:d=1.8,lowpass=f=3000,aecho=0.8:0.5:60:0.2")

# "Mall V R" so Kokoro spells it; GET GAMES etc. are the buttons' own words
LINES = {
    # no games at all (sideload build: GET GAMES)
    "alarm_nogames_1": "Warning. No game files detected. Press GET GAMES to load the arcade.",
    "alarm_nogames_2": "Alert. This pod is empty. Press the red GET GAMES button below.",
    "alarm_nogames_3": "Attention, player. Game library offline. Press GET GAMES to download it.",
    # no games, store build (players bring their own)
    "alarm_nogames_store_1": "Warning. No game files detected. Copy your games into the Mall V R folder on this headset.",
    "alarm_nogames_store_2": "Alert. Game library offline. Connect a USB cable and copy your games folder in.",
    # All files access not given yet
    "alarm_files_1": "Access denied. Mall V R needs file access. Press ALLOW FILE ACCESS.",
    "alarm_files_2": "Warning. Storage locked. Press ALLOW FILE ACCESS, then switch Mall V R on.",
    # a missing game picked
    "missing_1": "That game is not installed. Press GET GAMES.",
    "missing_2": "Game file not found. GET GAMES can download it.",
    "missing_store_1": "That game is not installed. Copy it into the Mall V R folder.",
    "needcd_1": "This game needs its CD image. Press GET GAMES.",
    "needcd_store_1": "This game needs its CD image. Copy it into the game's folder.",
    # the setup hexes' tutorial cards (read when a card opens; hall_info.txt [setup ...] has the full text)
    "tut_files_1": "Step one. Press ALLOW FILE ACCESS. Switch Mall V R on in the list, then come back here.",
    "tut_getgames_1": "Step two. Press GET GAMES, then GET beside each pack. Stay on Wi-Fi and keep the headset awake until it says done.",
    "tut_copygames_1": "Step two. On a PC, go to gainesphere dot com slash get games. Connect this headset with a USB cable, "
                       "and the PC tool copies your games over. Then press CHECK AGAIN.",
    "tut_play_1": "Step three. Pick a game with a cyan edge and press START. To come back here, hold A and B.",
    # once per start, games found
    "welcome_1": "Welcome to Mall V R. Systems online.",
    "welcome_2": "Welcome back, player. Choose your reality.",
    "welcome_3": "Pod systems online. Select a game.",
}

_k = None


def ff(args): subprocess.run([r"C:\ffmpeg\bin\ffmpeg.exe", "-v", "error", "-y"] + args, check=True)


def say(name, text, voice, out_dir):
    global _k
    os.makedirs(out_dir, exist_ok=True)
    key = hashlib.sha1(("%s|%s|%s" % (voice, SPEED, text)).encode()).hexdigest()[:12]
    raw = os.path.join(TTS, "hall_%s_%s_raw.wav" % (voice, key))
    if not os.path.exists(raw):
        if _k is None:
            from kokoro_onnx import Kokoro
            _k = Kokoro(os.path.join(TTS, "kokoro-v1.0.onnx"), os.path.join(TTS, "voices-v1.0.bin"))
        a, sr = _k.create(text, voice=voice, speed=SPEED, lang="en-us")
        sf.write(raw, a, sr)
    a, sr = sf.read(raw)
    voc = raw.replace("_raw.wav", "_voc.wav"); sf.write(voc, vocode(a, sr), sr)
    fxd = raw.replace("_raw.wav", "_fx.wav")
    ff(["-i", voc, "-filter_complex", FX, "-ac", "1", fxd])
    out = os.path.join(out_dir, name + ".ogg")
    if name.startswith("alarm"):
        ff(["-f", "lavfi", "-i", KLAXON, "-i", fxd, "-filter_complex", "[0:a][1:a]concat=n=2:v=0:a=1,aresample=44100", "-ac", "1",
            "-c:a", "libvorbis", "-q:a", "4", out])
    else:
        ff(["-i", fxd, "-ac", "1", "-c:a", "libvorbis", "-q:a", "4", out])
    print(os.path.relpath(out, os.path.join(HERE, "..", "..")))


if __name__ == "__main__":
    if "--candidates" in sys.argv:
        for v in CANDIDATES: say("candidate_" + v, LINES["alarm_nogames_1"], v, os.path.join(OUT, "_candidates"))
    else:
        for n, t in LINES.items(): say(n, t, VOICE, OUT)

"""Vertical 9:16 promo video (1080x1920, 30 fps, ~36 s) for short-video apps.

    sh tools/build_host.sh && python tools/media/make_video.py [out.mp4]

Gameplay is recorded tick by tick through the 640x480 headless runner, and
the runner's --sounds log scores it with the game's own synthesized effects
(recipes read from platform/desktop/audio_win32.c) over a generated beat.
Needs Pillow, numpy and an ffmpeg binary: $FFMPEG, ffmpeg on PATH, or the
imageio-ffmpeg package.
"""
import os
import pathlib
import re
import shutil
import subprocess
import sys
import wave

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from make_promo import (BONE, CALC_MEDIA, FIRE, GOLD, INK, MEDIA, ORANGE, ROOT, STORM,  # noqa: E402
                        background, calculator, font, glow_text, pill, still, text)

W, H, FPS, RATE = 1080, 1920, 30, 44100
RUNNER = ROOT / "build" / ("ad_headless_hd.exe" if os.name == "nt" else "ad_headless_hd")
TMP = ROOT / "build" / "video_tmp"
GAME_Y = 430                      # top of the 1080x960 gameplay window
ZH = ["--lang", "1"]
PLAY = ('-:30 z:2 -:20 D@200/140:12 -:4 1@200/140:2 -:8 3@200/140:2 -:10 R@230/120:14 -:4 '
        '2@230/120:2 -:8 4@230/120:2 -:10 l@175/110:2 l@175/110:30 -:6 r@175/110:2 -:10 '
        'U@150/90:12 -:4 5@150/90:2 -:8 l@140/100:2 l@140/100:30 -:6 6@140/100:2 -:30')

# name, runner args, script, first tick, frames (one per tick)
CLIPS = {
    "wolf": (["--new", "--class", "4", "--preset", "2", "--fast", "0.6", "--sig"], "-:900", 300, 420),
    "bone": (["--new", "--class", "3", "--preset", "0", "--fast", "0.5", "--sig"], "-:1300", 560, 600),
    "fire": (["--new", "--class", "1", "--preset", "0", "--fast", "0.4", "--sig"], "-:900", 0, 600),
    "play": (["--new", "--class", "0", "--fast", "0.25"], PLAY, 30, 150),
}


def ffmpeg_exe():
    exe = os.environ.get("FFMPEG") or shutil.which("ffmpeg")
    if exe:
        return exe
    try:
        import imageio_ffmpeg
    except ImportError:
        sys.exit("ffmpeg not found: set FFMPEG, put ffmpeg on PATH or pip install imageio-ffmpeg")
    return imageio_ffmpeg.get_ffmpeg_exe()


def record(name):
    """Run the headless runner once; returns (frames, [(frame index, sound id, volume)])."""
    args, script, start, count = CLIPS[name]
    prefix, log = TMP / f"{name}_", TMP / f"{name}.snd"
    cmd = [str(RUNNER)] + args + ZH + ["--script", script, "--record", f"{start}:{count}:1:{prefix}",
                                       "--sounds", str(log)]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        sys.exit(f"runner failed for {name}: {r.stderr}")
    frames = [still(p) for p in sorted(TMP.glob(f"{name}_*.png"))]
    sounds = []
    for line in log.read_text().split("\n"):
        if line.strip():
            tick, sid, vol = (int(v) for v in line.split())
            if start <= tick < start + count:
                sounds.append((tick + 1 - start, sid, vol))   # the effect shows on the next frame
    return frames, sounds


def game_window(frame, zoom=1.0):
    """Centre 540x480 of the 640x480 frame, doubled to 1080x960 (crisp pixels)."""
    w = 540 / zoom
    h = 480 / zoom
    box = (320 - w / 2, 240 - h / 2, 320 + w / 2, 240 + h / 2)
    return frame.resize((1080, 960), Image.NEAREST, box=box)


# ------------------------------------------------------------------ audio

WAVES = {"W_SQUARE": 0, "W_TRI": 1, "W_SAW": 2, "W_SINE": 3, "W_NOISE": 4}


def load_recipes():
    """Sound recipes straight from the desktop synth, so the video sounds like the game."""
    ids = re.search(r"typedef enum \{(.*?)SND_COUNT", (ROOT / "src/core/sound.h").read_text(), re.S).group(1)
    order = [s.strip() for s in ids.replace("\n", " ").split(",") if s.strip()]
    src = (ROOT / "platform/desktop/audio_win32.c").read_text()
    layer = r"\{\s*(W_\w+),\s*([\d.]+)f?,\s*([\d.]+)f?,\s*(\d+),\s*(\d+),\s*([\d.]+)f?,\s*(\d+)\s*\}"
    recipes = {}
    for m in re.finditer(r"\[(SND_\w+)\]\s*=\s*\{\s*" + layer + r",\s*" + layer + r"\s*\}", src):
        g = m.groups()
        recipes[order.index(g[0])] = [(WAVES[g[i]], float(g[i + 1]), float(g[i + 2]), int(g[i + 3]),
                                       int(g[i + 4]), float(g[i + 5]), int(g[i + 6])) for i in (1, 8)]
    if len(recipes) != len(order):
        sys.exit(f"parsed {len(recipes)} of {len(order)} sound recipes from audio_win32.c")
    return recipes


def osc(wave, phase, rng):
    p = phase % 1.0
    if wave == 0:
        return np.where(p < 0.5, 0.8, -0.8)
    if wave == 1:
        return np.where(p < 0.5, 4 * p - 1, 3 - 4 * p)
    if wave == 2:
        return 2 * p - 1
    if wave == 3:
        return np.sin(2 * np.pi * p)
    return rng.uniform(-1, 1, p.shape)


def render_layer(layer, rng):
    wave, f0, f1, ms, delay, vol, steps = layer
    n = ms * RATE // 1000
    out = np.zeros(delay * RATE // 1000 + n)
    if n == 0 or vol == 0:
        return out
    t = np.arange(n) / n
    if steps:
        freq = f0 * f1 ** np.minimum((t * steps).astype(int), steps - 1)
    else:
        freq = f0 + (f1 - f0) * t
    env = np.minimum(np.arange(n) / (RATE / 300), 1.0) * (1 - t) ** 2
    out[delay * RATE // 1000:] = osc(wave, np.cumsum(freq) / RATE, rng) * env * vol
    return out


def sfx_bank():
    rng = np.random.default_rng(7)
    bank = {}
    for sid, (a, b) in load_recipes().items():
        la, lb = render_layer(a, rng), render_layer(b, rng)
        clip = np.zeros(max(len(la), len(lb)))
        clip[:len(la)] += la
        clip[:len(lb)] += lb
        bank[sid] = np.clip(clip, -1, 1)
    return bank


def beat(seconds, bpm=124):
    """Dark synth backing: kick on every beat, offbeat hats, a minor bass line."""
    n = int(seconds * RATE)
    out = np.zeros(n)
    rng = np.random.default_rng(3)
    spb = int(RATE * 60 / bpm)
    kick_t = np.arange(int(RATE * 0.25)) / RATE
    kick = np.sin(2 * np.pi * np.cumsum(50 + 110 * np.exp(-kick_t * 30)) / RATE) * np.exp(-kick_t * 9)
    hat = rng.uniform(-1, 1, int(RATE * 0.05)) * np.exp(-np.arange(int(RATE * 0.05)) / 300)
    notes = (55.0, 55.0, 65.41, 49.0)                      # A1 A1 C2 G1, one per bar
    for k, s in enumerate(range(0, n, spb)):
        seg = out[s:s + len(kick)]
        seg += kick[:len(seg)] * 0.55
        h = out[s + spb // 2:s + spb // 2 + len(hat)]
        h += hat[:len(h)] * 0.12
        f = notes[(k // 4) % 4]
        t = np.arange(min(spb, n - s)) / RATE
        out[s:s + len(t)] += np.sign(np.sin(2 * np.pi * f * t)) * 0.07 * np.exp(-t * 3)
    return out


def write_wav(path, samples):
    pcm = (np.clip(samples, -1, 1) * 32000).astype("<i2")
    with wave.open(str(path), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(pcm.tobytes())


# ---------------------------------------------------------------- visuals

def text_layer(s, size, fill, glow, stroke=6):
    """Pre-rendered glowing title as a cropped RGBA image, scaled per frame for pop-ins."""
    f = font(size)
    l, t, r, b = ImageDraw.Draw(Image.new("L", (1, 1))).textbbox((0, 0), s, font=f, stroke_width=stroke)
    pad = 40
    lay = Image.new("RGBA", (r - l + pad * 2, b - t + pad * 2), (0, 0, 0, 0))
    halo = Image.new("RGBA", lay.size, (0, 0, 0, 0))
    ImageDraw.Draw(halo).text((pad - l, pad - t), s, font=f, fill=glow + (255,), stroke_width=stroke + 6,
                              stroke_fill=glow + (255,))
    lay = Image.alpha_composite(lay, halo.filter(ImageFilter.GaussianBlur(14)))
    ImageDraw.Draw(lay).text((pad - l, pad - t), s, font=f, fill=fill, stroke_width=stroke, stroke_fill=INK)
    return lay


def place(img, layer, center, k):
    """Pop-in: scale 1.6 -> 1.0 and fade in over the first 7 frames (k = frames since start)."""
    if k < 0:
        return
    p = min(k / 7, 1.0)
    scale = 1.0 + 0.6 * (1 - p) ** 2
    lay = layer if scale == 1.0 else layer.resize((int(layer.width * scale), int(layer.height * scale)),
                                                  Image.BILINEAR)
    if p < 1:
        lay = lay.copy()
        lay.putalpha(lay.getchannel("A").point(lambda a: int(a * p)))
    img.paste(lay, (int(center[0] - lay.width / 2), int(center[1] - lay.height / 2)), lay)


def flash(img, k, frames=5):
    if k < frames:
        return Image.blend(img, Image.new("RGB", img.size, (255, 250, 235)), 0.7 * (1 - k / frames))
    return img


class Scene:
    """A stretch of the video: frames to show, overlays, and the sounds that go with them."""

    def __init__(self, length, base, game=None, titles=(), sounds=(), stingers=()):
        self.length, self.base, self.game = length, base, game
        self.inset = None               # (frames, (x, y, w, h)) for footage shown inside a drawing
        self.titles = titles            # (layer, (x, y), first frame)
        self.sounds = list(sounds)      # (frame, sound id, volume)
        self.sounds += [(f, sid, 3) for f, sid in stingers]
        punch = [0.0] * length
        hits = {f for f, sid, _ in self.sounds if sid in (1, 3, 4)}   # crit, elite, boss
        for i in range(length):
            punch[i] = 1.0 if i in hits else (punch[i - 1] * 0.8 if i else 0.0)
        self.punch = punch

    def frame(self, i):
        img = self.base.copy()
        if self.game:
            img.paste(game_window(self.game[min(i, len(self.game) - 1)], 1 + 0.07 * self.punch[i]), (0, GAME_Y))
            ImageDraw.Draw(img).rectangle([0, GAME_Y - 6, W, GAME_Y - 1], fill=ORANGE)
            ImageDraw.Draw(img).rectangle([0, GAME_Y + 960, W, GAME_Y + 965], fill=ORANGE)
        if self.inset:
            shots, (x, y, w, h) = self.inset
            img.paste(shots[i % len(shots)].resize((w, h), Image.NEAREST), (x, y))
        for layer, xy, start in self.titles:
            place(img, layer, xy, i - start)
        return flash(img, i)


def base_card(seed, caption, color=ORANGE, sub=None):
    img = background(seed, color, (W, H))
    if caption:
        text(img, (W / 2, 1490), caption, 44, BONE, "mm")
    if sub:
        text(img, (W / 2, 1560), sub, 36, GOLD, "mm", bold=False)
    text(img, (W / 2, 1850), "灰烬深渊 Ashen Depths · 原创像素 ARPG", 30, (150, 140, 150), "mm", bold=False)
    return img


# ----------------------------------------------------------------- script

SND = {name: i for i, name in enumerate(re.search(
    r"typedef enum \{(.*?)SND_COUNT", (ROOT / "src/core/sound.h").read_text(), re.S).group(1)
    .replace("\n", " ").replace(" ", "").strip(",").split(","))}


def cut(clip, first, length):
    frames, sounds = clip
    return (frames[first:first + length],
            [(f - first, sid, vol) for f, sid, vol in sounds if first <= f < first + length])


def best_window(clip, length, avoid=None):
    """First frame of the most hectic stretch, scored by the sounds the game played."""
    weight = {SND["SND_CRIT"]: 3, SND["SND_KILL"]: 1, SND["SND_ELITE_DIE"]: 6, SND["SND_BOSS_DIE"]: 10}
    frames, sounds = clip
    best, best_score = 0, -1.0
    for first in range(0, len(frames) - length + 1, 5):
        if avoid is not None and first < avoid[0] + avoid[1] and avoid[0] < first + length:
            continue
        score = sum(weight.get(sid, 0.2) for f, sid, _ in sounds if first <= f < first + length)
        if score > best_score:
            best, best_score = first, score
    return best


def headline(a, b, color, glow, size_a=118, size_b=74):
    return [(text_layer(a, size_a, color, glow), (W / 2, 200), 0),
            (text_layer(b, size_b, BONE, glow, 5), (W / 2, 335), 6)]


def build_scene(clip, length, title, sub, color, caption, seed, avoid=None):
    frames, sounds = cut(clip, best_window(clip, length, avoid), length)
    return Scene(length, base_card(seed, caption, color), frames, headline(title, sub, color, color), sounds,
                 [(0, SND["SND_BOSS_DIE"])])


def calc_scene():
    gif = CALC_MEDIA / "late_game.gif"
    with Image.open(gif) as im:
        shots = []
        for i in range(im.n_frames):
            im.seek(i)
            shots += [im.convert("RGB")] * 3                # 10 fps clip -> 30 fps
    base = base_card(11, "专为 TI-Nspire CX 计算器打造", ORANGE, "320×240 像素 · Ndless 运行")
    calculator(base, shots[0], 330, 470)
    scene = Scene(75, base, None, headline("掌上暗黑", "计算器版", GOLD, FIRE), (), [(0, SND["SND_STAIRS"])])
    scene.inset = (shots, (370, 518, 340, 255))
    return scene


def stats_scene(dim):
    backdrop = base_card(12, None)
    backdrop.paste(dim.filter(ImageFilter.GaussianBlur(10)), (0, GAME_Y))
    base = Image.blend(base_card(12, None), backdrop, 0.35)
    items = (("6 大职业", 520), ("18 种流派", 760), ("15 幕剧情", 1000), ("300+ 小时", 1240))
    titles = [(text_layer("内容量管饱", 110, GOLD, FIRE), (W / 2, 220), 0)]
    titles += [(text_layer(s, 120, ORANGE, FIRE), (W / 2, y), 10 + k * 14) for k, (s, y) in enumerate(items)]
    stingers = [(10 + k * 14, SND["SND_DROP_RARE"]) for k in range(4)]
    return Scene(90, base, None, titles, (), stingers + [(0, SND["SND_LEVEL"])])


def cta_scene():
    base = base_card(13, "解压，双击 exe 就能玩", ORANGE, "计算器版搜 ti-idle-arpg（需 Ndless）")
    title = still(MEDIA / "desktop_title.png").resize((800, 600), Image.NEAREST)
    ImageDraw.Draw(base).rectangle([134, 694, 946, 1306], fill=ORANGE)
    base.paste(title, (140, 700))
    titles = [(text_layer("免费 · MIT 开源", 110, GOLD, FIRE), (W / 2, 220), 0),
              (text_layer("GitHub 搜 ARPG-CX", 78, BONE, ORANGE, 5), (W / 2, 380), 8)]
    return Scene(105, base, None, titles, (), [(0, SND["SND_DROP_UNIQUE"]), (8, SND["SND_DROP_LEGEND"])])


def scenes():
    TMP.mkdir(parents=True, exist_ok=True)
    wolf, bone, fire, play = (record(n) for n in ("wolf", "bone", "fire", "play"))
    hook_first = best_window(wolf, 90)
    hook_frames, hook_sounds = cut(wolf, hook_first, 90)
    hook = Scene(90, base_card(10, "风暴狼德 vs 空洞之王", STORM), hook_frames,
                 [(text_layer("在计算器上", 120, GOLD, FIRE), (W / 2, 190), 0),
                  (text_layer("做了个暗黑4？", 128, ORANGE, FIRE), (W / 2, 335), 10)],
                 hook_sounds, [(0, SND["SND_BOSS_DIE"]), (10, SND["SND_ELITE_DIE"])])
    pc_frames, pc_sounds = cut(play, 0, 150)
    pc = Scene(150, base_card(14, "空格切换 挂机 / 手动", STORM, "WASD 移动 · 鼠标点怪 · 1-6 技能 · Q 喝药"),
               pc_frames, headline("电脑版", "键盘鼠标亲自打", STORM, STORM), pc_sounds, [(0, SND["SND_UI_OK"])])
    return [hook, calc_scene(), pc,
            build_scene(wolf, 150, "风暴狼德", "每一击劈下天雷 · 永不掉血", STORM, "核心暗金「风暴嚎叫之皮」", 15,
                        (hook_first, 90)),
            build_scene(bone, 150, "骨刺死灵", "骨矛炸裂 · 巨型骨矛震屏", BONE, "核心暗金「初代守护者之脊」", 16),
            build_scene(fire, 150, "陨石火法", "火球双爆 · 陨石如雨", FIRE, "核心暗金「炼狱之心」", 17),
            stats_scene(game_window(fire[0][60])), cta_scene()]


def soundtrack(all_scenes, path):
    total = sum(sc.length for sc in all_scenes) / FPS
    mix = beat(total + 1)[:int(total * RATE)] * 0.8
    bank, gains = sfx_bank(), (0.0, 0.3, 0.6, 1.0)
    t0 = 0
    for sc in all_scenes:
        for f, sid, vol in sc.sounds:
            clip = bank[sid] * gains[min(vol, 3)] * 0.55
            s = int((t0 + f) / FPS * RATE)
            seg = mix[s:s + len(clip)]
            seg += clip[:len(seg)]
        t0 += sc.length
    fade = int(RATE * 0.8)
    mix[-fade:] *= np.linspace(1, 0, fade)
    write_wav(path, np.tanh(mix * 1.2))      # soft clip keeps stacked hits from cracking


def encode(all_scenes, wav, out):
    cmd = [ffmpeg_exe(), "-y", "-loglevel", "error", "-f", "rawvideo", "-pix_fmt", "rgb24", "-s", f"{W}x{H}",
           "-r", str(FPS), "-i", "-", "-i", str(wav), "-c:v", "libx264", "-preset", "slow", "-crf", "17",
           "-pix_fmt", "yuv420p", "-c:a", "aac", "-b:a", "192k", "-shortest", "-movflags", "+faststart", str(out)]
    proc = subprocess.Popen(cmd, stdin=subprocess.PIPE)
    try:
        for sc in all_scenes:
            for i in range(sc.length):
                proc.stdin.write(sc.frame(i).tobytes())
    finally:
        proc.stdin.close()
    if proc.wait() != 0:
        sys.exit("ffmpeg failed to encode the video")


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    out = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "build" / "promo" / "ashen_depths_promo.mp4"
    out.parent.mkdir(parents=True, exist_ok=True)
    if not RUNNER.is_file():
        sys.exit(f"{RUNNER} missing: run sh tools/build_host.sh first")
    shutil.rmtree(TMP, ignore_errors=True)
    all_scenes = scenes()
    wav = TMP / "soundtrack.wav"
    soundtrack(all_scenes, wav)
    encode(all_scenes, wav, out)
    shutil.rmtree(TMP, ignore_errors=True)
    print(out, f"{sum(sc.length for sc in all_scenes) / FPS:.1f}s", f"{out.stat().st_size // 1024} KB")


if __name__ == "__main__":
    main()

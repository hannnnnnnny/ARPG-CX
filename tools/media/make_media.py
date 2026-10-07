"""Regenerate the README screenshots and clips in docs/media (Windows build).

    sh tools/build_host.sh && python tools/media/make_media.py   (project root)
    python tools/media/make_media.py --only season_butcher,season_mythic
    python tools/media/make_media.py --en     (English UI, *_en files for README.md)

Everything is rendered by the game itself through the 640x480 headless
runner (build/ad_headless_hd: the same GFX_HD renderer and 12px CJK font as
AshenDepthsDesktop.exe), frame by frame. Scripted mouse input is replayed
by the runner; the cursor arrow is painted on afterwards at the scripted
position, since the real one belongs to Windows. Needs Pillow.
"""
import glob
import os
import re
import shutil
import subprocess
import sys

from PIL import Image, ImageDraw

RUNNER = os.path.join('build', 'ad_headless_hd.exe' if os.name == 'nt' else 'ad_headless_hd')
OUT = os.path.join('docs', 'media')
TMP = os.path.join('build', 'media_tmp')
GFX_S = 2          # logical 320x240 -> 640x480
EN = '--en' in sys.argv          # English UI, files named *_en (README.md); default Chinese (README.zh-CN.md)
LANG = ['--lang', '0' if EN else '1']
SUFFIX = '_en' if EN else ''

# Play by hand: auto battle, then Space (z) for manual, WASD, skill keys 1-6,
# clicks into the pack.
PLAY = ('-:90 z:2 -:20 D@200/140:12 -:4 1@200/140:2 -:8 3@200/140:2 -:10 R@230/120:14 -:4 '
        '2@230/120:2 -:8 4@230/120:2 -:10 l@175/110:2 l@175/110:30 -:6 r@175/110:2 -:10 '
        'U@150/90:12 -:4 5@150/90:2 -:8 l@140/100:2 l@140/100:30 -:6 6@140/100:2 -:30')
# Menus with the mouse: open, click tabs, click rows, wheel.
MENUS = ('-:40 O:2 -:20 -@60/7:10 l@60/7:2 -:20 -@100/48:10 l@100/48:2 -:20 -@100/70:10 l@100/70:2 -:20 '
         '-@220/7:10 l@220/7:2 -:24 -@300/7:10 l@300/7:2 -:20 '
         '-@120/58:10 l@120/58:2 -:10 l@120/58:2 -:30')

CLIPS = [
    # name, runner args, script, (first tick, frames, ticks per frame), ms per frame
    ('desktop_play', ['--new', '--class', '0', '--fast', '0.25'] + LANG, PLAY, (0, 130, 3), 100),
    ('desktop_menus', ['--new', '--class', '1', '--fast', '1.5'] + LANG, MENUS, (30, 110, 3), 100),
    ('desktop_battle', ['--new', '--class', '3', '--fast', '0.3'] + LANG, '-:400', (0, 125, 2), 66),
    # the three signature builds, wearing their build-defining uniques
    ('meta_storm_werewolf', ['--new', '--class', '4', '--preset', '2', '--fast', '0.6', '--sig'] + LANG, '-:600',
     (180, 120, 3), 100),
    ('meta_bone_spear', ['--new', '--class', '3', '--preset', '0', '--fast', '0.5', '--sig'] + LANG, '-:1100',
     (640, 120, 3), 100),
    ('meta_inferno', ['--new', '--class', '1', '--preset', '0', '--fast', '0.4', '--sig'] + LANG, '-:500',
     (20, 120, 3), 100),
    # season content: the Fleshrender, mythic powers, a blood harvest
    ('season_butcher', ['--new', '--class', '0', '--fast', '1.5', '--butcher'] + LANG, '-:900', (20, 120, 3), 100),
    ('season_mythic', ['--new', '--class', '3', '--preset', '1', '--fast', '1.0', '--mythic', '1,5,9,11,16'] + LANG,
     '-:900', (330, 120, 3), 100),
    ('season_harvest', ['--new', '--class', '0', '--fast', '0.8', '--event', '6'] + LANG, '-:700', (40, 120, 3), 100),
]

SHOTS = [
    ('desktop_title', ['--save', os.path.join(TMP, 'fresh.sav')] + LANG, '-:60', 50),
    ('desktop_battle', ['--new', '--class', '0', '--fast', '0.25'] + LANG, '-:200@200/120:2 -:60', 150),
    ('desktop_hero', ['--new', '--class', '3', '--fast', '1.5'] + LANG, '-:40 T:2 -:30', 70),
    ('desktop_skills', ['--new', '--class', '1', '--fast', '1.5'] + LANG, '-:40 T:2 -:5 T:2 -:5 T:2 -:30', 80),
    ('desktop_goals', ['--new', '--class', '5', '--fast', '6'] + LANG, '-:40' + ' T:2 -:5' * 6 + ' -:30', 105),
    ('desktop_options', ['--new', '--class', '2', '--fast', '1'] + LANG, '-:40 B:2 -:30', 70),
]


def mouse_timeline(script):
    """Tick -> (x, y) of the scripted pointer (None before the first '@')."""
    pos, out = None, []
    for tok in script.split():
        buttons, ticks = tok.rsplit(':', 1)
        m = re.search(r'@(\d+)/(\d+)', buttons)
        if m:
            pos = (int(m.group(1)), int(m.group(2)))
        out += [pos] * int(ticks)
    return out


def draw_cursor(im, pos):
    if pos is None:
        return im
    x, y = pos[0] * GFX_S, pos[1] * GFX_S
    arrow = [(x, y), (x, y + 22), (x + 6, y + 16), (x + 11, y + 26), (x + 15, y + 24), (x + 10, y + 15), (x + 17, y + 15)]
    d = ImageDraw.Draw(im)
    d.polygon(arrow, fill=(255, 255, 255), outline=(0, 0, 0))
    return im


def run(args, script, extra):
    r = subprocess.run([RUNNER] + args + ['--script', script] + extra, capture_output=True, text=True)
    if r.returncode != 0:
        sys.exit(f'runner failed: {args}\n{r.stderr}')


def make_gif(name, frames, ms):
    palette = frames[len(frames) // 2].quantize(colors=255, method=Image.MEDIANCUT)
    q = [f.quantize(palette=palette, dither=Image.NONE) for f in frames]
    path = os.path.join(OUT, f'{name}{SUFFIX}.gif')
    q[0].save(path, save_all=True, append_images=q[1:], duration=ms, loop=0, optimize=True)
    print(f'{path}: {len(frames)} frames, {os.path.getsize(path) // 1024} KB')


def clip(name, args, script, rec, ms):
    prefix = os.path.join(TMP, name + '_')
    run(args, script, ['--record', f'{rec[0]}:{rec[1]}:{rec[2]}:{prefix}'])
    timeline = mouse_timeline(script)
    frames = []
    for i, f in enumerate(sorted(glob.glob(prefix + '*.png'))):
        tick = rec[0] + i * rec[2]
        pos = timeline[tick] if tick < len(timeline) else (timeline[-1] if timeline else None)
        frames.append(draw_cursor(Image.open(f).convert('RGB'), pos))
    make_gif(name, frames, ms)


def shot(name, args, script, tick):
    path = os.path.join(TMP, name + '.png')
    run(args, script, ['--shot', f'{tick}:{path}'])
    timeline = mouse_timeline(script)
    draw_cursor(Image.open(path).convert('RGB'), timeline[tick] if tick < len(timeline) else None) \
        .save(os.path.join(OUT, name + SUFFIX + '.png'))


def main():
    only = sys.argv[sys.argv.index('--only') + 1].split(',') if '--only' in sys.argv else None
    shutil.rmtree(TMP, ignore_errors=True)
    os.makedirs(TMP)
    os.makedirs(OUT, exist_ok=True)
    for c in CLIPS:
        if only is None or c[0] in only:
            clip(*c)
    for s in SHOTS:
        if only is None or s[0] in only:
            shot(*s)
    shutil.rmtree(TMP, ignore_errors=True)


if __name__ == '__main__':
    main()

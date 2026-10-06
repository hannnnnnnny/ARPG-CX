"""Vertical 3:4 promo cards (1080x1440) for social posts, built from the
game's own recorded frames in docs/media of both repos.

    python tools/media/make_promo.py [out_dir]     (default build/promo)
"""
import pathlib
import random
import sys

from PIL import Image, ImageDraw, ImageFilter, ImageFont, ImageSequence

ROOT = pathlib.Path(__file__).resolve().parents[2]
MEDIA = ROOT / "docs" / "media"
CALC_MEDIA = ROOT.parent / "Ashen-Depths" / "docs" / "media"
W, H = 1080, 1440
FONT_DIR = pathlib.Path("C:/Windows/Fonts")
ORANGE, GOLD, BONE, STORM, FIRE = (255, 122, 26), (255, 205, 90), (226, 222, 205), (120, 200, 255), (255, 92, 40)
INK, PANEL = (14, 10, 18), (30, 22, 34)


def font(size, bold=True):
    return ImageFont.truetype(str(FONT_DIR / ("msyhbd.ttc" if bold else "msyh.ttc")), size)


def gif_frame(name, index):
    with Image.open(MEDIA / name) as im:
        for i, fr in enumerate(ImageSequence.Iterator(im)):
            if i == index:
                return fr.convert("RGB")
    raise IndexError(f"{name} has no frame {index}")


def still(path):
    with Image.open(path) as im:
        return im.convert("RGB")


def crop_scale(img, box, width):
    """Crop a region and enlarge it with nearest neighbour to keep pixels crisp."""
    part = img.crop(box)
    return part.resize((width, round(width * part.height / part.width)), Image.NEAREST)


def background(seed, glow=ORANGE):
    """Dark gradient with a warm glow at the top and drifting embers."""
    bg = Image.new("RGB", (W, H), INK)
    d = ImageDraw.Draw(bg)
    for y in range(H):
        t = y / H
        d.line([(0, y), (W, y)], fill=(int(24 - 16 * t), int(14 - 8 * t), int(26 - 16 * t)))
    halo = Image.new("RGB", (W, H), (0, 0, 0))
    ImageDraw.Draw(halo).ellipse([-200, -520, W + 200, 420], fill=tuple(c // 3 for c in glow))
    bg = Image.blend(bg, Image.composite(halo, bg, halo.convert("L").point(lambda v: 255 if v else 0)), 0.55)
    bg = bg.filter(ImageFilter.GaussianBlur(2))
    d = ImageDraw.Draw(bg)
    rnd = random.Random(seed)
    for _ in range(90):
        x, y, r = rnd.randrange(W), rnd.randrange(H), rnd.choice((2, 2, 3, 4))
        d.rectangle([x, y, x + r, y + r], fill=rnd.choice((ORANGE, GOLD, FIRE)))
    return bg


def glow_text(img, xy, text, size, fill, anchor="mm", glow=None, stroke=6):
    """Bold text with a dark outline and a soft coloured glow behind it."""
    f = font(size)
    layer = Image.new("RGBA", img.size, (0, 0, 0, 0))
    ImageDraw.Draw(layer).text(xy, text, font=f, fill=(glow or fill) + (255,), anchor=anchor,
                               stroke_width=stroke + 6, stroke_fill=(glow or fill) + (255,))
    img.paste(layer.filter(ImageFilter.GaussianBlur(14)), (0, 0), layer.filter(ImageFilter.GaussianBlur(14)))
    ImageDraw.Draw(img).text(xy, text, font=f, fill=fill, anchor=anchor, stroke_width=stroke, stroke_fill=INK)


def text(img, xy, s, size, fill=BONE, anchor="la", bold=True, stroke=0):
    ImageDraw.Draw(img).text(xy, s, font=font(size, bold), fill=fill, anchor=anchor,
                             stroke_width=stroke, stroke_fill=INK)


def framed(img, shot, xy, color=ORANGE, border=6):
    """Paste a screenshot with a drop shadow and a coloured border."""
    x, y = xy
    shadow = Image.new("RGBA", (shot.width + 80, shot.height + 80), (0, 0, 0, 0))
    ImageDraw.Draw(shadow).rectangle([40, 52, shot.width + 40, shot.height + 52], fill=(0, 0, 0, 200))
    shadow = shadow.filter(ImageFilter.GaussianBlur(18))
    img.paste(shadow, (x - 40, y - 40), shadow)
    ImageDraw.Draw(img).rectangle([x - border, y - border, x + shot.width + border - 1,
                                   y + shot.height + border - 1], fill=color)
    img.paste(shot, (x, y))


def pill(img, xy, s, size=34, fg=INK, bg=GOLD, anchor="l"):
    """Rounded tag; anchor 'l' places it by its left edge, 'm' by its centre."""
    f = font(size)
    l, t, r, b = ImageDraw.Draw(img).textbbox((0, 0), s, font=f)
    w, h = r - l + size, b - t + size * 0.6
    x, y = xy
    if anchor == "m":
        x -= w / 2
    ImageDraw.Draw(img).rounded_rectangle([x, y, x + w, y + h], radius=h / 2, fill=bg)
    ImageDraw.Draw(img).text((x + w / 2, y + h / 2), s, font=f, fill=fg, anchor="mm")
    return x + w


def card(img, box, title, body, accent):
    """Feature card: accent bar, title line, smaller body line."""
    x0, y0, x1, y1 = box
    d = ImageDraw.Draw(img)
    d.rounded_rectangle(box, radius=18, fill=PANEL)
    d.rounded_rectangle([x0, y0, x0 + 12, y1], radius=6, fill=accent)
    text(img, (x0 + 36, y0 + 22), title, 40, accent)
    text(img, (x0 + 36, y0 + 82), body, 30, BONE, bold=False)


WOLF = ("meta_storm_werewolf.gif", 70, (40, 40, 580, 330))
BONE_HIT = ("meta_bone_spear.gif", 15, (40, 60, 580, 335))
BONE_WIDE = ("meta_bone_spear.gif", 75, (100, 40, 600, 320))
FIRE_RING = ("meta_inferno.gif", 0, (40, 22, 580, 330))
FIRE_HIT = ("meta_inferno.gif", 45, (90, 120, 610, 400))


def shot(pick, width):
    name, index, box = pick
    return crop_scale(gif_frame(name, index), box, width)


def page_cover():
    img = background(1)
    pill(img, (W / 2, 70), "原创独立游戏 · 免费开源", 32, anchor="m")
    glow_text(img, (W / 2, 230), "在计算器上", 128, GOLD, glow=FIRE)
    glow_text(img, (W / 2, 390), "做了个暗黑4", 140, ORANGE, glow=FIRE)
    text(img, (W / 2, 505), "TI-Nspire CX 计算器版 + Windows 电脑版", 40, BONE, "mm")
    hero = shot(WOLF, 960)
    framed(img, hero, (60, 570))
    glow_text(img, (W - 90, 590 + hero.height - 40), "290K!", 96, GOLD, "rs", glow=FIRE)
    y = 590 + hero.height + 40
    x = 60
    for tag, col in (("风暴狼德", STORM), ("骨刺死灵", BONE), ("陨石火法", FIRE)):
        x = pill(img, (x, y), tag, 38, bg=col) + 22
    text(img, (W / 2, y + 150), "挂机自动刷 · 也能键鼠亲自打", 44, BONE, "mm")
    text(img, (W / 2, H - 70), "6 职业 · 18 流派 · 300+ 小时 · 键鼠 + 音效", 40, GOLD, "mm")
    return img


def build_row(img, y, pick, name, line, dmg, color):
    s = shot(pick, 560)
    framed(img, s, (60, y), color)
    text(img, (670, y + 20), name, 60, color)
    for k, part in enumerate(line):
        text(img, (670, y + 116 + k * 50), "• " + part, 34, BONE, bold=False)
    glow_text(img, (60 + 560 - 20, y + s.height - 16), dmg, 72, GOLD, "rs", glow=color)
    return y + s.height


def page_builds():
    img = background(2)
    glow_text(img, (W / 2, 120), "三大版本 T0 流派", 100, GOLD, glow=FIRE)
    text(img, (W / 2, 230), "特效拉满 · 伤害爆炸 · 越打越强", 42, BONE, "mm")
    y = 310
    rows = ((WOLF, "风暴狼德", ("撕碎变闪电", "无限连打", "天雷回血护盾"), "290K", STORM),
            (BONE_HIT, "骨刺死灵", ("骨矛炸成碎骨", "暴击白骨新星", "巨型骨矛震屏"), "406K", BONE),
            (FIRE_RING, "陨石火法", ("火球爆炸两次", "陨石雨从天降", "全屏闪光震动"), "陨石雨!", FIRE))
    for pick, name, line, dmg, col in rows:
        y = build_row(img, y, pick, name, line, dmg, col) + 56
    return img


def page_wolf():
    img = background(3, STORM)
    glow_text(img, (W / 2, 120), "永不掉血的狼德", 104, (220, 240, 255), glow=STORM)
    text(img, (W / 2, 225), "核心暗金「风暴嚎叫之皮」 + 两个专属威能", 38, STORM, "mm")
    framed(img, shot(WOLF, 960), (60, 300), STORM)
    cards = (("撕碎 = 免费闪电", "不耗资源，无限连打"),
             ("每一击都劈下天雷", "分叉连锁，打一下回一口血"),
             ("够不到就扑过去", "80 像素内自动突进，机动拉满"),
             ("生命 ×8 · 减伤 40%", "站撸首领，血条几乎不动"))
    y = 880
    for k, (t, b) in enumerate(cards):
        x0 = 60 + (k % 2) * 490
        card(img, (x0, y + (k // 2) * 190, x0 + 470, y + (k // 2) * 190 + 160), t, b, STORM)
    text(img, (W / 2, H - 80), "首领 30% 掉落 · 每打一个守护者自动升级", 36, GOLD, "mm")
    return img


def half(img, y, pick, title, sub, lines, color):
    s = shot(pick, 860)
    glow_text(img, (110, y + 40), title, 76, color, "lm", glow=color)
    text(img, (W - 110, y + 46), sub, 30, GOLD, "rm")
    framed(img, s, (110, y + 100), color)
    text(img, (W / 2, y + 128 + s.height), "  ·  ".join(lines), 34, BONE, "mt")
    return y + 190 + s.height


def page_bone_fire():
    img = background(4, FIRE)
    y = half(img, 40, BONE_WIDE, "骨刺死灵", "核心暗金「初代守护者之脊」",
             ("碎骨扇形穿透", "白骨新星回血", "每 6 根巨型骨矛"), BONE)
    half(img, y + 10, FIRE_HIT, "陨石火法", "核心暗金「炼狱之心」",
         ("火球双爆", "35% 召唤陨石", "燃烧敌人死亡爆炸"), FIRE)
    return img


def page_content():
    img = background(5)
    glow_text(img, (W / 2, 120), "内容量管饱", 110, GOLD, glow=FIRE)
    text(img, (W / 2, 230), "不是小 demo，是能刷几百小时的完整游戏", 40, BONE, "mm")
    stats = (("6", "职业"), ("18", "预设流派"), ("15", "幕剧情"),
             ("300+", "小时内容"), ("60", "种威能"), ("5", "种语言"))
    for k, (num, label) in enumerate(stats):
        x0, y0 = 60 + (k % 3) * 330, 300 + (k // 3) * 220
        ImageDraw.Draw(img).rounded_rectangle([x0, y0, x0 + 300, y0 + 196], radius=22, fill=PANEL)
        glow_text(img, (x0 + 150, y0 + 82), num, 84, ORANGE, glow=FIRE, stroke=4)
        text(img, (x0 + 150, y0 + 158), label, 36, BONE, "mm")
    left = crop_scale(still(MEDIA / "desktop_hero.png"), (0, 0, 640, 480), 468)
    right = crop_scale(still(MEDIA / "desktop_skills.png"), (0, 0, 640, 480), 468)
    framed(img, left, (60, 790))
    framed(img, right, (552, 790))
    lines = ("技能树 · 巅峰盘 · 威能 · 回火精造 · 宝石雕文",
             "楼层事件 · 悬赏 · 成就 · 离线收益 · 转生")
    for k, s in enumerate(lines):
        text(img, (W / 2, 1210 + k * 60), s, 36, BONE, "mm", bold=False)
    text(img, (W / 2, H - 70), "系统对标暗黑4 · 代码、像素美术、剧情全部原创", 34, GOLD, "mm")
    return img


def calculator(img, screen, x, y):
    """A simple drawn handheld: dark body, bezel, screen, key grid."""
    d = ImageDraw.Draw(img)
    d.rounded_rectangle([x, y, x + 420, y + 700], radius=48, fill=(44, 46, 54), outline=(90, 94, 104), width=4)
    d.rounded_rectangle([x + 24, y + 30, x + 396, y + 340], radius=16, fill=(12, 12, 14))
    img.paste(screen.resize((340, 255), Image.NEAREST), (x + 40, y + 48))
    text(img, (x + 210, y + 320), "TI-Nspire CX", 20, (170, 174, 184), "mm", bold=False)
    d.ellipse([x + 150, y + 370, x + 270, y + 460], fill=(70, 74, 84))
    for r in range(4):
        for c in range(5):
            kx, ky = x + 36 + c * 72, y + 490 + r * 50
            d.rounded_rectangle([kx, ky, kx + 60, ky + 36], radius=8, fill=(64, 68, 78) if r else (210, 120, 40))


def page_platforms():
    img = background(6)
    glow_text(img, (W / 2, 110), "一份代码 两个平台", 96, GOLD, glow=FIRE)
    calc = still(CALC_MEDIA / "title.png") if CALC_MEDIA.is_dir() else still(MEDIA / "desktop_title.png")
    calculator(img, calc, 60, 230)
    text(img, (270, 980), "计算器版", 52, ORANGE, "mm")
    text(img, (270, 1040), "320×240 · Ndless", 32, BONE, "mm", bold=False)
    pc = crop_scale(gif_frame("desktop_play.gif", 60), (0, 0, 640, 480), 520)
    framed(img, pc, (520, 330), STORM)
    text(img, (780, 760), "电脑版", 52, STORM, "mm")
    for k, s in enumerate(("WASD 移动 · 鼠标点怪", "1-6 技能 · Q 喝药", "空格 手动 / 挂机切换", "合成音效 · 12px 清晰中文")):
        text(img, (780, 830 + k * 52), s, 32, BONE, "mm", bold=False)
    glow_text(img, (W / 2, 1200), "想自己打就自己打", 64, BONE, glow=ORANGE, stroke=4)
    glow_text(img, (W / 2, 1300), "想挂机就挂机", 64, BONE, glow=ORANGE, stroke=4)
    return img


def page_download():
    img = background(7)
    glow_text(img, (W / 2, 130), "免费下载", 120, GOLD, glow=FIRE)
    text(img, (W / 2, 250), "MIT 开源 · 无广告 · 无内购", 44, BONE, "mm")
    steps = (("1", "GitHub 搜索", "hannnnnnnny/ARPG-CX"),
             ("2", "点 README 里的", "「一键下载」橙色按钮"),
             ("3", "解压，双击", "AshenDepthsDesktop.exe"))
    for k, (n, a, b) in enumerate(steps):
        y0 = 340 + k * 210
        ImageDraw.Draw(img).rounded_rectangle([60, y0, W - 60, y0 + 180], radius=24, fill=PANEL)
        ImageDraw.Draw(img).ellipse([100, y0 + 40, 200, y0 + 140], fill=ORANGE)
        text(img, (150, y0 + 90), n, 60, INK, "mm")
        text(img, (240, y0 + 40), a, 36, BONE, bold=False)
        text(img, (240, y0 + 92), b, 46, GOLD)
    framed(img, crop_scale(still(MEDIA / "desktop_title.png"), (0, 0, 640, 480), 440), (320, 990))
    text(img, (W / 2, H - 40), "计算器版：GitHub 搜 hannnnnnnny/ti-idle-arpg（需 Ndless）", 30, BONE, "ms", bold=False)
    return img


PAGES = (("01_cover", page_cover), ("02_builds", page_builds), ("03_storm_werewolf", page_wolf),
         ("04_bone_inferno", page_bone_fire), ("05_content", page_content),
         ("06_platforms", page_platforms), ("07_download", page_download))


def main():
    sys.stdout.reconfigure(encoding="utf-8")  # output folder may have a Chinese name
    out = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "build" / "promo"
    out.mkdir(parents=True, exist_ok=True)
    for name, make in PAGES:
        path = out / f"{name}.png"
        make().save(path, optimize=True)
        print(path)


if __name__ == "__main__":
    main()

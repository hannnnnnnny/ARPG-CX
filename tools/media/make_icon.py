"""Build the Windows icon (platform/desktop/app.ico) from the game's own art.

    python tools/media/make_icon.py     (project root, needs Pillow)

The skeleton's first frame is read straight from src/data/art.c and its
colours from palette_color() in src/gfx/sprites.c, so the icon always
matches the shipped sprite. Large sizes get an ember glow behind it and a
bronze frame; 16 and 24 px keep just the sprite so it still reads.
"""
import pathlib
import re

from PIL import Image, ImageDraw, ImageFilter

ROOT = pathlib.Path(__file__).resolve().parents[2]
OUT = ROOT / "platform" / "desktop" / "app.ico"
SIZES = [16, 24, 32, 48, 64, 128, 256]


def palette():
    src = (ROOT / "src" / "gfx" / "sprites.c").read_text(encoding="utf-8")
    body = src[src.index("uint16_t palette_color(char c)"):]
    body = body[:body.index("default:")]
    return {m[0]: tuple(int(v) for v in m[1:]) for m in
            re.findall(r"case '(.)': return RGB565\((\d+), (\d+), (\d+)\);", body)}


def skeleton():
    src = (ROOT / "src" / "data" / "art.c").read_text(encoding="utf-8")
    block = src[src.index("/* Skeleton */"):]
    rows = re.findall(r'"([^"]{16})"', block)[:16]
    assert len(rows) == 16
    return rows


def sprite(rows, pal):
    im = Image.new("RGBA", (16, 16), (0, 0, 0, 0))
    for y, row in enumerate(rows):
        for x, ch in enumerate(row):
            if ch != ".":
                im.putpixel((x, y), pal[ch] + (255,))
    return im.crop(im.getbbox())


def framed(spr, size):
    tile = Image.new("RGBA", (size, size), (22, 14, 14, 255))
    glow = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    m = size // 8
    ImageDraw.Draw(glow).ellipse([m, m * 2, size - m, size], fill=(210, 90, 24, 200))
    tile.alpha_composite(glow.filter(ImageFilter.GaussianBlur(size / 7)))
    k = max(1, (size * 13 // 16) // max(spr.size))           # whole-pixel scale only
    big = spr.resize((spr.width * k, spr.height * k), Image.NEAREST)
    tile.alpha_composite(big, ((size - big.width) // 2, size - big.height - size // 12))
    edge = max(1, size // 32)
    ImageDraw.Draw(tile).rectangle([0, 0, size - 1, size - 1], outline=(214, 128, 52, 255), width=edge)
    return tile


def bare(spr, size):
    tile = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    k = max(1, size // max(spr.size))
    big = spr.resize((spr.width * k, spr.height * k), Image.NEAREST)
    tile.alpha_composite(big, ((size - big.width) // 2, (size - big.height) // 2))
    return tile


def main():
    spr = sprite(skeleton(), palette())
    images = [bare(spr, s) if s <= 24 else framed(spr, s) for s in SIZES]
    images[-1].save(OUT, format="ICO", sizes=[(s, s) for s in SIZES], append_images=images[:-1])
    images[-1].save(ROOT / "build" / "app_icon_preview.png")
    print(OUT, OUT.stat().st_size)


if __name__ == "__main__":
    main()

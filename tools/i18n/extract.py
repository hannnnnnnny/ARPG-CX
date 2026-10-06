"""List every English string literal in the game sources that may reach the
screen (has two upper-case letters in a row and is not a pixel-art row).

    python tools/i18n/extract.py [--missing]   (run from the project root)

With --missing, only strings that have no row in tools/i18n/strings.tsv
are printed, so new text never ships untranslated.
"""
import glob
import os
import re
import sys

ART = re.compile(r'^[.kwsSdrRyYnNgGpPbBoOecfx12345aAtcClLmMhz]+$')
LIT = re.compile(r'"((?:[^"\\\n]|\\.)*)"')
SKIP_FILES = {'art.c', 'art_hero.c', 'art_icons.c', 'font.c'}


def literals():
    found = {}
    files = glob.glob('src/game/*.c') + glob.glob('src/gfx/*.c') + glob.glob('src/data/*.c')
    for path in sorted(files):
        if os.path.basename(path) in SKIP_FILES:
            continue
        text = open(path, encoding='utf-8').read()
        text = re.sub(r'/\*.*?\*/', '', text, flags=re.S)
        text = re.sub(r'#include.*', '', text)
        for m in LIT.finditer(text):
            s = m.group(1).encode('ascii', 'replace').decode('unicode_escape')   # C escapes
            # pixel-art rows are long and use '.' for transparency; a short
            # word like "LONG" only looks like one
            is_art = ART.match(s) and ('.' in s or len(s) >= 8)
            if re.search(r'[A-Z]{2}', s) and not is_art:
                found.setdefault(s, os.path.basename(path))
                if s.startswith('ASPECT OF '):        # legendary item names use the tail
                    found.setdefault(s[7:], os.path.basename(path))
    return found


def known():
    keys = set()
    for path in glob.glob('tools/i18n/*.tsv'):
        for line in open(path, encoding='utf-8'):
            if line.strip() and not line.startswith('//'):
                keys.add(line.split('\t')[0])
    return keys


def main():
    found = literals()
    have = known() if '--missing' in sys.argv else set()
    rows = [(f, s) for s, f in found.items() if s not in have]
    for f, s in sorted(rows):
        print(f'{f}\t{s}')
    print(f'# {len(rows)} strings', file=sys.stderr)


if __name__ == '__main__':
    main()

"""Apply exact-text source edits listed in a patch spec file.

    python tools/i18n/patch.py SPEC

SPEC format (UTF-8):
    @@@ path/to/file.c
    <<<
    old text (exact, may span lines)
    ===
    new text
    >>>
Every old text must occur exactly once in its file, or nothing is written.
"""
import sys


def parse(spec):
    edits, path = [], None
    lines = open(spec, encoding='utf-8').read().split('\n')
    i = 0
    while i < len(lines):
        line = lines[i]
        if line.startswith('@@@ '):
            path = line[4:].strip()
        elif line == '<<<':
            old, i = [], i + 1
            while lines[i] != '===':
                old.append(lines[i]); i += 1
            new, i = [], i + 1
            while lines[i] != '>>>':
                new.append(lines[i]); i += 1
            edits.append((path, '\n'.join(old), '\n'.join(new)))
        i += 1
    return edits


def main():
    edits = parse(sys.argv[1])
    files = {}
    for path, old, new in edits:
        text = files.get(path) or open(path, encoding='utf-8').read()
        n = text.count(old)
        if n != 1:
            sys.exit(f'{path}: expected 1 match, found {n}:\n{old}')
        files[path] = text.replace(old, new)
    for path, text in files.items():
        open(path, 'w', encoding='utf-8', newline='\n').write(text)
    print(f'{len(edits)} edits in {len(files)} files')


if __name__ == '__main__':
    main()

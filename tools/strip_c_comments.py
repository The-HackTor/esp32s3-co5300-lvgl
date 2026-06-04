#!/usr/bin/env python3
import re
import sys
from pathlib import Path


def strip(src):
    out = []
    i = 0
    n = len(src)
    while i < n:
        c = src[i]
        if c == '"':
            j = i + 1
            while j < n and src[j] != '"':
                if src[j] == '\\' and j + 1 < n:
                    j += 2
                else:
                    j += 1
            out.append(src[i:j + 1])
            i = j + 1
            continue
        if c == "'":
            j = i + 1
            while j < n and src[j] != "'":
                if src[j] == '\\' and j + 1 < n:
                    j += 2
                else:
                    j += 1
            out.append(src[i:j + 1])
            i = j + 1
            continue
        if c == '/' and i + 1 < n and src[i + 1] == '*':
            j = src.find('*/', i + 2)
            if j < 0:
                break
            i = j + 2
            continue
        if c == '/' and i + 1 < n and src[i + 1] == '/':
            j = src.find('\n', i)
            if j < 0:
                break
            i = j
            continue
        out.append(c)
        i += 1
    text = ''.join(out)
    text = re.sub(r'[ \t]+\n', '\n', text)
    text = re.sub(r'\n{3,}', '\n\n', text)
    return text.lstrip('\n')


def main():
    if len(sys.argv) < 2:
        sys.exit("usage: strip_c_comments.py FILE [FILE ...]")
    for path in sys.argv[1:]:
        p = Path(path)
        src = p.read_text()
        new = strip(src)
        if new != src:
            p.write_text(new)
            print(f"stripped: {path}")
        else:
            print(f"unchanged: {path}")


if __name__ == "__main__":
    main()

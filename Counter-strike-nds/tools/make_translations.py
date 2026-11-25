#!/usr/bin/env python3
"""Generate source/lang/lang_data.c from tools/translations.csv.

translations.csv (UTF-8, comma separated, can be edited in a spreadsheet):
    en,ru,de,fr,es,it
    one line per text of the game, the English text exactly like in the code.
    An empty cell: the English text is used.

The texts are converted to the font of the game (data/text_bmp.bin, 256 cells
of 8x8 pixels, Windows-1252 layout): European letters use their Windows-1252
cell, Russian letters use the cells of CYRILLIC_CELLS (free cells of the font,
see tools/make_font_template.py) or a Latin letter with the same look.

Run after editing translations.csv:
    python3 tools/make_translations.py
"""
import csv
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LANGUAGES = ["en", "ru", "de", "fr", "es", "it"]
LANGUAGE_NAMES = ["English", "Русский", "Deutsch", "Français", "Español", "Italiano"]

# Russian letters drawn in free cells of the font
CYRILLIC_UPPER = "БГДЖЗИЙЛПУФЦЧШЩЪЫЬЭЮЯ"
CYRILLIC_LOWER = "бвгджзийклмнптфцчшщъыьэюя"
UPPER_CELLS = list(range(1, 10)) + list(range(11, 23))
LOWER_CELLS = (list(range(23, 32)) + [127, 136, 137, 138, 139] +
               list(range(141, 149)) + [150, 151, 152])
CYRILLIC_CELLS = dict(zip(CYRILLIC_UPPER, UPPER_CELLS))
CYRILLIC_CELLS.update(zip(CYRILLIC_LOWER, LOWER_CELLS))
# Russian letters with the same look as a Latin letter (cell of the Latin one)
CYRILLIC_SAME = {
    "А": "A", "В": "B", "Е": "E", "К": "K", "М": "M", "Н": "H", "О": "O",
    "Р": "P", "С": "C", "Т": "T", "Х": "X", "Ё": "Ë",
    "а": "a", "е": "e", "о": "o", "р": "p", "с": "c", "у": "y", "х": "x",
    "ё": "ë",
}
# Other characters (typographic quotes...)
OTHER = {"’": "'", "‘": "'", "“": '"', "”": '"', "–": "-", "—": "-", "…": "..."}

# Cells of the font without a drawing (only checked for the European letters)
EMPTY_CELLS = set(range(0, 32)) | {127, 136, 137, 138, 139, 140, 141, 142, 143,
    144, 145, 146, 147, 148, 150, 151, 152, 153, 154, 155, 157, 158, 160, 162,
    164, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 180, 182, 183, 184,
    185, 186, 187, 188, 189, 190}


def encode(text, where):
    out = bytearray()
    for ch in text:
        ch = OTHER.get(ch, ch)
        if len(ch) > 1:
            out += ch.encode("ascii")
            continue
        if ch == "\n":
            out.append(10)
        elif ch in CYRILLIC_CELLS:
            out.append(CYRILLIC_CELLS[ch])
        elif ch in CYRILLIC_SAME:
            out += CYRILLIC_SAME[ch].encode("cp1252")
        else:
            try:
                b = ch.encode("cp1252")
            except UnicodeEncodeError:
                sys.exit(f"{where}: character '{ch}' is not in the font")
            if b[0] in EMPTY_CELLS:
                print(f"warning: {where}: character '{ch}' has no drawing in the font")
            out += b
    return bytes(out)


def c_string(data):
    s = '"'
    for b in data:
        if 32 <= b < 127 and chr(b) not in '"\\?':
            s += chr(b)
        else:
            s += "\\%03o" % b
    return s + '"'


FORMAT = re.compile(r"%[-+ #0]*[0-9]*(?:\.[0-9]+)?[a-zA-Z%]")


def main():
    rows = []
    with open(os.path.join(ROOT, "tools", "translations.csv"), encoding="utf-8", newline="") as f:
        reader = csv.reader(f)
        header = next(reader)
        if header[:len(LANGUAGES)] != LANGUAGES:
            sys.exit(f"translations.csv: the first line must be {','.join(LANGUAGES)}")
        for number, row in enumerate(reader, start=2):
            if not row or not row[0]:
                continue
            row = (row + [""] * len(LANGUAGES))[:len(LANGUAGES)]
            rows.append((number, row))

    # The English texts must exist in the code (else the translation is never used)
    code = ""
    for folder, _, files in os.walk(os.path.join(ROOT, "source")):
        for name in files:
            if name.endswith(".c") and name != "lang_data.c":
                code += open(os.path.join(folder, name), encoding="utf-8", errors="replace").read()

    seen = set()
    lines = []
    for number, row in rows:
        english = row[0]
        if english in seen:
            sys.exit(f"translations.csv line {number}: '{english}' is twice in the file")
        seen.add(english)
        c_english = english.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n")
        if '"' + c_english + '"' not in code:
            print(f"warning: line {number}: '{english}' not found in the code")
        formats = FORMAT.findall(english)
        cells = []
        for index, text in enumerate(row):
            if index > 0 and text:
                if FORMAT.findall(text) != formats:
                    sys.exit(f"translations.csv line {number} ({LANGUAGES[index]}): the values "
                             f"{FORMAT.findall(text)} must be the same as in English {formats}")
            cells.append(c_string(encode(text, f"line {number} ({LANGUAGES[index]})")) if text else "NULL")
        lines.append("    {" + ", ".join(cells) + "},")

    names = [c_string(encode(n, "language name")) for n in LANGUAGE_NAMES]
    output = os.path.join(ROOT, "source", "lang", "lang_data.c")
    with open(output, "w", encoding="utf-8") as f:
        f.write("// Generated by tools/make_translations.py from tools/translations.csv.\n")
        f.write("// Don't edit this file: edit translations.csv and run the script again.\n\n")
        f.write('#include "lang.h"\n\n')
        f.write("const char *const langStrings[][LANG_COUNT] = {\n")
        f.write("\n".join(lines) + "\n};\n\n")
        f.write(f"const int langStringCount = {len(lines)};\n\n")
        f.write("const char *const langNames[LANG_COUNT] = {" + ", ".join(names) + "};\n")
    print(f"{output}: {len(lines)} texts")


main()

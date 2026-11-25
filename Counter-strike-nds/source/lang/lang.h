// SPDX-License-Identifier: MIT
//
// Languages of the game: the texts are written in English in the code and
// translated when they are printed (TextPrint(), printLongText(), buttons) or
// formatted (TR() on the format of sprintf()).
//
// Translations: tools/translations.csv, then tools/make_translations.py
// (generates lang_data.c).

#ifndef LANG_H_
#define LANG_H_

#include <nds.h>

enum Language
{
    LANG_EN = 0,
    LANG_RU,
    LANG_DE,
    LANG_FR,
    LANG_ES,
    LANG_IT,
    LANG_COUNT
};

// Current language (saved)
extern int currentLanguage;

// Generated data (lang_data.c)
extern const char *const langStrings[][LANG_COUNT];
extern const int langStringCount;
extern const char *const langNames[LANG_COUNT];

// Prepare the translation table (once, at the start)
void langInit(void);

// Change the language (wraps around)
void langSetLanguage(int language);

// Translated text of an English text of the code (the English text itself if
// there is no translation). The result uses the cells of the game font.
const char *TR(const char *english);

// NE_TextPrint() with the translation. A text centered on the screen in
// English stays centered.
int TextPrint(int slot, int x, int y, u32 color, const char *text);

#endif // LANG_H_

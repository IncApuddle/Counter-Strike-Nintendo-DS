// SPDX-License-Identifier: MIT
//
// Languages of the game, see lang.h

#include <stdlib.h>
#include <string.h>

#include <NEMain.h>

#include "lang.h"

int currentLanguage = LANG_EN;

// Hash of the English texts, sorted, with the index of the text
typedef struct
{
    u32 hash;
    u16 index;
} LangHash;

static LangHash *hashes = NULL;

static u32 hashText(const char *text)
{
    // FNV-1a
    u32 hash = 2166136261u;
    while (*text)
    {
        hash ^= (u8)*text++;
        hash *= 16777619u;
    }
    return hash;
}

void langInit(void)
{
    if (hashes != NULL)
        return;

    hashes = malloc(sizeof(LangHash) * langStringCount);
    if (hashes == NULL)
        return;

    // Insertion sort (about 150 texts, done once)
    for (int i = 0; i < langStringCount; i++)
    {
        LangHash entry = {hashText(langStrings[i][LANG_EN]), i};
        int j = i - 1;
        while (j >= 0 && hashes[j].hash > entry.hash)
        {
            hashes[j + 1] = hashes[j];
            j--;
        }
        hashes[j + 1] = entry;
    }
}

void langSetLanguage(int language)
{
    if (language < 0 || language >= LANG_COUNT)
        language = LANG_EN;
    currentLanguage = language;
}

const char *TR(const char *english)
{
    if (currentLanguage == LANG_EN || english == NULL || english[0] == '\0' || hashes == NULL)
        return english;

    u32 hash = hashText(english);

    // First text with this hash
    int min = 0, max = langStringCount;
    while (min < max)
    {
        int middle = (min + max) / 2;
        if (hashes[middle].hash < hash)
            min = middle + 1;
        else
            max = middle;
    }

    for (int i = min; i < langStringCount && hashes[i].hash == hash; i++)
    {
        const char *const *texts = langStrings[hashes[i].index];
        if (strcmp(texts[LANG_EN], english) == 0)
        {
            const char *translated = texts[currentLanguage];
            return translated != NULL ? translated : english;
        }
    }
    return english;
}

int TextPrint(int slot, int x, int y, u32 color, const char *text)
{
    const char *translated = TR(text);
    if (translated != text && strchr(text, '\n') == NULL)
    {
        // The English text was centered on the screen (32 columns): center
        // the translation too
        int length = strlen(text);
        int center = (32 - length) / 2;
        if (x >= center - 1 && x <= center + 1 && length > 1)
        {
            x = (32 - (int)strlen(translated)) / 2;
            if (x < 0)
                x = 0;
        }
    }
    return NE_TextPrint(slot, x, y, color, translated);
}

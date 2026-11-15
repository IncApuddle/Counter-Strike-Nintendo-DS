// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021-2022, Fewnity - Grégory Machefer
//
// This file is part of Counter Strike Nintendo DS Multiplayer Edition (CS:DS)

#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include "debug.h"
#include "input.h"

// File system
#include <fat.h>

#include <NEMain.h>

// File
FILE *debugFile;

// Enable debug text at top screen?
bool isDebugTopScreen = false;
// Enable debug text at bottom screen?
bool isDebugBottomScreen = false;
bool isDebugMode = true;

void initDebug()
{
    if (isDebugMode)
    {
        // Set debug mode if keys are pressed
        if (keys & KEY_START)
            isDebugTopScreen = true;

        if (keys & KEY_SELECT)
        {
            isDebugBottomScreen = true;
            //  Init debug console
            consoleDemoInit();
        }
    }
}

/**
 * @brief Print a message in the file
 *
 * @param text
 */
void debugPrint(const char *text)
{
    // Open the file
    if (debugFile == NULL)
    {
        debugFile = fopen("fat:/debug_cs.log", "a");
    }

    // Print the message and a new line
    fprintf(debugFile, "%s", text);
    fprintf(debugFile, "\n");

    // Close the file
    if (debugFile != NULL)
    {
        fclose(debugFile);
    }
}

/**
 * @brief Print Nitro Engine error in debug mode
 *
 * @param text Text to print
 */
void error_handler(const char *text)
{
    // Simple handler. You could write this to a file instead, for example.
    printf("%s", text);

    // Write in the file
    // debugPrint(text);
}

// Boot log
// --------
//
// Writes the progress of the boot process to "cs_boot.log" in the root of the
// SD card (if it can be used). It also prints it to the debug console (hold
// SELECT while the game starts to see it on the bottom screen). Every line is
// flushed, so if the game freezes the last line shows where it stopped.

static bool bootLogEnabled = false;
static FILE *bootLogFile = NULL;
static int bootLogLines = 0;

void bootLogEnable(void)
{
    bootLogEnabled = true;
}

void bootLog(const char *text)
{
    printf("%s\n", text);

    // Don't fill the SD card if something is called every frame
    if (!bootLogEnabled || bootLogLines > 200)
        return;

    if (bootLogFile == NULL)
    {
        bootLogFile = fopen("fat:/cs_boot.log", "w");
        if (bootLogFile == NULL)
            bootLogFile = fopen("sd:/cs_boot.log", "w");
        if (bootLogFile == NULL)
        {
            bootLogEnabled = false;
            return;
        }
    }

    fprintf(bootLogFile, "%s\n", text);
    fflush(bootLogFile);
    // fflush() only empties the buffer of the C library. fsync() makes the FAT
    // driver write its cache to the SD card, so the log survives a freeze.
    fsync(fileno(bootLogFile));
    bootLogLines++;
}

// Network log
// -----------
//
// Writes what happens with the connection to the server to "cs_net.log" (root
// of the SD card). Used to find why the game can't connect to a server.

static FILE *netLogFile = NULL;
static int netLogLines = 0;

void netLog(const char *format, ...)
{
    if (netLogLines > 1000)
        return;

    if (netLogFile == NULL)
    {
        netLogFile = fopen("fat:/cs_net.log", "w");
        if (netLogFile == NULL)
            netLogFile = fopen("sd:/cs_net.log", "w");
        if (netLogFile == NULL)
        {
            netLogLines = 1000; // Don't try again
            return;
        }
    }

    va_list args;
    va_start(args, format);
    vfprintf(netLogFile, format, args);
    va_end(args);
    fputc('\n', netLogFile);

    fflush(netLogFile);
    fsync(fileno(netLogFile));
    netLogLines++;
}

// Writes the current step of the network loop to "cs_stage.txt". The file is
// overwritten every time, so if the game freezes it contains the step where
// it stopped and the number of frames before that.
static FILE *netStageFile = NULL;
static unsigned int netStageCount = 0;

// Writing the file several times per frame makes the game slow, enable it only
// to find where the game freezes.
static bool netStageEnabled = false;

void netStageEnable(bool enable)
{
    netStageEnabled = enable;
}

void netStage(const char *stage)
{
    if (!netStageEnabled)
        return;

    if (netStageFile == NULL)
    {
        netStageFile = fopen("fat:/cs_stage.txt", "w");
        if (netStageFile == NULL)
            netStageFile = fopen("sd:/cs_stage.txt", "w");
        if (netStageFile == NULL)
            return;
    }

    netStageCount++;
    fseek(netStageFile, 0, SEEK_SET);
    // Fixed width so the old text is always overwritten
    fprintf(netStageFile, "%10u %-40.40s\n", netStageCount, stage);
    fflush(netStageFile);
    fsync(fileno(netStageFile));
}

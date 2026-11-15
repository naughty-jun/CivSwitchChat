#pragma once

#include <stdbool.h>

/*
 * File logger. Creates "YYYYMMDD_HHMMSS.log"
 * on every launch and appends each log line to it.
 *
 * `nro_path` is argv[0]. When it has no directory part, "sdmc:/switch" is used.
 * Returns false if the file could not be opened; logging then stays in memory only.
 */
bool appLoggerOpen(const char* nro_path);

/* Path of the opened log file, or NULL when no file is open. */
const char* appLoggerGetPath(void);

/* Writes one line with a [HH:MM:SS] prefix. Does nothing when no file is open. */
void appLoggerWrite(const char* line);

void appLoggerClose(void);

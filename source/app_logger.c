#include "app_logger.h"
#include "app_paths.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#define LOG_DIRECTORY_NAME   "civswitchchat_logs"
#define DIRECTORY_MODE       0777

#define PATH_BUFFER_SIZE     512
#define TIME_BUFFER_SIZE     32

#define FILE_NAME_TIME_FORMAT "%Y%m%d_%H%M%S"
#define LINE_TIME_FORMAT      "%H:%M:%S"
#define FALLBACK_FILE_NAME    "unknown_time"
#define LOG_FILE_EXTENSION    ".log"

static FILE* g_log_file = NULL;
static char  g_log_path[PATH_BUFFER_SIZE];

static bool ensureDirectory(const char* path)
{
    if (mkdir(path, DIRECTORY_MODE) == 0)
        return true;

    return errno == EEXIST;
}

/* ---- Time ---- */

static bool formatCurrentTime(char* output, size_t output_size, const char* format)
{
    time_t now = time(NULL);
    if (now == (time_t)-1)
        return false;

    struct tm* local = localtime(&now);
    if (!local)
        return false;

    return strftime(output, output_size, format, local) > 0;
}

static bool buildLogFilePath(const char* directory, char* output, size_t output_size)
{
    char stamp[TIME_BUFFER_SIZE];

    if (!formatCurrentTime(stamp, sizeof(stamp), FILE_NAME_TIME_FORMAT))
        snprintf(stamp, sizeof(stamp), "%s", FALLBACK_FILE_NAME);

    int written = snprintf(
        output, output_size, "%s%c%s%s",
        directory, APP_PATH_SEPARATOR, stamp, LOG_FILE_EXTENSION
    );

    return written > 0 && (size_t)written < output_size;
}

/* ---- Public API ---- */

bool appLoggerOpen(const char* nro_path)
{
    if (g_log_file)
        return true;

    char base_directory[PATH_BUFFER_SIZE];
    char log_directory[PATH_BUFFER_SIZE];

    if (!appPathsGetNroDirectory(nro_path, base_directory, sizeof(base_directory)))
        return false;

    int written = snprintf(
        log_directory, sizeof(log_directory), "%s%c%s",
        base_directory, APP_PATH_SEPARATOR, LOG_DIRECTORY_NAME
    );
    if (written <= 0 || (size_t)written >= sizeof(log_directory))
        return false;

    if (!ensureDirectory(log_directory))
        return false;

    if (!buildLogFilePath(log_directory, g_log_path, sizeof(g_log_path)))
        return false;

    g_log_file = fopen(g_log_path, "a");
    return g_log_file != NULL;
}

const char* appLoggerGetPath(void)
{
    return g_log_file ? g_log_path : NULL;
}

void appLoggerWrite(const char* line)
{
    if (!g_log_file || !line)
        return;

    char stamp[TIME_BUFFER_SIZE];

    if (formatCurrentTime(stamp, sizeof(stamp), LINE_TIME_FORMAT))
        fprintf(g_log_file, "[%s] %s\n", stamp, line);
    else
        fprintf(g_log_file, "%s\n", line);

    /* Flush every line so the log survives a crash or a forced exit. */
    fflush(g_log_file);
}

void appLoggerClose(void)
{
    if (!g_log_file)
        return;

    fclose(g_log_file);
    g_log_file = NULL;
}

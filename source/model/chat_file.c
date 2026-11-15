#include "chat_file.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "../app_paths.h"

#define TXT_EXTENSION ".txt"

static bool hasTxtExtension(const char* name)
{
    size_t name_len = strlen(name);
    size_t ext_len = strlen(TXT_EXTENSION);

    if (name_len <= ext_len)
        return false;

    return strcasecmp(name + name_len - ext_len, TXT_EXTENSION) == 0;
}

static int compareEntries(const void* a, const void* b)
{
    return strcasecmp(((const ChatFileEntry*)a)->name, ((const ChatFileEntry*)b)->name);
}

bool chatFileListTxtFiles(const char* directory, ChatFileList* out_list)
{
    memset(out_list, 0, sizeof(*out_list));

    DIR* dir = opendir(directory);
    if (!dir)
        return false;

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (!hasTxtExtension(entry->d_name))
            continue;

        if (out_list->count >= CHAT_FILE_LIST_MAX) {
            out_list->truncated = true;
            continue;
        }

        snprintf(
            out_list->entries[out_list->count].name,
            CHAT_FILE_NAME_MAX,
            "%s",
            entry->d_name
        );
        out_list->count++;
    }

    closedir(dir);

    qsort(out_list->entries, (size_t)out_list->count, sizeof(ChatFileEntry), compareEntries);
    return true;
}

static size_t trimLine(char* line)
{
    size_t len = strlen(line);

    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r' || line[len - 1] == ' ' || line[len - 1] == '\t')) {
        line[--len] = '\0';
    }

    return len;
}

static size_t getFileSize(FILE* file)
{
    long original = ftell(file);
    if (original < 0 || fseek(file, 0, SEEK_END) != 0)
        return 0;

    long size = ftell(file);
    fseek(file, original, SEEK_SET);

    return size > 0 ? (size_t)size : 0;
}

/* Reads non-empty, trimmed lines from `file` into out_content, up to its capacity. */
static void readLinesInto(FILE* file, ChatFileContent* out_content)
{
    char raw_line[CHAT_FILE_LINE_MAX];

    while (fgets(raw_line, sizeof(raw_line), file)) {
        if (trimLine(raw_line) == 0)
            continue;

        if (out_content->line_count >= CHAT_FILE_LINE_COUNT_MAX) {
            out_content->line_count_exceeds_limit = true;
            continue;
        }

        snprintf(
            out_content->lines[out_content->line_count],
            CHAT_FILE_LINE_MAX,
            "%s",
            raw_line
        );
        out_content->line_count++;
    }
}

bool chatFileRead(const char* directory, const char* file_name, ChatFileContent* out_content)
{
    memset(out_content, 0, sizeof(*out_content));

    char path[CHAT_FILE_NAME_MAX * 2];
    snprintf(path, sizeof(path), "%s%c%s", directory, APP_PATH_SEPARATOR, file_name);

    FILE* file = fopen(path, "r");
    if (!file)
        return false;

    out_content->file_size = getFileSize(file);

    if (out_content->file_size > CHAT_FILE_SIZE_MAX_BYTES) {
        out_content->size_rejected = true;
        fclose(file);
        return false;
    }

    readLinesInto(file, out_content);

    fclose(file);
    return true;
}

bool chatFileNeedsSizeWarning(size_t file_size)
{
    return file_size > CHAT_FILE_SIZE_WARN_BYTES;
}

bool chatFileNeedsLineCountWarning(s32 line_count)
{
    return line_count >= CHAT_FILE_LINE_WARN_COUNT;
}

bool chatFileLineTooLongToSend(const char* line)
{
    return strlen(line) > CHAT_LINE_SEND_MAX_LENGTH;
}

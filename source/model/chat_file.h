#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <switch.h>

#define CHAT_FILE_NAME_MAX      256
#define CHAT_FILE_LIST_MAX      64
#define CHAT_FILE_LINE_MAX      256   /* Including the NUL terminator. */
#define CHAT_FILE_LINE_COUNT_MAX 1000 /* Lines beyond this count are dropped while reading. */
#define CHAT_FILE_LINE_WARN_COUNT 200 /* Files at or above this many lines need a warning. */
#define CHAT_FILE_SIZE_WARN_BYTES (100 * 1024) /* Files at or below this size need no warning. */
#define CHAT_FILE_SIZE_MAX_BYTES  (1024 * 1024) /* Files above this size are refused outright. */
#define CHAT_LINE_SEND_MAX_LENGTH 255 /* Lines longer than this are not sent. */

typedef struct {
    char name[CHAT_FILE_NAME_MAX];
} ChatFileEntry;

typedef struct {
    ChatFileEntry entries[CHAT_FILE_LIST_MAX];
    s32 count;
    bool truncated; /* More than CHAT_FILE_LIST_MAX .txt files exist. */
} ChatFileList;

/* Lists the ".txt" files directly inside `directory`, sorted by name. */
bool chatFileListTxtFiles(const char* directory, ChatFileList* out_list);

typedef struct {
    char lines[CHAT_FILE_LINE_COUNT_MAX][CHAT_FILE_LINE_MAX];
    s32 line_count;
    size_t file_size;
    bool line_count_exceeds_limit; /* The file has more non-empty lines than fit in `lines`. */
    bool size_rejected;            /* file_size > CHAT_FILE_SIZE_MAX_BYTES; nothing was read. */
} ChatFileContent;

/*
 * Reads `directory/file_name` and splits it into non-empty, trimmed lines.
 * Returns false when the file could not be opened, or when it is larger than
 * CHAT_FILE_SIZE_MAX_BYTES (out_content->size_rejected is set in that case,
 * with file_size filled in and no lines read).
 */
bool chatFileRead(const char* directory, const char* file_name, ChatFileContent* out_content);

/* True when the file should be flagged to the user before sending. */
bool chatFileNeedsSizeWarning(size_t file_size);
bool chatFileNeedsLineCountWarning(s32 line_count);

/* True when `line` is too long to send and must be skipped. */
bool chatFileLineTooLongToSend(const char* line);

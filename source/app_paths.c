#include "app_paths.h"

#include <stdio.h>
#include <string.h>

#define DEFAULT_BASE_DIR "sdmc:/switch"

bool appPathsGetNroDirectory(const char* nro_path, char* output, size_t output_size)
{
    const char* last_separator = nro_path ? strrchr(nro_path, APP_PATH_SEPARATOR) : NULL;

    if (!last_separator) {
        int written = snprintf(output, output_size, "%s", DEFAULT_BASE_DIR);
        return written > 0 && (size_t)written < output_size;
    }

    size_t length = (size_t)(last_separator - nro_path);
    if (length >= output_size)
        return false;

    memcpy(output, nro_path, length);
    output[length] = '\0';
    return true;
}

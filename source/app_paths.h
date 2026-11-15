#pragma once

#include <stdbool.h>
#include <stddef.h>

#define APP_PATH_SEPARATOR '/'

/*
 * Directory part of `nro_path` (normally argv[0]), without the trailing
 * separator. Falls back to "sdmc:/switch" when `nro_path` has no directory
 * part (NULL, empty, or no separator).
 */
bool appPathsGetNroDirectory(const char* nro_path, char* output, size_t output_size);

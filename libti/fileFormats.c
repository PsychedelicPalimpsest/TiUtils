#include "fileFormats.h"
#include <string.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

char endswith(const char *str, const char *substr) {
    size_t len_str = strlen(str);
    size_t len_sub = strlen(substr);
    return len_sub <= len_str &&
           0 == strncmp(str + len_str - len_sub, substr, len_sub);
}


enum FileFormat lookup_format(const char *filename) {
#   pragma unroll
    for (int i = 0; i < ARRAY_SIZE(FORMATS); i++) {
        for (int j = 0; FORMATS[i].format_id[j][0]; j++) {
            if (endswith(filename, FORMATS[i].format_id[j])) {
                return FORMATS[i].format;
            }
        }
    }
    return FMT_UNKNOWN;
}
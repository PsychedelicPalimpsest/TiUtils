#include "makefile.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>


char* strlist_join(struct StrList* strlist, const char* sep) {
    size_t alloc = strlist->head * 16 + 16;
    char* new_str = malloc(alloc);
    new_str[0] = 0;

    size_t pos = 0;

    size_t sep_len = strlen(sep);

    for (int i = 0; i < strlist->head; i++) {
        size_t len = strlen(strlist->strings[i]);
        if (len + sep_len + pos + 1 >= alloc)
            new_str = realloc(new_str, alloc *= 2);

        memcpy(new_str + pos, strlist->strings[i], len);
        pos += len;

        if (i + 1 != strlist->head) {
            memcpy(new_str + pos, sep, sep_len);
            pos += sep_len;
        }
        new_str[pos] = 0;
    }
    return new_str;
}



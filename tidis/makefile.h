#pragma once
#include <stddef.h>
#include <stdlib.h>
#include <string.h>


/* Note: string ptrs not in use are undefined, meaning any AT OR AFTER the head */
struct StrList {
    size_t alloc;
    size_t head;

    char* strings[];
};


static __always_inline
struct StrList* _strlist_allocate(size_t size) {
    struct StrList* strlist = malloc(sizeof(struct StrList) + size * sizeof(char*));
    strlist->alloc = size;
    strlist->head = 0;

    return strlist;
}

#define strlist_new() _strlist_allocate(128)

static __always_inline
void strlist_append(struct StrList** strlist, char* str) {
    if ((*strlist)->head >= (*strlist)->alloc) {
        (*strlist)->alloc *= 2;
        *strlist = realloc(*strlist, (*strlist)->alloc);
    }
    struct StrList* l = *strlist;
    l->strings[l->head++] = str;
}

static __always_inline
void strlist_pop(struct StrList* strlist) {
    if (strlist->head == 0) return;
    strlist->head--;
}

static __always_inline
void strlist_free(struct StrList* strlist) {
    for (int i = 0; i < strlist->head; i++) {
        free(strlist->strings[i]);
    }
    free(strlist);
}

char* strlist_join(struct StrList* strlist, const char* sep);


struct MakefileVariable {
    const char* name;

    /* Space joined */
    struct StrList* value;
};

struct MakefileRule {
    const char* output;

    /* Space joined */
    struct StrList* inputs;

    /* Newline joined */
    struct StrList* lines;
};


struct Makefile {
    int variable_head;
    int rules_head;

    struct MakefileVariable variables[64];
    struct MakefileRule rules[64];
};


static __always_inline
struct Makefile* makefile_new() { return calloc(1, sizeof(struct Makefile)); }

static __always_inline
void makefile_free(struct Makefile* makefile) {
    for (int i = 0; i < makefile->variable_head; i++)
        strlist_free(makefile->variables[i].value);
    for (int i = 0; i < makefile->rules_head; i++) {
        strlist_free(makefile->rules[i].inputs);
        strlist_free(makefile->rules[i].lines);
    }
    free(makefile);
}
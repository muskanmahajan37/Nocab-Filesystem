#include <string.h>
#include <stdlib.h>
#include <alloca.h>

///////
// remove
#include <stdio.h>

#include "slist.h"

slist*
s_cons(const char* text, slist* rest)
{
    //printf("  s_cons @\"%s\"\n", text);
    slist* xs = malloc(sizeof(slist));
    //printf("  s_cons flag 1\n");
    xs->data = strdup(text);
    //printf("  s_cons flag 2\n");
    xs->refs = 1;
    //printf("  s_cons flag 3\n");
    xs->next = rest;
    //printf("  s_cons exit\n");
    return xs;
}

void
s_free(slist* xs)
{
    if (xs == 0) {
        return;
    }

    xs->refs -= 1;

    if (xs->refs == 0) {
        s_free(xs->next);
        free(xs->data);
        free(xs);
    }
}

slist*
s_split(const char* text, char delim)
{
	printf("s_split enter @:\"%s\"\n", text);
    if (*text == 0 || text[0] == '\0') {
        return 0;
    }

    //printf("s_split flag 1\n");
    int plen = 0;
    while (text[plen] != 0 && text[plen] != delim) {
        plen += 1;
    }

    printf("s_split flag 2\n");

    int skip = 0;
    if (text[plen] == delim) {
        skip = 1;
    }

    //printf("s_split flag 3\n");

    slist* rest = s_split(text + plen + skip, delim);
    char*  part = alloca(plen + 2);
    memcpy(part, text, plen);

    printf("s_split flag 4\n");
    part[plen] = 0;
    
    //printf("s_split adding part:\"%s\"\n", part);
    printf("!!!!!s_split assigning cons:\"%s\"\n", part);
    slist* result = s_cons(part, rest);
    printf("!!!!!!!!s_split returning:\"%s\"\n", result->data);
    return result;
}


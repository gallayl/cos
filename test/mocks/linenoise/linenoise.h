#pragma once

#include <stddef.h>

typedef struct linenoiseCompletions
{
    size_t len;
    char **cvec;
} linenoiseCompletions;

void linenoiseAddCompletion(linenoiseCompletions *completions, const char *text);

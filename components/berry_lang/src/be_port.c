#include "be_sys.h"
#include "berry.h"

#include <stdio.h>

BERRY_API void be_writebuffer(const char *buffer, size_t length)
{
    be_fwrite(stdout, buffer, length);
}

BERRY_API char *
be_readstring(char *buffer, // NOLINT(readability-non-const-parameter) - output buffer required by Berry port API
              size_t size)
{
    return be_fgets(stdin, buffer, (int)size);
}

void *be_fopen(const char *filename, const char *modes)
{
    return fopen(filename, modes);
}

int be_fclose(void *hfile)
{
    return fclose(hfile);
}

size_t be_fwrite(void *hfile, const void *buffer, size_t length)
{
    return fwrite(buffer, 1, length, hfile);
}

size_t be_fread(void *hfile, void *buffer, size_t length)
{
    return fread(buffer, 1, length, hfile);
}

char *be_fgets(void *hfile, void *buffer, int size)
{
    return fgets(buffer, size, hfile);
}

int be_fseek(void *hfile, long offset)
{
    return fseek(hfile, offset, SEEK_SET);
}

long int be_ftell(void *hfile)
{
    return ftell(hfile);
}

long int be_fflush(void *hfile)
{
    return fflush(hfile);
}

size_t be_fsize(void *hfile)
{
    long int size;
    long int offset = be_ftell(hfile);
    if (offset < 0 || fseek(hfile, 0L, SEEK_END) != 0)
    {
        return 0;
    }

    size = ftell(hfile);
    if (fseek(hfile, offset, SEEK_SET) != 0 || size < 0)
    {
        return 0;
    }
    return (size_t)size;
}

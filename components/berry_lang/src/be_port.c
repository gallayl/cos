#include "be_sys.h"
#include "berry.h"

#include <stdio.h>

BERRY_API void be_writebuffer(const char *buffer, size_t length)
{
    be_fwrite(stdout, buffer, length);
}

BERRY_API char *be_readstring(char *buffer, size_t size)
{
    return be_fgets(stdin, buffer, (int)size);
}

void *be_fopen(const char *filename, const char *modes)
{
    return fopen(filename, modes);
}

int be_fclose(void *handle)
{
    return fclose(handle);
}

size_t be_fwrite(void *handle, const void *buffer, size_t length)
{
    return fwrite(buffer, 1, length, handle);
}

size_t be_fread(void *handle, void *buffer, size_t length)
{
    return fread(buffer, 1, length, handle);
}

char *be_fgets(void *handle, void *buffer, int size)
{
    return fgets(buffer, size, handle);
}

int be_fseek(void *handle, long offset)
{
    return fseek(handle, offset, SEEK_SET);
}

long int be_ftell(void *handle)
{
    return ftell(handle);
}

long int be_fflush(void *handle)
{
    return fflush(handle);
}

size_t be_fsize(void *handle)
{
    long int size;
    long int offset = be_ftell(handle);
    if (offset < 0 || fseek(handle, 0L, SEEK_END) != 0)
    {
        return 0;
    }

    size = ftell(handle);
    if (fseek(handle, offset, SEEK_SET) != 0 || size < 0)
    {
        return 0;
    }
    return (size_t)size;
}

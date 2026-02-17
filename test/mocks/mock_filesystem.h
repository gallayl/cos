#pragma once

#include <stdbool.h>
#include <stddef.h>

void mock_filesystem_reset(void);
void mock_filesystem_set_sd_mounted(bool mounted);
void mock_filesystem_add_directory(const char *path);

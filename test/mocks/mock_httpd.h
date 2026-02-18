#pragma once

#include <stddef.h>

void mock_httpd_reset(void);

/** Set the value that httpd_req_get_hdr_value_str returns for the next call. */
void mock_httpd_set_header(const char *field, const char *value);

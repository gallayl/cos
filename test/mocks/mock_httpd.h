#pragma once

#include <stddef.h>
#include <stdint.h>

void mock_httpd_reset(void);

/** Set the value that httpd_req_get_hdr_value_str returns for the next call. */
void mock_httpd_set_header(const char *field, const char *value);

/** Set request body data returned by httpd_req_recv in sequence. */
void mock_httpd_set_recv_body(const uint8_t *data, size_t len);

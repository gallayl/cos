#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef void (*multipart_field_cb)(const char *name, const char *value, void *ctx);
typedef bool (*multipart_file_cb)(const char *field_name, const char *file_name, const uint8_t *data, size_t len,
                                  bool is_first, bool is_final, void *ctx);

esp_err_t http_multipart_parse(httpd_req_t *req, multipart_field_cb on_field, multipart_file_cb on_file, void *ctx);

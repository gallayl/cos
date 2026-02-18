#pragma once

void mock_sntp_reset(void);

/** Check whether esp_sntp_init() was called. */
int mock_sntp_get_init_count(void);

/** Get the server name set via esp_sntp_setservername(). */
const char *mock_sntp_get_server(void);

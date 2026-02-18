#include "mock_sntp.h"
#include "esp_sntp.h"

#include <stddef.h>

static int s_init_count = 0;
static const char *s_server = NULL;

void mock_sntp_reset(void)
{
    s_init_count = 0;
    s_server = NULL;
}

int mock_sntp_get_init_count(void)
{
    return s_init_count;
}

const char *mock_sntp_get_server(void)
{
    return s_server;
}

void esp_sntp_setoperatingmode(esp_sntp_operatingmode_t operating_mode)
{
    (void)operating_mode;
}

void esp_sntp_setservername(int index, const char *server)
{
    (void)index;
    s_server = server;
}

void esp_sntp_init(void)
{
    s_init_count++;
}

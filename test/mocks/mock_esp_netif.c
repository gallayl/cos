#include "mock_esp_netif.h"
#include "esp_netif.h"

#include <string.h>

static struct esp_netif_obj s_sta_netif;
static struct esp_netif_obj s_ap_netif;

struct esp_netif_obj
{
    int dummy;
};

void mock_esp_netif_reset(void)
{
    memset(&s_sta_netif, 0, sizeof(s_sta_netif));
    memset(&s_ap_netif, 0, sizeof(s_ap_netif));
}

esp_err_t esp_netif_init(void)
{
    return ESP_OK;
}

esp_netif_t *esp_netif_create_default_wifi_sta(void)
{
    return &s_sta_netif;
}

esp_netif_t *esp_netif_create_default_wifi_ap(void)
{
    return &s_ap_netif;
}

esp_err_t esp_netif_get_ip_info(esp_netif_t *esp_netif, esp_netif_ip_info_t *ip_info)
{
    if (esp_netif == NULL || ip_info == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    memset(ip_info, 0, sizeof(*ip_info));
    return ESP_OK;
}

esp_netif_t *esp_netif_get_handle_from_ifkey(const char *if_key)
{
    (void)if_key;
    return &s_sta_netif;
}

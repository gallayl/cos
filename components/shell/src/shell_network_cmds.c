#include "shell_cmds.h"

#include "esp_log.h"

static const char *const TAG = "shell_network";

void shell_register_network_commands(void)
{
    /* R22 M1: nslookup migrated to Berry command script (storage/flash_data/scripts/nslookup.be). */
    ESP_LOGI(TAG, "Network commands provided by Berry scripts (ping, nslookup)");
}

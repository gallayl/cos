#include "calibration.h"

#include "esp_console.h"
#include "esp_log.h"

static const char *const TAG = "cal_cmd";

static int calibrate_handler(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    esp_err_t err = calibration_run();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Calibration failed: %s", esp_err_to_name(err));
        return 1;
    }
    return 0;
}

void calibration_register_commands(void)
{
    const esp_console_cmd_t cmd = {
        .command = "calibrate",
        .help = "Run interactive touch screen calibration",
        .hint = NULL,
        .func = &calibrate_handler,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
    ESP_LOGI(TAG, "Registered 'calibrate' command");
}

#pragma once

#define ESP_LOGE(tag, fmt, ...)
#define ESP_LOGW(tag, fmt, ...)
#define ESP_LOGI(tag, fmt, ...)
#define ESP_LOGD(tag, fmt, ...)
#define ESP_LOGV(tag, fmt, ...)

#define ESP_RETURN_ON_ERROR(x, tag, fmt, ...)     \
    do                                            \
    {                                             \
        esp_err_t err_rc_ = (x);                  \
        if (err_rc_ != ESP_OK)                    \
        {                                         \
            return err_rc_;                       \
        }                                         \
    } while (0)

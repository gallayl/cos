#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

bool wifi_is_connected(void);
bool wifi_has_stored_credentials(void);
bool wifi_is_service_started(void);
bool wifi_is_forced_off_by_profile(void);

#ifdef __cplusplus
}
#endif
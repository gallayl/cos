#include "time_sync.h"

#include "esp_log.h"
#include "esp_sntp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const TAG = "time_sync";

#define NTP_SERVER "pool.ntp.org"
#define TIMEZONE "CET-1CEST,M3.5.0,M10.5.0/3"

/* Year threshold: if time() returns a year <= 2020, clock is not synced. */
#define SYNCED_YEAR_THRESHOLD 2020

void time_sync_register_commands(void);

/* Portable setenv wrapper (POSIX setenv is not available on all hosts). */
static void set_tz_env(const char *tz)
{
#if defined(_WIN32) || defined(__MINGW32__)
    _putenv_s("TZ", tz);
#else
    setenv("TZ", tz, 1);
#endif
    tzset();
}

/* Compute UTC offset in seconds from the difference of localtime and gmtime. */
static long utc_offset_seconds(time_t t)
{
    struct tm local_tm, utc_tm;

#if defined(_WIN32) || defined(__MINGW32__)
    localtime_s(&local_tm, &t);
    gmtime_s(&utc_tm, &t);
#else
    localtime_r(&t, &local_tm);
    gmtime_r(&t, &utc_tm);
#endif

    int day_diff = local_tm.tm_yday - utc_tm.tm_yday;
    /* Handle year boundary: if local is Jan 1 and utc is Dec 31, day_diff
       is large but should be +1.  Likewise for the reverse. */
    if (day_diff > 1)
    {
        day_diff = -1;
    }
    else if (day_diff < -1)
    {
        day_diff = 1;
    }

    return (long)(day_diff * 86400 + (local_tm.tm_hour - utc_tm.tm_hour) * 3600 +
                  (local_tm.tm_min - utc_tm.tm_min) * 60 + (local_tm.tm_sec - utc_tm.tm_sec));
}

esp_err_t time_sync_init(void)
{
    esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, NTP_SERVER);
    esp_sntp_init();

    set_tz_env(TIMEZONE);

    time_sync_register_commands();

    ESP_LOGI(TAG, "SNTP initialized (server=%s, TZ=%s)", NTP_SERVER, TIMEZONE);
    return ESP_OK;
}

bool time_sync_is_synced(void)
{
    time_t now = time(NULL);
    struct tm t;
#if defined(_WIN32) || defined(__MINGW32__)
    gmtime_s(&t, &now);
#else
    gmtime_r(&now, &t);
#endif
    return (t.tm_year + 1900) > SYNCED_YEAR_THRESHOLD;
}

time_t time_sync_get_epoch(void)
{
    return time(NULL);
}

esp_err_t time_sync_get_utc(char *buf, size_t len)
{
    if (buf == NULL || len < 21)
    {
        return ESP_ERR_INVALID_SIZE;
    }

    time_t now = time(NULL);
    struct tm t;
#if defined(_WIN32) || defined(__MINGW32__)
    gmtime_s(&t, &now);
#else
    gmtime_r(&now, &t);
#endif
    strftime(buf, len, "%FT%TZ", &t);
    return ESP_OK;
}

esp_err_t time_sync_get_local(char *buf, size_t len)
{
    if (buf == NULL || len < 26)
    {
        return ESP_ERR_INVALID_SIZE;
    }

    time_t now = time(NULL);
    struct tm t;
#if defined(_WIN32) || defined(__MINGW32__)
    localtime_s(&t, &now);
#else
    localtime_r(&now, &t);
#endif

    size_t written = strftime(buf, len, "%FT%T", &t);
    if (written == 0)
    {
        return ESP_ERR_INVALID_SIZE;
    }

    long off = utc_offset_seconds(now);
    char sign = (off >= 0) ? '+' : '-';
    if (off < 0)
    {
        off = -off;
    }
    int off_h = (int)(off / 3600);
    int off_m = (int)((off % 3600) / 60);

    int remaining = (int)(len - written);
    if (remaining < 7)
    {
        return ESP_ERR_INVALID_SIZE;
    }
    snprintf(buf + written, (size_t)remaining, "%c%02d:%02d", sign, off_h, off_m);
    return ESP_OK;
}

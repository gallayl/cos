#include "nvs.h"
#include "mock_nvs.h"

#include <stdbool.h>
#include <string.h>

#define MOCK_NVS_MAX_ENTRIES 32
#define MOCK_NVS_MAX_KEY_LEN 16
#define MOCK_NVS_MAX_BLOB_LEN 64

typedef struct
{
    char key[MOCK_NVS_MAX_KEY_LEN];
    uint8_t data[MOCK_NVS_MAX_BLOB_LEN];
    size_t size;
    bool used;
} mock_nvs_entry_t;

static mock_nvs_entry_t entries[MOCK_NVS_MAX_ENTRIES];
static int forced_error = 0;

void mock_nvs_reset(void)
{
    memset(entries, 0, sizeof(entries));
    forced_error = 0;
}

void mock_nvs_set_next_error(int err)
{
    forced_error = err;
}

static int consume_forced_error(void)
{
    int err = forced_error;
    forced_error = 0;
    return err;
}

static mock_nvs_entry_t *find_entry(const char *key)
{
    for (int i = 0; i < MOCK_NVS_MAX_ENTRIES; i++)
    {
        if (entries[i].used && strncmp(entries[i].key, key, MOCK_NVS_MAX_KEY_LEN) == 0)
        {
            return &entries[i];
        }
    }
    return NULL;
}

static mock_nvs_entry_t *alloc_entry(const char *key)
{
    mock_nvs_entry_t *existing = find_entry(key);
    if (existing)
    {
        return existing;
    }
    for (int i = 0; i < MOCK_NVS_MAX_ENTRIES; i++)
    {
        if (!entries[i].used)
        {
            entries[i].used = true;
            strncpy(entries[i].key, key, MOCK_NVS_MAX_KEY_LEN - 1);
            entries[i].key[MOCK_NVS_MAX_KEY_LEN - 1] = '\0';
            return &entries[i];
        }
    }
    return NULL;
}

esp_err_t nvs_open(const char *namespace_name, nvs_open_mode_t open_mode, nvs_handle_t *out_handle)
{
    (void)namespace_name;
    (void)open_mode;

    int err = consume_forced_error();
    if (err)
    {
        return err;
    }

    *out_handle = 1;
    return ESP_OK;
}

void nvs_close(nvs_handle_t handle)
{
    (void)handle;
}

esp_err_t nvs_set_blob(nvs_handle_t handle, const char *key, const void *value, size_t length)
{
    (void)handle;

    int err = consume_forced_error();
    if (err)
    {
        return err;
    }

    if (length > MOCK_NVS_MAX_BLOB_LEN)
    {
        return ESP_ERR_INVALID_SIZE;
    }

    mock_nvs_entry_t *entry = alloc_entry(key);
    if (!entry)
    {
        return ESP_ERR_NO_MEM;
    }

    memcpy(entry->data, value, length);
    entry->size = length;
    return ESP_OK;
}

esp_err_t nvs_get_blob(nvs_handle_t handle, const char *key, void *out_value, size_t *length)
{
    (void)handle;

    int err = consume_forced_error();
    if (err)
    {
        return err;
    }

    mock_nvs_entry_t *entry = find_entry(key);
    if (!entry)
    {
        return ESP_ERR_NVS_NOT_FOUND;
    }

    if (out_value == NULL)
    {
        *length = entry->size;
        return ESP_OK;
    }

    if (*length < entry->size)
    {
        return ESP_ERR_INVALID_SIZE;
    }

    memcpy(out_value, entry->data, entry->size);
    *length = entry->size;
    return ESP_OK;
}

esp_err_t nvs_commit(nvs_handle_t handle)
{
    (void)handle;
    return ESP_OK;
}

esp_err_t nvs_erase_key(nvs_handle_t handle, const char *key)
{
    (void)handle;
    mock_nvs_entry_t *entry = find_entry(key);
    if (!entry)
    {
        return ESP_ERR_NVS_NOT_FOUND;
    }
    entry->used = false;
    return ESP_OK;
}

esp_err_t nvs_erase_all(nvs_handle_t handle)
{
    (void)handle;
    mock_nvs_reset();
    return ESP_OK;
}

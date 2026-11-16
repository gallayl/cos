#include "service_lifecycle.h"

#include <string.h>

service_descriptor_t *service_lifecycle_find(service_descriptor_t *services, size_t count, const char *id)
{
    if (services == NULL || id == NULL)
    {
        return NULL;
    }

    for (size_t i = 0; i < count; ++i)
    {
        if (services[i].id != NULL && strcmp(services[i].id, id) == 0)
        {
            return &services[i];
        }
    }

    return NULL;
}

bool service_lifecycle_dependencies_satisfied(service_descriptor_t *services, size_t count,
                                              const service_descriptor_t *svc)
{
    if (services == NULL || svc == NULL)
    {
        return false;
    }

    for (size_t i = 0; i < svc->depends_on_count; ++i)
    {
        const char *dep_id = svc->depends_on[i];
        service_descriptor_t *dep = service_lifecycle_find(services, count, dep_id);
        if (dep == NULL || !dep->enabled || dep->state != SERVICE_LIFECYCLE_STARTED)
        {
            return false;
        }
    }

    return true;
}

void service_lifecycle_boot(service_descriptor_t *services, size_t count)
{
    if (services == NULL)
    {
        return;
    }

    for (size_t i = 0; i < count; ++i)
    {
        service_descriptor_t *svc = &services[i];
        if (!svc->enabled)
        {
            svc->state = SERVICE_LIFECYCLE_UNINITIALIZED;
            svc->last_result = ESP_OK;
            continue;
        }

        if (!service_lifecycle_dependencies_satisfied(services, count, svc))
        {
            svc->state = SERVICE_LIFECYCLE_FAILED;
            svc->last_result = ESP_ERR_INVALID_STATE;
            continue;
        }

        service_lifecycle_hook_t hook = svc->init;
        if (hook == NULL)
        {
            hook = svc->start;
        }

        if (hook == NULL)
        {
            /* No init/start hook: treat as started/available marker. */
            svc->state = SERVICE_LIFECYCLE_STARTED;
            svc->last_result = ESP_OK;
            continue;
        }

        esp_err_t err = hook(svc);
        svc->last_result = err;
        svc->state = (err == ESP_OK) ? SERVICE_LIFECYCLE_STARTED : SERVICE_LIFECYCLE_FAILED;
    }
}

void service_lifecycle_stop_all(service_descriptor_t *services, size_t count)
{
    if (services == NULL || count == 0)
    {
        return;
    }

    for (size_t i = count; i > 0; --i)
    {
        service_descriptor_t *svc = &services[i - 1];
        if (svc->state != SERVICE_LIFECYCLE_STARTED)
        {
            continue;
        }

        if ((svc->capabilities & SERVICE_CAP_CAN_STOP) == 0 || svc->stop == NULL)
        {
            svc->state = SERVICE_LIFECYCLE_UNSUPPORTED_STOP;
            continue;
        }

        esp_err_t err = svc->stop(svc);
        svc->last_result = err;
        svc->state = (err == ESP_OK) ? SERVICE_LIFECYCLE_STOPPED : SERVICE_LIFECYCLE_FAILED;
    }
}

const char *service_lifecycle_state_name(service_lifecycle_state_t state)
{
    switch (state)
    {
    case SERVICE_LIFECYCLE_UNINITIALIZED:
        return "uninitialized";
    case SERVICE_LIFECYCLE_STARTED:
        return "started";
    case SERVICE_LIFECYCLE_STOPPED:
        return "stopped";
    case SERVICE_LIFECYCLE_FAILED:
        return "failed";
    case SERVICE_LIFECYCLE_UNSUPPORTED_STOP:
        return "unsupported-stop";
    default:
        return "unknown";
    }
}

#pragma once

#include "esp_err.h"

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /** Lifecycle state of a service descriptor. */
    typedef enum
    {
        SERVICE_LIFECYCLE_UNINITIALIZED = 0,
        SERVICE_LIFECYCLE_STARTED,
        SERVICE_LIFECYCLE_STOPPED,
        SERVICE_LIFECYCLE_FAILED,
        SERVICE_LIFECYCLE_UNSUPPORTED_STOP,
    } service_lifecycle_state_t;

    /** Capability flags for a service descriptor. */
    typedef enum
    {
        SERVICE_CAP_NONE = 0,
        SERVICE_CAP_CAN_START = 1u << 0,
        SERVICE_CAP_CAN_STOP = 1u << 1,
        SERVICE_CAP_CAN_DEINIT = 1u << 2,
        SERVICE_CAP_ALWAYS_ON = 1u << 3,
    } service_capability_t;

    typedef struct service_descriptor service_descriptor_t;

    /** Lifecycle hook. Returns ESP_OK on success. */
    typedef esp_err_t (*service_lifecycle_hook_t)(service_descriptor_t *svc);

    /**
     * @brief Static service descriptor.
     *
     * Describes one boot/service unit managed by the unified lifecycle model.
     * Dependency ids are stable string identifiers matching descriptor->id.
     */
    struct service_descriptor
    {
        const char *id;                  /**< Stable service identifier. */
        const char *name;                /**< Human-readable service name. */
        const char *const *depends_on;   /**< Required dependency service ids. */
        size_t depends_on_count;         /**< Number of dependency ids. */
        service_lifecycle_hook_t init;   /**< Init/start hook (required for boot-managed services). */
        service_lifecycle_hook_t start;  /**< Optional start hook (used if init is NULL or for restart flows). */
        service_lifecycle_hook_t stop;   /**< Optional stop/deinit hook. */
        unsigned capabilities;           /**< Bitmask of service_capability_t. */
        service_lifecycle_state_t state; /**< Current lifecycle state. */
        esp_err_t last_result;           /**< Result of last lifecycle operation. */
        bool enabled;                    /**< Whether service is enabled for boot/run. */
        void *context;                   /**< Optional hook context. */
    };

    /**
     * @brief Find a descriptor by id in a static table.
     * @return Descriptor pointer or NULL if not found.
     */
    service_descriptor_t *service_lifecycle_find(service_descriptor_t *services, size_t count, const char *id);

    /**
     * @brief Check whether all required dependencies are in STARTED state.
     */
    bool service_lifecycle_dependencies_satisfied(service_descriptor_t *services, size_t count,
                                                  const service_descriptor_t *svc);

    /**
     * @brief Run boot/init for all enabled descriptors in table order.
     *
     * For each enabled descriptor, if required dependencies are missing or not started,
     * the service is marked FAILED with ESP_ERR_INVALID_STATE and skipped (fail-fast for that
     * service; dependents will also fail dependency checks when reached in order).
     * Otherwise invokes init (or start if init is NULL). On success sets STARTED; on failure
     * sets FAILED and records result.
     *
     * Disabled descriptors are left UNINITIALIZED.
     */
    void service_lifecycle_boot(service_descriptor_t *services, size_t count);

    /**
     * @brief Stop all started/stoppable descriptors in reverse table order (best-effort).
     *
     * Services without stop capability/hook are marked UNSUPPORTED_STOP (if started) or left
     * as-is and reported via state. Hook failures are recorded but do not prevent continuing.
     */
    void service_lifecycle_stop_all(service_descriptor_t *services, size_t count);

    /**
     * @brief Human-readable name for a lifecycle state.
     */
    const char *service_lifecycle_state_name(service_lifecycle_state_t state);

#ifdef __cplusplus
}
#endif

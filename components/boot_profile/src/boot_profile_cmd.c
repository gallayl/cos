#include "boot_profile.h"

#include "esp_console.h"

#include <stdio.h>
#include <string.h>

static void print_usage(void)
{
    printf("Usage: boot_profile [set <default|low-memory-offline>|clear]\n");
}

static void print_status(void)
{
    char stored[40] = {0};
    bool has_stored = false;
    (void)boot_profile_get_stored(stored, sizeof(stored), &has_stored);

    printf("Stored:    %s%s\n", stored, has_stored ? "" : " (none/invalid -> default)");
    printf("Effective: %s (current boot)\n", boot_profile_name(boot_profile_get_effective()));
    printf("Next boot: %s\n", boot_profile_name(boot_profile_get_next()));
    printf("Profiles:  %s, %s\n", BOOT_PROFILE_NAME_DEFAULT, BOOT_PROFILE_NAME_LOW_MEMORY_OFFLINE);
    printf("Note: changes take effect on next boot/restart.\n");

    if (boot_profile_get_effective() == BOOT_PROFILE_LOW_MEMORY_OFFLINE)
    {
        printf("Profile policy: Wi-Fi, network diagnostics, HTTP server, WebSocket, time sync, and Bluetooth "
               "stack/radio are forced off/unavailable for this boot (Bluetooth init still runs for "
               "compatibility/recovery; stored BT auto-enable preserved).\n");
    }
}

static int cmd_boot_profile(int argc, char **argv)
{
    if (argc == 1)
    {
        print_status();
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "clear") == 0)
    {
        esp_err_t err = boot_profile_clear_stored();
        if (err != ESP_OK)
        {
            printf("boot_profile: failed to clear stored profile (%s)\n", esp_err_to_name(err));
            return 1;
        }
        printf("Stored boot profile cleared. Next boot: %s (takes effect on next boot/restart)\n",
               boot_profile_name(boot_profile_get_next()));
        return 0;
    }

    if (argc == 3 && strcmp(argv[1], "set") == 0)
    {
        if (!boot_profile_parse(argv[2], NULL))
        {
            printf("boot_profile: invalid profile '%s'\n", argv[2]);
            print_usage();
            return 1;
        }

        esp_err_t err = boot_profile_set_stored(argv[2]);
        if (err != ESP_OK)
        {
            printf("boot_profile: failed to store profile '%s' (%s)\n", argv[2], esp_err_to_name(err));
            return 1;
        }

        printf("Stored boot profile set to '%s'. Takes effect on next boot/restart.\n", argv[2]);
        printf("Effective profile for current boot remains: %s\n", boot_profile_name(boot_profile_get_effective()));
        return 0;
    }

    print_usage();
    return 1;
}

void boot_profile_register_commands(void)
{
    const esp_console_cmd_t cmd = {
        .command = "boot_profile",
        .help = "Get/set boot profile (default, low-memory-offline). Changes apply on next boot.",
        .hint = NULL,
        .func = &cmd_boot_profile,
    };
    esp_console_cmd_register(&cmd);
}

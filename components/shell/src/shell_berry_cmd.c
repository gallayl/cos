#include "berry_runtime.h"
#include "filesystem.h"
#include "network.h"
#include "shell.h"
#include "system.h"

#include "esp_console.h"
#include "esp_log.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const TAG = "shell_berry";

esp_err_t shell_berry_run_autoexec(void)
{
    if (!vfs_exists("/flash/autoexec.be"))
    {
        return ESP_OK;
    }

    esp_err_t err = berry_runtime_init();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Could not initialize Berry for autoexec: %s", esp_err_to_name(err));
        return err;
    }

    int exit_status = 0;
    err = berry_runtime_run_file("/flash/autoexec.be", 0, NULL, &exit_status);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Could not run /flash/autoexec.be: %s", esp_err_to_name(err));
        return err;
    }
    if (exit_status != 0)
    {
        ESP_LOGW(TAG, "/flash/autoexec.be returned status %d", exit_status);
    }
    return ESP_OK;
}

static void print_http_stack(void)
{
    network_http_worker_stack_info_t http_stack;
    const esp_err_t http_err = network_http_get_worker_stack_info(&http_stack);

    if (http_err != ESP_OK)
    {
        printf("HTTP worker stack (R8/P4): unavailable (%s)\n", esp_err_to_name(http_err));
        return;
    }
    if (http_stack.available)
    {
        printf("HTTP worker stack (R8/P4): cfg=%u unit=%u hwm_words=%u hwm_bytes=%u used~=%u method=%s scheme=%s "
               "status=%d err=%s\n",
               (unsigned)http_stack.configured_stack, (unsigned)http_stack.stack_unit_bytes,
               (unsigned)http_stack.hwm_words, (unsigned)http_stack.hwm_bytes, (unsigned)http_stack.used_bytes,
               http_stack.last_method, http_stack.https ? "https" : "http", http_stack.last_status,
               esp_err_to_name(http_stack.last_error));
    }
    else
    {
        printf("HTTP worker stack (R8/P4): not started (cfg=%u unit=%u)\n", (unsigned)http_stack.configured_stack,
               (unsigned)http_stack.stack_unit_bytes);
    }
}

static void print_profile(void)
{
    system_info_t info;
    const esp_err_t sys_err = system_get_info(&info);
    berry_runtime_memory_info_t berry_info;
    const esp_err_t berry_err = berry_runtime_get_memory_info(&berry_info);
    network_http_worker_stack_info_t http_stack;
    const esp_err_t http_err = network_http_get_worker_stack_info(&http_stack);

    printf("Berry memory profile (R2/R5/R8)\n");
    if (sys_err == ESP_OK)
    {
        printf("  Default heap:  free=%u min=%u total=%u largest=%u\n", (unsigned)info.free_heap,
               (unsigned)info.min_free_heap, (unsigned)info.total_heap, (unsigned)info.max_alloc_block);
        printf("  Internal heap: free=%u min=%u total=%u largest=%u\n", (unsigned)info.internal_free_heap,
               (unsigned)info.internal_min_free_heap, (unsigned)info.internal_total_heap,
               (unsigned)info.internal_max_alloc_block);
    }
    else
    {
        printf("  Default heap:  unavailable (%s)\n", esp_err_to_name(sys_err));
    }
    if (berry_err == ESP_OK)
    {
        printf("  Berry GC:      initialized=%s managed=%u bytes step_rate=%d\n", berry_info.initialized ? "yes" : "no",
               (unsigned)berry_info.gc_managed_bytes, berry_info.gc_step_rate);
        printf("  Heap snapshot: free=%u min=%u largest=%u internal_free=%u internal_min=%u internal_largest=%u\n",
               (unsigned)berry_info.heap_free, (unsigned)berry_info.heap_min_free,
               (unsigned)berry_info.heap_largest_block, (unsigned)berry_info.internal_free,
               (unsigned)berry_info.internal_min_free, (unsigned)berry_info.internal_largest_block);
    }
    else
    {
        printf("  Berry GC:      unavailable (%s)\n", esp_err_to_name(berry_err));
    }
    if (http_err == ESP_OK)
    {
        if (http_stack.available)
        {
            printf("  HTTP worker:   cfg=%u unit=%u hwm=%u used~=%u bytes method=%s %s status=%d err=%s\n",
                   (unsigned)http_stack.configured_stack, (unsigned)http_stack.stack_unit_bytes,
                   (unsigned)http_stack.hwm_bytes, (unsigned)http_stack.used_bytes, http_stack.last_method,
                   http_stack.https ? "https" : "http", http_stack.last_status, esp_err_to_name(http_stack.last_error));
        }
        else
        {
            printf("  HTTP worker:   not started (cfg=%u unit=%u)\n", (unsigned)http_stack.configured_stack,
                   (unsigned)http_stack.stack_unit_bytes);
        }
    }
}

static int cmd_berry(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: berry run <path> [args...] | berry eval <source> | berry close | berry "
               "profile | berry http-stack | berry gc | berry gc-step <rate>\n");
        return 1;
    }

    if (strcmp(argv[1], "profile") == 0 || strcmp(argv[1], "heap") == 0)
    {
        if (argc != 2)
        {
            printf("Usage: berry profile\n");
            return 1;
        }
        print_profile();
        return 0;
    }

    if (strcmp(argv[1], "http-stack") == 0 || strcmp(argv[1], "net-stack") == 0)
    {
        if (argc != 2)
        {
            printf("Usage: berry http-stack\n");
            return 1;
        }
        print_http_stack();
        return 0;
    }

    if (strcmp(argv[1], "gc") == 0)
    {
        if (argc != 2)
        {
            printf("Usage: berry gc\n");
            return 1;
        }
        esp_err_t err = berry_runtime_init();
        if (err != ESP_OK)
        {
            printf("berry: initialization failed (%s)\n", esp_err_to_name(err));
            return 1;
        }
        size_t before = 0;
        size_t after = 0;
        err = berry_runtime_gc_collect(&before, &after);
        if (err != ESP_OK)
        {
            printf("berry: gc failed (%s)\n", esp_err_to_name(err));
            return 1;
        }
        const size_t reclaimed = before > after ? before - after : 0;
        printf("Berry GC: before=%u after=%u reclaimed=%u bytes\n", (unsigned)before, (unsigned)after,
               (unsigned)reclaimed);
        print_profile();
        return 0;
    }

    if (strcmp(argv[1], "gc-step") == 0)
    {
        if (argc != 3)
        {
            printf("Usage: berry gc-step <rate>  (200=default; try 150/125 for R5 experiments)\n");
            return 1;
        }
        esp_err_t err = berry_runtime_init();
        if (err != ESP_OK)
        {
            printf("berry: initialization failed (%s)\n", esp_err_to_name(err));
            return 1;
        }
        char *end = NULL;
        const long rate = strtol(argv[2], &end, 10);
        if (end == argv[2] || *end != '\0' || rate < 100 || rate > 500)
        {
            printf("berry: gc-step rate must be an integer in [100,500] (default 200)\n");
            return 1;
        }
        err = berry_runtime_set_gc_step_rate((int)rate);
        if (err != ESP_OK)
        {
            printf("berry: gc-step failed (%s)\n", esp_err_to_name(err));
            return 1;
        }
        printf("Berry GC step rate: %d (default 200; keep default unless R5 benchmarks justify change)\n", (int)rate);
        return 0;
    }

    esp_err_t err = berry_runtime_init();

    if (err != ESP_OK)
    {
        printf("berry: initialization failed (%s)\n", esp_err_to_name(err));
        return 1;
    }

    if (strcmp(argv[1], "close") == 0)
    {
        if (argc != 2)
        {
            printf("Usage: berry close\n");
            return 1;
        }
        err = berry_runtime_close_app();
    }
    else if (strcmp(argv[1], "run") == 0)
    {
        if (argc < 3)
        {
            printf("Usage: berry run <path> [args...]\n");
            return 1;
        }
        char path[VFS_PATH_MAX];
        err = shell_resolve_relative(argv[2], path, sizeof(path));
        if (err == ESP_OK)
        {
            int exit_status = 1;
            err = berry_runtime_run_file(path, argc - 3, argv + 3, &exit_status);
            if (err == ESP_OK)
            {
                return exit_status;
            }
        }
    }
    else if (strcmp(argv[1], "eval") == 0)
    {
        if (argc < 3)
        {
            printf("Usage: berry eval <source>\n");
            return 1;
        }
        size_t source_size = 1;
        for (int i = 2; i < argc; ++i)
        {
            size_t arg_size = strlen(argv[i]);
            size_t separator_size = i > 2 ? 1 : 0;
            if (source_size > SIZE_MAX - separator_size || arg_size > SIZE_MAX - source_size - separator_size)
            {
                printf("berry: source is too large\n");
                return 1;
            }
            source_size += arg_size + separator_size;
        }

        char *source = (char *)malloc(source_size);
        if (source == NULL)
        {
            printf("berry: out of memory\n");
            return 1;
        }

        size_t offset = 0;
        for (int i = 2; i < argc; ++i)
        {
            if (i > 2)
            {
                source[offset++] = ' ';
            }
            size_t arg_size = strlen(argv[i]);
            memcpy(source + offset, argv[i], arg_size);
            offset += arg_size;
        }
        source[offset] = '\0';
        err = berry_runtime_eval(source, "shell");
        free(source);
    }
    else
    {
        printf("berry: unknown operation '%s'\n", argv[1]);
        printf("Usage: berry run <path> [args...] | berry eval <source> | berry close | berry "
               "profile | berry http-stack | berry gc | berry gc-step <rate>\n");
        return 1;
    }

    if (err != ESP_OK)
    {
        printf("berry: execution failed (%s)\n", esp_err_to_name(err));
        return 1;
    }

    return 0;
}

void shell_register_berry_commands(void)
{
    const esp_console_cmd_t command = {
        .command = "berry",
        .help = "Run/eval Berry entrypoint scripts; profile/http-stack/gc tools",
        .hint = NULL,
        .func = &cmd_berry,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&command));
}

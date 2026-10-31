#include "filesystem.h"
#include "shell.h"

#include "esp_console.h"
#include "esp_log.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const TAG = "shell_fs";

/* ---- Helper: resolve command argument to absolute virtual path ---- */

static esp_err_t resolve_arg(const char *arg, char *abs_path, size_t len)
{
    if (arg == NULL || arg[0] == '\0')
    {
        strncpy(abs_path, shell_get_cwd(), len - 1);
        abs_path[len - 1] = '\0';
        return ESP_OK;
    }
    return shell_resolve_relative(arg, abs_path, len);
}

/* ---- cd ---- */

static int cmd_cd(int argc, char **argv)
{
    const char *target = (argc > 1) ? argv[1] : "/flash";

    char abs_path[VFS_PATH_MAX];
    esp_err_t err = shell_resolve_relative(target, abs_path, sizeof(abs_path));
    if (err != ESP_OK)
    {
        printf("cd: invalid path\n");
        return 1;
    }

    err = shell_set_cwd(abs_path);
    if (err != ESP_OK)
    {
        printf("cd: %s: not a directory\n", abs_path);
        return 1;
    }
    return 0;
}

/* ---- pwd ---- */

static int cmd_pwd(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    printf("%s\n", shell_get_cwd());
    return 0;
}

/* ---- ls / dir ---- */

static int cmd_ls(int argc, char **argv)
{
    char abs_path[VFS_PATH_MAX];
    esp_err_t err = resolve_arg((argc > 1) ? argv[1] : NULL, abs_path, sizeof(abs_path));
    if (err != ESP_OK)
    {
        printf("ls: invalid path\n");
        return 1;
    }

    vfs_dir_entry_t *entries = malloc(VFS_ENTRIES_MAX * sizeof(vfs_dir_entry_t));
    if (entries == NULL)
    {
        printf("ls: out of memory\n");
        return 1;
    }

    size_t count = 0;
    err = vfs_list_dir(abs_path, entries, VFS_ENTRIES_MAX, &count);
    if (err != ESP_OK)
    {
        free(entries);
        printf("ls: cannot open '%s'\n", abs_path);
        return 1;
    }

    if (count == 0)
    {
        free(entries);
        printf("(empty)\n");
        return 0;
    }

    size_t total_bytes = 0;
    size_t file_count = 0;
    size_t dir_count = 0;

    for (size_t i = 0; i < count; i++)
    {
        if (entries[i].is_dir)
        {
            printf("  <DIR>  %s/\n", entries[i].name);
            dir_count++;
        }
        else
        {
            printf("  %5u  %s\n", (unsigned)entries[i].size, entries[i].name);
            total_bytes += entries[i].size;
            file_count++;
        }
    }

    free(entries);
    printf("%u file(s), %u dir(s), %u bytes\n", (unsigned)file_count, (unsigned)dir_count, (unsigned)total_bytes);
    return 0;
}

/* ---- mkdir / md ---- */

static int cmd_mkdir(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: mkdir <path>\n");
        return 1;
    }

    char abs_path[VFS_PATH_MAX];
    esp_err_t err = shell_resolve_relative(argv[1], abs_path, sizeof(abs_path));
    if (err != ESP_OK)
    {
        printf("mkdir: invalid path\n");
        return 1;
    }

    err = vfs_mkdir(abs_path);
    if (err != ESP_OK)
    {
        printf("mkdir: failed to create '%s'\n", abs_path);
        return 1;
    }
    return 0;
}

/* ---- rm / del ---- */

static int cmd_rm(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: rm <path>\n");
        return 1;
    }

    char abs_path[VFS_PATH_MAX];
    esp_err_t err = shell_resolve_relative(argv[1], abs_path, sizeof(abs_path));
    if (err != ESP_OK)
    {
        printf("rm: invalid path\n");
        return 1;
    }

    err = vfs_remove(abs_path);
    if (err != ESP_OK)
    {
        printf("rm: failed to remove '%s'\n", abs_path);
        return 1;
    }
    return 0;
}

/* ---- cp ---- */

static int cmd_cp(int argc, char **argv)
{
    if (argc < 3)
    {
        printf("Usage: cp <source> <destination>\n");
        return 1;
    }

    char source[VFS_PATH_MAX];
    char destination[VFS_PATH_MAX];
    if (shell_resolve_relative(argv[1], source, sizeof(source)) != ESP_OK ||
        shell_resolve_relative(argv[2], destination, sizeof(destination)) != ESP_OK)
    {
        printf("cp: invalid path\n");
        return 1;
    }

    if (vfs_is_directory(source))
    {
        printf("cp: '%s' is a directory; directory copies are not supported\n", source);
        return 1;
    }
    if (!vfs_exists(source))
    {
        printf("cp: source '%s' does not exist\n", source);
        return 1;
    }
    if (vfs_exists(destination))
    {
        printf("cp: destination '%s' already exists\n", destination);
        return 1;
    }

    esp_err_t err = vfs_copy_file(source, destination);
    if (err != ESP_OK)
    {
        printf("cp: failed to copy '%s' to '%s'\n", source, destination);
        return 1;
    }
    return 0;
}

/* ---- mv ---- */

static int cmd_mv(int argc, char **argv)
{
    if (argc < 3)
    {
        printf("Usage: mv <source> <destination>\n");
        return 1;
    }

    char source[VFS_PATH_MAX];
    char destination[VFS_PATH_MAX];
    if (shell_resolve_relative(argv[1], source, sizeof(source)) != ESP_OK ||
        shell_resolve_relative(argv[2], destination, sizeof(destination)) != ESP_OK)
    {
        printf("mv: invalid path\n");
        return 1;
    }

    if (vfs_is_directory(source))
    {
        printf("mv: '%s' is a directory; directory moves are not supported\n", source);
        return 1;
    }
    if (!vfs_exists(source))
    {
        printf("mv: source '%s' does not exist\n", source);
        return 1;
    }
    if (vfs_exists(destination))
    {
        printf("mv: destination '%s' already exists\n", destination);
        return 1;
    }

    esp_err_t err = vfs_rename_file(source, destination);
    if (err == ESP_ERR_NOT_SUPPORTED)
    {
        err = vfs_copy_file(source, destination);
        if (err == ESP_OK)
        {
            err = vfs_remove(source);
            if (err != ESP_OK)
            {
                if (vfs_remove(destination) != ESP_OK)
                {
                    printf("mv: failed to remove source '%s' and could not roll back '%s'\n", source, destination);
                }
                else
                {
                    printf("mv: failed to remove source '%s'; destination was rolled back\n", source);
                }
                return 1;
            }
        }
    }

    if (err != ESP_OK)
    {
        printf("mv: failed to move '%s' to '%s'\n", source, destination);
        return 1;
    }
    return 0;
}

/* ---- rmdir ---- */

static int cmd_rmdir(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: rmdir <directory>\n");
        return 1;
    }

    char abs_path[VFS_PATH_MAX];
    if (shell_resolve_relative(argv[1], abs_path, sizeof(abs_path)) != ESP_OK)
    {
        printf("rmdir: invalid path\n");
        return 1;
    }

    if (strcmp(abs_path, "/") == 0 || strcmp(abs_path, "/flash") == 0 || strcmp(abs_path, "/sdcard") == 0)
    {
        printf("rmdir: cannot remove a filesystem root\n");
        return 1;
    }
    if (strcmp(abs_path, shell_get_cwd()) == 0)
    {
        printf("rmdir: cannot remove the current working directory\n");
        return 1;
    }
    if (!vfs_is_directory(abs_path))
    {
        printf("rmdir: '%s' is not a directory\n", abs_path);
        return 1;
    }

    esp_err_t err = vfs_remove(abs_path);
    if (err != ESP_OK)
    {
        printf("rmdir: failed to remove '%s' (directory must be empty)\n", abs_path);
        return 1;
    }
    return 0;
}

/* ---- cat / type ---- */

typedef struct
{
    bool output_failed;
    bool has_output;
    char last_char;
} cat_output_context_t;

static esp_err_t cat_write_chunk(const void *data, size_t len, void *context)
{
    cat_output_context_t *output = context;
    if (fwrite(data, 1, len, stdout) != len)
    {
        output->output_failed = true;
        return ESP_FAIL;
    }

    output->has_output = true;
    output->last_char = ((const char *)data)[len - 1];
    return ESP_OK;
}

static int cmd_cat(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: cat <path>\n");
        return 1;
    }

    char abs_path[VFS_PATH_MAX];
    esp_err_t err = shell_resolve_relative(argv[1], abs_path, sizeof(abs_path));
    if (err != ESP_OK)
    {
        printf("cat: invalid path\n");
        return 1;
    }

    if (vfs_is_directory(abs_path))
    {
        printf("cat: '%s' is a directory\n", abs_path);
        return 1;
    }

    cat_output_context_t output = {false, false, '\0'};
    err = vfs_read_file_stream(abs_path, cat_write_chunk, &output);
    if (err != ESP_OK)
    {
        if (output.output_failed)
        {
            printf("\ncat: failed to write output\n");
        }
        else
        {
            printf("cat: cannot read '%s'\n", abs_path);
        }
        return 1;
    }

    if (output.has_output)
    {
        if (output.last_char != '\n')
        {
            printf("\n");
        }
    }
    return 0;
}

/* ---- touch ---- */

static int cmd_touch(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: touch <path>\n");
        return 1;
    }

    char abs_path[VFS_PATH_MAX];
    esp_err_t err = shell_resolve_relative(argv[1], abs_path, sizeof(abs_path));
    if (err != ESP_OK)
    {
        printf("touch: invalid path\n");
        return 1;
    }

    if (!vfs_exists(abs_path))
    {
        err = vfs_write_file(abs_path, "", 0);
        if (err != ESP_OK)
        {
            printf("touch: cannot create '%s'\n", abs_path);
            return 1;
        }
    }
    return 0;
}

/* ---- hexdump ---- */

static int cmd_hexdump(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: hexdump <path>\n");
        return 1;
    }

    char abs_path[VFS_PATH_MAX];
    esp_err_t err = shell_resolve_relative(argv[1], abs_path, sizeof(abs_path));
    if (err != ESP_OK)
    {
        printf("hexdump: invalid path\n");
        return 1;
    }

    char buf[256];
    size_t bytes_read = 0;
    err = vfs_read_file(abs_path, buf, sizeof(buf), &bytes_read);
    if (err != ESP_OK)
    {
        printf("hexdump: cannot read '%s'\n", abs_path);
        return 1;
    }

    for (size_t offset = 0; offset < bytes_read; offset += 16)
    {
        printf("%04x  ", (unsigned)offset);

        for (size_t j = 0; j < 16; j++)
        {
            if (offset + j < bytes_read)
            {
                printf("%02x ", (unsigned char)buf[offset + j]);
            }
            else
            {
                printf("   ");
            }
            if (j == 7)
            {
                printf(" ");
            }
        }

        printf(" |");
        for (size_t j = 0; j < 16 && (offset + j) < bytes_read; j++)
        {
            char c = buf[offset + j];
            printf("%c", isprint((unsigned char)c) ? c : '.');
        }
        printf("|\n");
    }

    printf("%u bytes", (unsigned)bytes_read);
    if (bytes_read == sizeof(buf))
    {
        printf(" (truncated, max %u)", (unsigned)sizeof(buf));
    }
    printf("\n");
    return 0;
}

/* ---- Registration ---- */

static void register_cmd(const char *name, const char *help, const char *hint, esp_console_cmd_func_t func)
{
    const esp_console_cmd_t cmd = {
        .command = name,
        .help = help,
        .hint = hint,
        .func = func,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

void shell_register_fs_commands(void)
{
    register_cmd("cd", "Change working directory", "<path>", &cmd_cd);
    register_cmd("pwd", "Print working directory", NULL, &cmd_pwd);

    register_cmd("ls", "List directory contents", "[path]", &cmd_ls);
    register_cmd("dir", "List directory contents (alias for ls)", "[path]", &cmd_ls);

    register_cmd("mkdir", "Create a directory", "<path>", &cmd_mkdir);
    register_cmd("md", "Create a directory (alias for mkdir)", "<path>", &cmd_mkdir);

    register_cmd("rm", "Remove a file or empty directory", "<path>", &cmd_rm);
    register_cmd("del", "Remove a file or empty directory (alias for rm)", "<path>", &cmd_rm);
    register_cmd("cp", "Copy a file without overwriting", "<source> <destination>", &cmd_cp);
    register_cmd("mv", "Move a file without overwriting", "<source> <destination>", &cmd_mv);
    register_cmd("rmdir", "Remove an empty directory", "<directory>", &cmd_rmdir);

    register_cmd("cat", "Display file contents", "<path>", &cmd_cat);
    register_cmd("type", "Display file contents (alias for cat)", "<path>", &cmd_cat);

    register_cmd("touch", "Create an empty file", "<path>", &cmd_touch);
    register_cmd("hexdump", "Hex dump of a file", "<path>", &cmd_hexdump);

    ESP_LOGI(TAG, "Filesystem commands registered");
}

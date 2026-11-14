#include "esp_err.h"
#include "unity.h"

#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

/* Minimal in-memory backend for vfs.c seam */
#define MOCK_MAX_NODES 64
#define MOCK_MAX_NAME 160
#define MOCK_MAX_DATA 512

typedef struct
{
    bool used;
    bool is_dir;
    char path[MOCK_MAX_NAME];
    char data[MOCK_MAX_DATA];
    size_t len;
} mock_node_t;

static mock_node_t s_nodes[MOCK_MAX_NODES];
static bool s_sd_mounted = true;

static mock_node_t *find_node(const char *path)
{
    for (int i = 0; i < MOCK_MAX_NODES; i++)
    {
        if (s_nodes[i].used && strcmp(s_nodes[i].path, path) == 0)
        {
            return &s_nodes[i];
        }
    }
    return NULL;
}

static mock_node_t *add_node(const char *path, bool is_dir)
{
    mock_node_t *n = find_node(path);
    if (n != NULL)
    {
        n->is_dir = is_dir;
        if (is_dir)
        {
            n->len = 0;
        }
        return n;
    }
    for (int i = 0; i < MOCK_MAX_NODES; i++)
    {
        if (!s_nodes[i].used)
        {
            memset(&s_nodes[i], 0, sizeof(s_nodes[i]));
            s_nodes[i].used = true;
            s_nodes[i].is_dir = is_dir;
            strncpy(s_nodes[i].path, path, sizeof(s_nodes[i].path) - 1);
            return &s_nodes[i];
        }
    }
    return NULL;
}

static void remove_node_path(const char *path)
{
    mock_node_t *n = find_node(path);
    if (n != NULL)
    {
        memset(n, 0, sizeof(*n));
    }
}

static bool has_children(const char *path)
{
    size_t plen = strlen(path);
    for (int i = 0; i < MOCK_MAX_NODES; i++)
    {
        if (!s_nodes[i].used)
        {
            continue;
        }
        if (strncmp(s_nodes[i].path, path, plen) == 0 && s_nodes[i].path[plen] == '/' && s_nodes[i].path[plen + 1] != '\0')
        {
            return true;
        }
    }
    return false;
}

static void mock_backend_reset(void)
{
    memset(s_nodes, 0, sizeof(s_nodes));
    s_sd_mounted = true;
    add_node("/littlefs", true);
}

/* POSIX seam implementations */

typedef struct
{
    bool used;
    char dirpath[MOCK_MAX_NAME];
    int index;
    struct dirent ent;
} mock_dir_t;

static mock_dir_t s_dirs[8];
typedef struct
{
    bool used;
    mock_node_t *node;
    size_t pos;
    bool writing;
    bool failed;
} mock_file_t;

static mock_file_t s_files[16];

static DIR *mock_opendir(const char *name)
{
    mock_node_t *n = find_node(name);
    if (n == NULL || !n->is_dir)
    {
        errno = ENOENT;
        return NULL;
    }
    for (int i = 0; i < 8; i++)
    {
        if (!s_dirs[i].used)
        {
            memset(&s_dirs[i], 0, sizeof(s_dirs[i]));
            s_dirs[i].used = true;
            strncpy(s_dirs[i].dirpath, name, sizeof(s_dirs[i].dirpath) - 1);
            s_dirs[i].index = 0;
            return (DIR *)&s_dirs[i];
        }
    }
    errno = EMFILE;
    return NULL;
}

static struct dirent *mock_readdir(DIR *dirp)
{
    mock_dir_t *d = (mock_dir_t *)dirp;
    if (d == NULL || !d->used)
    {
        return NULL;
    }
    size_t plen = strlen(d->dirpath);
    for (; d->index < MOCK_MAX_NODES; d->index++)
    {
        mock_node_t *n = &s_nodes[d->index];
        if (!n->used)
        {
            continue;
        }
        if (strncmp(n->path, d->dirpath, plen) != 0)
        {
            continue;
        }
        const char *rest = n->path + plen;
        if (rest[0] != '/')
        {
            continue;
        }
        rest++;
        if (rest[0] == '\0' || strchr(rest, '/') != NULL)
        {
            continue;
        }
        memset(&d->ent, 0, sizeof(d->ent));
        strncpy(d->ent.d_name, rest, sizeof(d->ent.d_name) - 1);
#ifdef DT_DIR
        d->ent.d_type = n->is_dir ? DT_DIR : DT_REG;
#endif
        d->index++;
        return &d->ent;
    }
    return NULL;
}

static int mock_closedir(DIR *dirp)
{
    mock_dir_t *d = (mock_dir_t *)dirp;
    if (d != NULL)
    {
        d->used = false;
    }
    return 0;
}

static int mock_stat(const char *path, struct stat *st)
{
    mock_node_t *n = find_node(path);
    if (n == NULL || st == NULL)
    {
        errno = ENOENT;
        return -1;
    }
    memset(st, 0, sizeof(*st));
    st->st_mode = n->is_dir ? S_IFDIR : S_IFREG;
    st->st_size = (off_t)n->len;
    return 0;
}

static int mock_mkdir(const char *path, mode_t mode)
{
    (void)mode;
    if (find_node(path) != NULL)
    {
        errno = EEXIST;
        return -1;
    }
    if (add_node(path, true) == NULL)
    {
        errno = ENOSPC;
        return -1;
    }
    return 0;
}

static int mock_rmdir(const char *path)
{
    mock_node_t *n = find_node(path);
    if (n == NULL || !n->is_dir)
    {
        errno = ENOENT;
        return -1;
    }
    if (has_children(path))
    {
        errno = ENOTEMPTY;
        return -1;
    }
    remove_node_path(path);
    return 0;
}

static int mock_unlink(const char *path)
{
    mock_node_t *n = find_node(path);
    if (n == NULL || n->is_dir)
    {
        errno = ENOENT;
        return -1;
    }
    remove_node_path(path);
    return 0;
}

static int mock_rename(const char *oldpath, const char *newpath)
{
    mock_node_t *n = find_node(oldpath);
    if (n == NULL)
    {
        errno = ENOENT;
        return -1;
    }
    strncpy(n->path, newpath, sizeof(n->path) - 1);
    n->path[sizeof(n->path) - 1] = '\0';
    return 0;
}

static FILE *mock_fopen(const char *path, const char *mode)
{
    bool write = (mode != NULL && strchr(mode, 'w') != NULL);
    bool read = (mode != NULL && strchr(mode, 'r') != NULL);
    mock_node_t *n = find_node(path);
    if (write)
    {
        if (n == NULL)
        {
            n = add_node(path, false);
        }
        if (n == NULL || n->is_dir)
        {
            errno = EIO;
            return NULL;
        }
        n->len = 0;
        memset(n->data, 0, sizeof(n->data));
    }
    else if (read)
    {
        if (n == NULL || n->is_dir)
        {
            errno = ENOENT;
            return NULL;
        }
    }
    for (int i = 0; i < 16; i++)
    {
        if (!s_files[i].used)
        {
            memset(&s_files[i], 0, sizeof(s_files[i]));
            s_files[i].used = true;
            s_files[i].node = n;
            s_files[i].writing = write;
            return (FILE *)&s_files[i];
        }
    }
    errno = EMFILE;
    return NULL;
}

static size_t mock_fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    mock_file_t *f = (mock_file_t *)stream;
    if (f == NULL || !f->used || f->node == NULL)
    {
        return 0;
    }
    size_t want = size * nmemb;
    size_t avail = (f->pos < f->node->len) ? (f->node->len - f->pos) : 0;
    size_t n = want < avail ? want : avail;
    if (n > 0)
    {
        memcpy(ptr, f->node->data + f->pos, n);
        f->pos += n;
    }
    return n;
}

static size_t mock_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    mock_file_t *f = (mock_file_t *)stream;
    if (f == NULL || !f->used || f->node == NULL)
    {
        return 0;
    }
    size_t n = size * nmemb;
    if (f->pos + n > sizeof(f->node->data))
    {
        f->failed = true;
        return 0;
    }
    memcpy(f->node->data + f->pos, ptr, n);
    f->pos += n;
    if (f->pos > f->node->len)
    {
        f->node->len = f->pos;
    }
    return n;
}

static int mock_fclose(FILE *stream)
{
    mock_file_t *f = (mock_file_t *)stream;
    if (f != NULL)
    {
        f->used = false;
    }
    return 0;
}

static int mock_fflush(FILE *stream)
{
    (void)stream;
    return 0;
}

static int mock_ferror(FILE *stream)
{
    mock_file_t *f = (mock_file_t *)stream;
    return (f != NULL && f->failed) ? 1 : 0;
}

static bool mock_sdcard_is_mounted(void)
{
    return s_sd_mounted;
}

static esp_err_t mock_flash_init(void)
{
    return ESP_OK;
}

static esp_err_t mock_sdcard_init(void)
{
    return ESP_OK;
}

/* Inject seam */
#define VFS_OPENDIR mock_opendir
#define VFS_READDIR mock_readdir
#define VFS_CLOSEDIR mock_closedir
#define VFS_STAT mock_stat
#define VFS_MKDIR mock_mkdir
#define VFS_RMDIR mock_rmdir
#define VFS_UNLINK mock_unlink
#define VFS_RENAME mock_rename
#define VFS_FOPEN mock_fopen
#define VFS_FREAD mock_fread
#define VFS_FWRITE mock_fwrite
#define VFS_FCLOSE mock_fclose
#define VFS_FFLUSH mock_fflush
#define VFS_FERROR mock_ferror
#define VFS_SDCARD_IS_MOUNTED mock_sdcard_is_mounted
#define VFS_FLASH_INIT mock_flash_init
#define VFS_SDCARD_INIT mock_sdcard_init

#include "filesystem.h"
#include "flash.h"
#include "sdcard.h"
/* Compile the real implementation with our seam */
#include "../components/filesystem/src/vfs.c"
#include "../components/filesystem/src/vfs_path.c"

void setUp(void)
{
    mock_backend_reset();
}

void tearDown(void) {}

/* ---- Arg validation ---- */

static void test_null_arguments_rejected(void)
{
    char buf[8];
    size_t n = 0;
    vfs_dir_entry_t entries[2];
    size_t count = 0;
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, vfs_list_dir(NULL, entries, 2, &count));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, vfs_list_dir("/flash", NULL, 2, &count));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, vfs_list_dir("/flash", entries, 2, NULL));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, vfs_read_file(NULL, buf, sizeof(buf), &n));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, vfs_read_file("/flash/a", NULL, sizeof(buf), &n));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, vfs_read_file("/flash/a", buf, sizeof(buf), NULL));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, vfs_write_file(NULL, "x", 1));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, vfs_mkdir(NULL));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, vfs_remove(NULL));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, vfs_copy_file(NULL, "/flash/b"));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, vfs_copy_file("/flash/a", NULL));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, vfs_rename_file(NULL, "/flash/b"));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, vfs_rename_file("/flash/a", NULL));
    bool is_dir = false;
    size_t size = 0;
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, vfs_stat(NULL, &is_dir, &size));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, vfs_stat("/flash", NULL, &size));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, vfs_stat("/flash", &is_dir, NULL));
}

/* ---- Virtual roots / exists / stat ---- */

static void test_virtual_root_and_flash_exist(void)
{
    TEST_ASSERT_TRUE(vfs_exists("/"));
    TEST_ASSERT_TRUE(vfs_exists("/flash"));
    TEST_ASSERT_TRUE(vfs_is_directory("/"));
    TEST_ASSERT_TRUE(vfs_is_directory("/flash"));
    bool is_dir = false;
    size_t size = 99;
    TEST_ASSERT_EQUAL(ESP_OK, vfs_stat("/flash", &is_dir, &size));
    TEST_ASSERT_TRUE(is_dir);
    TEST_ASSERT_EQUAL(0, size);
}

static void test_sdcard_exists_depends_on_mount(void)
{
    s_sd_mounted = true;
    TEST_ASSERT_TRUE(vfs_exists("/sdcard"));
    s_sd_mounted = false;
    TEST_ASSERT_FALSE(vfs_exists("/sdcard"));
    bool is_dir = false;
    size_t size = 0;
    TEST_ASSERT_EQUAL(ESP_ERR_NOT_FOUND, vfs_stat("/sdcard", &is_dir, &size));
}

static void test_list_dir_virtual_root(void)
{
    vfs_dir_entry_t entries[4];
    size_t count = 0;
    s_sd_mounted = true;
    TEST_ASSERT_EQUAL(ESP_OK, vfs_list_dir("/", entries, 4, &count));
    TEST_ASSERT_TRUE(count >= 1);
    bool found_flash = false;
    for (size_t i = 0; i < count; i++)
    {
        if (strcmp(entries[i].name, "flash") == 0)
        {
            found_flash = entries[i].is_dir;
        }
    }
    TEST_ASSERT_TRUE(found_flash);
}

static void test_resolve_and_exists_errors(void)
{
    TEST_ASSERT_FALSE(vfs_exists(NULL));
    TEST_ASSERT_FALSE(vfs_exists("/unknown/path"));
    TEST_ASSERT_FALSE(vfs_is_directory("/flash/missing"));
    bool is_dir = false;
    size_t size = 0;
    TEST_ASSERT_EQUAL(ESP_ERR_NOT_FOUND, vfs_stat("/flash/missing", &is_dir, &size));
}

/* ---- File/dir semantics ---- */

static void test_write_read_mkdir_remove_roundtrip(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, vfs_mkdir("/flash/dir"));
    TEST_ASSERT_TRUE(vfs_is_directory("/flash/dir"));
    TEST_ASSERT_EQUAL(ESP_OK, vfs_write_file("/flash/dir/a.txt", "hello", 5));
    TEST_ASSERT_TRUE(vfs_exists("/flash/dir/a.txt"));
    char buf[16];
    size_t n = 0;
    TEST_ASSERT_EQUAL(ESP_OK, vfs_read_file("/flash/dir/a.txt", buf, sizeof(buf), &n));
    TEST_ASSERT_EQUAL(5, n);
    TEST_ASSERT_EQUAL_MEMORY("hello", buf, 5);
    TEST_ASSERT_EQUAL(ESP_OK, vfs_remove("/flash/dir/a.txt"));
    TEST_ASSERT_FALSE(vfs_exists("/flash/dir/a.txt"));
}

static void test_read_missing_returns_not_found(void)
{
    char buf[8];
    size_t n = 0;
    TEST_ASSERT_EQUAL(ESP_ERR_NOT_FOUND, vfs_read_file("/flash/nope.txt", buf, sizeof(buf), &n));
}

static void test_remove_missing_and_dir_semantics(void)
{
    TEST_ASSERT_EQUAL(ESP_ERR_NOT_FOUND, vfs_remove("/flash/nope.txt"));
    TEST_ASSERT_EQUAL(ESP_OK, vfs_mkdir("/flash/d2"));
    TEST_ASSERT_EQUAL(ESP_OK, vfs_write_file("/flash/d2/f.txt", "x", 1));
    TEST_ASSERT_NOT_EQUAL(ESP_OK, vfs_remove("/flash/d2"));
    TEST_ASSERT_EQUAL(ESP_OK, vfs_remove("/flash/d2/f.txt"));
    TEST_ASSERT_EQUAL(ESP_OK, vfs_remove("/flash/d2"));
}

static void test_mkdir_existing_dir_ok(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, vfs_mkdir("/flash/edir"));
    TEST_ASSERT_EQUAL(ESP_OK, vfs_mkdir("/flash/edir"));
}

static void test_copy_file_basic_and_no_overwrite(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, vfs_write_file("/flash/src.txt", "abc", 3));
    TEST_ASSERT_EQUAL(ESP_OK, vfs_copy_file("/flash/src.txt", "/flash/dst.txt"));
    char buf[8];
    size_t n = 0;
    TEST_ASSERT_EQUAL(ESP_OK, vfs_read_file("/flash/dst.txt", buf, sizeof(buf), &n));
    TEST_ASSERT_EQUAL(3, n);
    TEST_ASSERT_EQUAL_MEMORY("abc", buf, 3);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, vfs_copy_file("/flash/src.txt", "/flash/dst.txt"));
    TEST_ASSERT_EQUAL(ESP_ERR_NOT_FOUND, vfs_copy_file("/flash/missing.txt", "/flash/other.txt"));
}

static void test_rename_file_same_mount(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, vfs_write_file("/flash/rn.txt", "z", 1));
    TEST_ASSERT_EQUAL(ESP_OK, vfs_rename_file("/flash/rn.txt", "/flash/rn2.txt"));
    TEST_ASSERT_FALSE(vfs_exists("/flash/rn.txt"));
    TEST_ASSERT_TRUE(vfs_exists("/flash/rn2.txt"));
    TEST_ASSERT_EQUAL(ESP_OK, vfs_write_file("/flash/rn.txt", "z", 1));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, vfs_rename_file("/flash/rn.txt", "/flash/rn2.txt"));
}

static void test_list_dir_contents(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, vfs_mkdir("/flash/listd"));
    TEST_ASSERT_EQUAL(ESP_OK, vfs_write_file("/flash/listd/one.txt", "1", 1));
    TEST_ASSERT_EQUAL(ESP_OK, vfs_mkdir("/flash/listd/sub"));
    vfs_dir_entry_t entries[8];
    size_t count = 0;
    TEST_ASSERT_EQUAL(ESP_OK, vfs_list_dir("/flash/listd", entries, 8, &count));
    bool found_file = false, found_dir = false;
    for (size_t i = 0; i < count; i++)
    {
        if (strcmp(entries[i].name, "one.txt") == 0)
        {
            found_file = !entries[i].is_dir;
        }
        if (strcmp(entries[i].name, "sub") == 0)
        {
            found_dir = entries[i].is_dir;
        }
    }
    TEST_ASSERT_TRUE(found_file);
    TEST_ASSERT_TRUE(found_dir);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_null_arguments_rejected);
    RUN_TEST(test_virtual_root_and_flash_exist);
    RUN_TEST(test_sdcard_exists_depends_on_mount);
    RUN_TEST(test_list_dir_virtual_root);
    RUN_TEST(test_resolve_and_exists_errors);
    RUN_TEST(test_write_read_mkdir_remove_roundtrip);
    RUN_TEST(test_read_missing_returns_not_found);
    RUN_TEST(test_remove_missing_and_dir_semantics);
    RUN_TEST(test_mkdir_existing_dir_ok);
    RUN_TEST(test_copy_file_basic_and_no_overwrite);
    RUN_TEST(test_rename_file_same_mount);
    RUN_TEST(test_list_dir_contents);
    return UNITY_END();
}

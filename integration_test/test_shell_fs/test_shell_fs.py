import re


BOOT_TIMEOUT = 30
CMD_TIMEOUT = 10
PROMPT_RE = re.compile(r"COS.+>")


def _wait_ready(dut):
    """Wait for shell boot (test app formats flash and seeds files on startup)."""
    dut.expect("SHELL_READY", timeout=BOOT_TIMEOUT)


# ── Boot & basics ──────────────────────────────────────────────────────


def test_shell_boots(dut):
    dut.expect("SHELL_READY", timeout=BOOT_TIMEOUT)


def test_pwd_default(dut):
    _wait_ready(dut)
    dut.write("pwd\n")
    dut.expect("/flash", timeout=CMD_TIMEOUT)


# ── File creation & listing ────────────────────────────────────────────


def test_touch_and_ls(dut):
    _wait_ready(dut)
    dut.write("touch /flash/newfile.txt\n")
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("ls /flash\n")
    dut.expect("newfile.txt", timeout=CMD_TIMEOUT)


def test_ls_shows_seeded_files(dut):
    _wait_ready(dut)
    dut.write("ls /flash\n")
    dut.expect("hello.txt", timeout=CMD_TIMEOUT)


# ── cat with actual content ────────────────────────────────────────────


def test_cat_with_content(dut):
    _wait_ready(dut)
    dut.write("cat /flash/hello.txt\n")
    dut.expect("Hello, COS!", timeout=CMD_TIMEOUT)


def test_cat_streams_large_file(dut):
    _wait_ready(dut)
    dut.write("cat /flash/large.txt\n")
    dut.expect(re.compile("A{600}B{600}"), timeout=CMD_TIMEOUT)


def test_autocomplete_command_and_path(dut):
    _wait_ready(dut)
    dut.write("ber\t\n")
    dut.expect("Usage: berry run", timeout=CMD_TIMEOUT)
    dut.write("cat /flash/hel\t\n")
    dut.expect("Hello, COS!", timeout=CMD_TIMEOUT)


def test_help_lists_registered_commands(dut):
    _wait_ready(dut)
    dut.write("help\n")
    dut.expect("cat", timeout=CMD_TIMEOUT)
    dut.expect("berry", timeout=CMD_TIMEOUT)


# ── hexdump ────────────────────────────────────────────────────────────


def test_hexdump(dut):
    _wait_ready(dut)
    dut.write("hexdump /flash/binary.bin\n")
    dut.expect("00 01 02 41 42 43", timeout=CMD_TIMEOUT)
    dut.expect("6 bytes", timeout=CMD_TIMEOUT)


# ── mkdir & cd ─────────────────────────────────────────────────────────


def test_mkdir_and_cd(dut):
    _wait_ready(dut)
    dut.write("mkdir /flash/testdir\n")
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("cd /flash/testdir\n")
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("pwd\n")
    dut.expect("/flash/testdir", timeout=CMD_TIMEOUT)


def test_nested_mkdir(dut):
    _wait_ready(dut)
    dut.write("mkdir /flash/outer\n")
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("mkdir /flash/outer/inner\n")
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("cd /flash/outer/inner\n")
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("pwd\n")
    dut.expect("/flash/outer/inner", timeout=CMD_TIMEOUT)


def test_cp_copies_without_overwriting(dut):
    _wait_ready(dut)
    dut.write("cp /flash/hello.txt /flash/copied.txt\n")
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("cat /flash/copied.txt\n")
    dut.expect("Hello, COS!", timeout=CMD_TIMEOUT)
    dut.write("cp /flash/large.txt /flash/large-copy.txt\n")
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("cat /flash/large-copy.txt\n")
    dut.expect(re.compile("A{600}B{600}"), timeout=CMD_TIMEOUT)
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("cp /flash/hello.txt /flash/copied.txt\n")
    dut.expect("already exists", timeout=CMD_TIMEOUT)


def test_cp_rejects_directory_copy(dut):
    _wait_ready(dut)
    dut.write("mkdir /flash/copy-source-dir\n")
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("cp /flash/copy-source-dir /flash/copy-destination\n")
    dut.expect("directory copies are not supported", timeout=CMD_TIMEOUT)


def test_mv_moves_file(dut):
    _wait_ready(dut)
    dut.write("touch /flash/move-source.txt\n")
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("mv /flash/move-source.txt /flash/move-destination.txt\n")
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("ls /flash\n")
    output = dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    listing = output.group().decode("utf-8", errors="replace")
    assert "move-destination.txt" in listing
    assert "move-source.txt" not in listing


def test_rmdir_only_removes_empty_directories(dut):
    _wait_ready(dut)
    dut.write("mkdir /flash/empty-to-remove\n")
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("rmdir /flash/empty-to-remove\n")
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)

    dut.write("mkdir /flash/nonempty-to-remove\n")
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("touch /flash/nonempty-to-remove/file.txt\n")
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("rmdir /flash/nonempty-to-remove\n")
    dut.expect("directory must be empty", timeout=CMD_TIMEOUT)


# ── Relative path resolution ──────────────────────────────────────────


def test_cd_relative(dut):
    _wait_ready(dut)
    dut.write("mkdir /flash/reldir\n")
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("cd /flash\n")
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("cd reldir\n")
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("pwd\n")
    dut.expect("/flash/reldir", timeout=CMD_TIMEOUT)


# ── rm ─────────────────────────────────────────────────────────────────


def test_rm_file(dut):
    _wait_ready(dut)
    dut.write("touch /flash/deleteme.txt\n")
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("rm /flash/deleteme.txt\n")
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("ls /flash\n")
    output = dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    assert "deleteme.txt" not in output.group().decode("utf-8", errors="replace")


# ── info command (flash usage) ─────────────────────────────────────────


def test_info_command(dut):
    _wait_ready(dut)
    dut.write("info\n")
    dut.expect("Flash", timeout=CMD_TIMEOUT)
    dut.expect(re.compile(r"Total:\s+\d+"), timeout=CMD_TIMEOUT)


# ── Alias smoke tests ─────────────────────────────────────────────────


def test_alias_dir(dut):
    _wait_ready(dut)
    dut.write("dir /flash\n")
    dut.expect("hello.txt", timeout=CMD_TIMEOUT)


def test_alias_type(dut):
    _wait_ready(dut)
    dut.write("type /flash/hello.txt\n")
    dut.expect("Hello, COS!", timeout=CMD_TIMEOUT)


def test_alias_md(dut):
    _wait_ready(dut)
    dut.write("md /flash/mdtest\n")
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("cd /flash/mdtest\n")
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("pwd\n")
    dut.expect("/flash/mdtest", timeout=CMD_TIMEOUT)


def test_alias_del(dut):
    _wait_ready(dut)
    dut.write("touch /flash/delalias.txt\n")
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("del /flash/delalias.txt\n")
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("ls /flash\n")
    output = dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    assert "delalias.txt" not in output.group().decode("utf-8", errors="replace")


# ── Error cases ────────────────────────────────────────────────────────


def test_cd_nonexistent(dut):
    _wait_ready(dut)
    dut.write("cd /flash/does_not_exist\n")
    dut.expect("not a directory", timeout=CMD_TIMEOUT)


def test_cat_nonexistent(dut):
    _wait_ready(dut)
    dut.write("cat /flash/no_such_file.txt\n")
    dut.expect("cannot read", timeout=CMD_TIMEOUT)


def test_rm_nonexistent(dut):
    _wait_ready(dut)
    dut.write("rm /flash/ghost.txt\n")
    dut.expect("failed to remove", timeout=CMD_TIMEOUT)

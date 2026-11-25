import re
import time

import pytest


BOOT_TIMEOUT = 30
CMD_TIMEOUT = 10
PROMPT_RE = re.compile(r"COS.+>")


def _wait_ready(dut):
    """Wait for shell prompt."""
    dut.write("\n")
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)


# ── Boot & basic operations (single boot to reduce QEMU hang exposure) ─


@pytest.mark.skip(
    reason="TODO(shell-fs-qemu): QEMU esp_partition_read hang in LittleFS metadata scans; re-enable when mitigated (docs/plan/2026-11-15-follow-up-shell-fs-qemu-hangs.md)."
)
def test_shell_fs(dut):
    dut.expect("AUTOEXEC_OK", timeout=BOOT_TIMEOUT)
    dut.expect("SHELL_READY", timeout=BOOT_TIMEOUT)
    _wait_ready(dut)

    # pwd default
    dut.write("pwd\n")
    dut.expect("/flash", timeout=CMD_TIMEOUT)
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)

    # touch and ls
    dut.write("touch /flash/newfile.txt\n")
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("ls /flash\n")
    dut.expect("newfile.txt", timeout=CMD_TIMEOUT)
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)

    # ls shows seeded files
    dut.write("ls /flash\n")
    dut.expect("hello.txt", timeout=CMD_TIMEOUT)
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)

    # cat with actual content
    dut.write("cat /flash/hello.txt\n")
    dut.expect("Hello, COS!", timeout=CMD_TIMEOUT)
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)

    # cat streams large file
    dut.write("cat /flash/large.txt\n")
    dut.expect(re.compile(r"A{100,}B{100,}"), timeout=CMD_TIMEOUT)
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)

    # help lists registered commands
    dut.write("help\n")
    dut.expect("cat", timeout=CMD_TIMEOUT)
    dut.expect("berry", timeout=CMD_TIMEOUT)
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)

    # ping/nslookup report unavailable network
    dut.write("ping example.com\n")
    dut.expect(r"ping: example.com failed \(ESP_ERR_INVALID_STATE\)", timeout=CMD_TIMEOUT)
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("nslookup example.com\n")
    dut.expect(r"nslookup: example.com failed \(ESP_ERR_INVALID_STATE\)", timeout=CMD_TIMEOUT)
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)

    # berry console read line async
    dut.write("berry run /flash/berry_input.be\n")
    dut.expect("Input: ", timeout=BOOT_TIMEOUT)
    dut.expect("INPUT_STARTED", timeout=CMD_TIMEOUT)
    dut.write("Cos user\n")
    dut.expect("INPUT_VALUE:Cos user", timeout=CMD_TIMEOUT)
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)

    # berry console read line can be cancelled
    dut.write("berry run /flash/berry_input.be\n")
    dut.expect("Input: ", timeout=BOOT_TIMEOUT)
    dut.expect("INPUT_STARTED", timeout=CMD_TIMEOUT)
    time.sleep(0.2)
    dut.write("\x03")
    dut.expect("INPUT_CANCELLED", timeout=CMD_TIMEOUT)
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)

    # hexdump
    dut.write("hexdump /flash/binary.bin\n")
    dut.expect("00 01 02 41 42 43", timeout=CMD_TIMEOUT)
    dut.expect("6 bytes", timeout=CMD_TIMEOUT)
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)

    # mkdir and cd
    dut.write("mkdir /flash/testdir\n")
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("cd /flash/testdir\n")
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("pwd\n")
    dut.expect("/flash/testdir", timeout=CMD_TIMEOUT)
    dut.write("cd /flash\n")
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)

    # info command (flash usage)
    dut.write("info\n")
    dut.expect("Flash", timeout=CMD_TIMEOUT)
    dut.expect(re.compile(r"Total:\s+\d+"), timeout=CMD_TIMEOUT)
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)

    # alias smoke tests
    dut.write("dir /flash\n")
    dut.expect("hello.txt", timeout=CMD_TIMEOUT)
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("type /flash/hello.txt\n")
    dut.expect("Hello, COS!", timeout=CMD_TIMEOUT)
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("md /flash/mdtest\n")
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("cd /flash/mdtest\n")
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("pwd\n")
    dut.expect("/flash/mdtest", timeout=CMD_TIMEOUT)
    dut.write("cd /flash\n")
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)

    # error cases
    dut.write("cd /flash/does_not_exist\n")
    dut.expect("not a directory", timeout=CMD_TIMEOUT)
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("cat /flash/no_such_file.txt\n")
    dut.expect("cannot read", timeout=CMD_TIMEOUT)
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("rm /flash/ghost.txt\n")
    dut.expect("failed to remove", timeout=CMD_TIMEOUT)
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)


# ── Skipped: QEMU esp_partition_read hang in LittleFS metadata scans ──
# Re-enable when mitigated (docs/plan/2026-11-15-follow-up-shell-fs-qemu-hangs.md).


@pytest.mark.skip(
    reason="TODO(shell-fs-qemu): QEMU esp_partition_read hang in LittleFS metadata scans; re-enable when mitigated (docs/plan/2026-11-15-follow-up-shell-fs-qemu-hangs.md)."
)
def test_autocomplete_command_and_path(dut):
    _wait_ready(dut)
    dut.write("ber\t\n")
    dut.expect("Usage: berry run", timeout=CMD_TIMEOUT)
    dut.write("cat /flash/hel\t\n")
    dut.expect("Hello, COS!", timeout=CMD_TIMEOUT)


@pytest.mark.skip(
    reason="TODO(shell-fs-qemu): QEMU esp_partition_read hang in LittleFS metadata scans; re-enable when mitigated (docs/plan/2026-11-15-follow-up-shell-fs-qemu-hangs.md)."
)
def test_shell_alias_lifecycle(dut):
    _wait_ready(dut)
    dut.write("alias ll ls\n")
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("ll /flash\n")
    dut.expect("hello.txt", timeout=CMD_TIMEOUT)
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("unalias ll\n")
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("ll\n")
    dut.expect("Unknown command: ll", timeout=CMD_TIMEOUT)


@pytest.mark.skip(
    reason="TODO(shell-fs-qemu): QEMU esp_partition_read hang in LittleFS metadata scans; re-enable when mitigated (docs/plan/2026-11-15-follow-up-shell-fs-qemu-hangs.md)."
)
def test_berry_filesystem_and_network_apis(dut):
    _wait_ready(dut)
    dut.write("berry run /flash/berry_api.be\n")
    fs_markers = (
        "FS_LIST_OK",
        "FS_EMPTY_OK",
        "FS_RELATIVE_OK",
        "FS_PARENT_OK",
        "FS_INVALID_PATH_OK",
        "FS_TEXT_OK",
        "FS_JSON_OK",
        "BERRY_FS_API_DONE",
    )
    for marker in fs_markers:
        dut.expect(marker, timeout=CMD_TIMEOUT)
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)

    dut.write("berry run /flash/berry_net.be\n")
    net_markers = (
        "NET_NO_WIFI_OK",
        "NET_TIMEOUT_BOUNDS_OK",
        "NET_INVALID_HOST_OK",
        "BERRY_NET_API_DONE",
    )
    for marker in net_markers:
        dut.expect(marker, timeout=CMD_TIMEOUT)
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)


@pytest.mark.skip(
    reason="TODO(shell-fs-qemu): QEMU esp_partition_read hang in LittleFS metadata scans; re-enable when mitigated (docs/plan/2026-11-15-follow-up-shell-fs-qemu-hangs.md)."
)
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


@pytest.mark.skip(
    reason="TODO(shell-fs-qemu): QEMU esp_partition_read hang in LittleFS metadata scans; re-enable when mitigated (docs/plan/2026-11-15-follow-up-shell-fs-qemu-hangs.md)."
)
def test_cp_copies_without_overwriting(dut):
    _wait_ready(dut)
    dut.write("cp /flash/hello.txt /flash/copied.txt\n")
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("cat /flash/copied.txt\n")
    dut.expect("Hello, COS!", timeout=CMD_TIMEOUT)
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("cp /flash/large.txt /flash/large-copy.txt\n")
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("cat /flash/large-copy.txt\n")
    dut.expect(re.compile(r"A{100,}B{100,}"), timeout=CMD_TIMEOUT)
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("cp /flash/hello.txt /flash/copied.txt\n")
    dut.expect("already exists", timeout=CMD_TIMEOUT)


@pytest.mark.skip(
    reason="TODO(shell-fs-qemu): QEMU esp_partition_read hang in LittleFS metadata scans; re-enable when mitigated (docs/plan/2026-11-15-follow-up-shell-fs-qemu-hangs.md)."
)
def test_cp_rejects_directory_copy(dut):
    _wait_ready(dut)
    dut.write("mkdir /flash/copy-source-dir\n")
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("cp /flash/copy-source-dir /flash/copy-destination\n")
    dut.expect("directory copies are not supported", timeout=CMD_TIMEOUT)


@pytest.mark.skip(
    reason="TODO(shell-fs-qemu): QEMU esp_partition_read hang in LittleFS metadata scans; re-enable when mitigated (docs/plan/2026-11-15-follow-up-shell-fs-qemu-hangs.md)."
)
def test_mv_moves_file(dut):
    _wait_ready(dut)
    dut.write("touch /flash/move-source.txt\n")
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("mv /flash/move-source.txt /flash/move-destination.txt\n")
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("ls /flash\n")
    dut.expect("move-destination.txt", timeout=CMD_TIMEOUT)
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("cat /flash/move-source.txt\n")
    dut.expect("cannot read", timeout=CMD_TIMEOUT)


@pytest.mark.skip(
    reason="TODO(shell-fs-qemu): QEMU esp_partition_read hang in LittleFS metadata scans; re-enable when mitigated (docs/plan/2026-11-15-follow-up-shell-fs-qemu-hangs.md)."
)
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
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)


@pytest.mark.skip(
    reason="TODO(shell-fs-qemu): QEMU esp_partition_read hang in LittleFS metadata scans; re-enable when mitigated (docs/plan/2026-11-15-follow-up-shell-fs-qemu-hangs.md)."
)
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


@pytest.mark.skip(
    reason="TODO(shell-fs-qemu): QEMU esp_partition_read hang in LittleFS metadata scans; re-enable when mitigated (docs/plan/2026-11-15-follow-up-shell-fs-qemu-hangs.md)."
)
def test_rm_file(dut):
    _wait_ready(dut)
    dut.write("touch /flash/deleteme.txt\n")
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("rm /flash/deleteme.txt\n")
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("ls /flash\n")
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT, not_matching=["deleteme.txt"])


@pytest.mark.skip(
    reason="TODO(shell-fs-qemu): QEMU esp_partition_read hang in LittleFS metadata scans; re-enable when mitigated (docs/plan/2026-11-15-follow-up-shell-fs-qemu-hangs.md)."
)
def test_alias_del(dut):
    _wait_ready(dut)
    dut.write("touch /flash/delalias.txt\n")
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("del /flash/delalias.txt\n")
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)
    dut.write("ls /flash\n")
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT, not_matching=["delalias.txt"])


def test_berry_system_sleep_returns_promise(dut):
    dut.expect("SHELL_READY", timeout=BOOT_TIMEOUT)
    _wait_ready(dut)
    dut.write('berry eval "import system; print(type(system.sleep(50)))"\n')
    dut.expect("instance", timeout=CMD_TIMEOUT)
    dut.expect(PROMPT_RE, timeout=CMD_TIMEOUT)

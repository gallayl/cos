import re


BOOT_TIMEOUT = 30
CMD_TIMEOUT = 10


def _wait_ready_and_format(dut):
    """Wait for shell boot and format flash (QEMU starts with unformatted partition)."""
    dut.expect("SHELL_READY", timeout=BOOT_TIMEOUT)
    dut.write("format\n")
    dut.expect("Format complete", timeout=CMD_TIMEOUT)


def test_shell_boots(dut):
    dut.expect("SHELL_READY", timeout=BOOT_TIMEOUT)


def test_pwd_default(dut):
    dut.expect("SHELL_READY", timeout=BOOT_TIMEOUT)
    dut.write("pwd\n")
    dut.expect("/flash", timeout=CMD_TIMEOUT)


def test_touch_and_ls(dut):
    _wait_ready_and_format(dut)
    dut.write("touch /flash/hello.txt\n")
    dut.expect(re.compile(r"COS.+>"), timeout=CMD_TIMEOUT)
    dut.write("ls /flash\n")
    dut.expect("hello.txt", timeout=CMD_TIMEOUT)


def test_mkdir_and_cd(dut):
    _wait_ready_and_format(dut)
    dut.write("mkdir /flash/testdir\n")
    dut.expect(re.compile(r"COS.+>"), timeout=CMD_TIMEOUT)
    dut.write("cd /flash/testdir\n")
    dut.expect(re.compile(r"COS.+>"), timeout=CMD_TIMEOUT)
    dut.write("pwd\n")
    dut.expect("/flash/testdir", timeout=CMD_TIMEOUT)


def test_cat_written_file(dut):
    _wait_ready_and_format(dut)
    dut.write("touch /flash/readme.txt\n")
    dut.expect(re.compile(r"COS.+>"), timeout=CMD_TIMEOUT)
    dut.write("cat /flash/readme.txt\n")
    dut.expect(re.compile(r"COS.+>"), timeout=CMD_TIMEOUT)


def test_rm_file(dut):
    _wait_ready_and_format(dut)
    dut.write("touch /flash/deleteme.txt\n")
    dut.expect(re.compile(r"COS.+>"), timeout=CMD_TIMEOUT)
    dut.write("rm /flash/deleteme.txt\n")
    dut.expect(re.compile(r"COS.+>"), timeout=CMD_TIMEOUT)
    dut.write("ls /flash\n")
    output = dut.expect(re.compile(r"COS.+>"), timeout=CMD_TIMEOUT)
    assert "deleteme.txt" not in output.group().decode("utf-8", errors="replace")

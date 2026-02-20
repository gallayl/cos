import re


BOOT_TIMEOUT = 30
CMD_TIMEOUT = 10
PROMPT_RE = re.compile(r"COS.+>")


def _wait_ready(dut):
    dut.expect("SYSTEM_READY", timeout=BOOT_TIMEOUT)


def test_system_boots(dut):
    dut.expect("SYSTEM_READY", timeout=BOOT_TIMEOUT)


def test_uptime(dut):
    _wait_ready(dut)
    dut.write("uptime\n")
    dut.expect(re.compile(r"Up \d+s"), timeout=CMD_TIMEOUT)


def test_memory(dut):
    _wait_ready(dut)
    dut.write("memory\n")
    dut.expect(re.compile(r"Free heap:\s+\d+"), timeout=CMD_TIMEOUT)
    dut.expect(re.compile(r"Min free heap:\s+\d+"), timeout=CMD_TIMEOUT)
    dut.expect(re.compile(r"Total heap:\s+\d+"), timeout=CMD_TIMEOUT)


def test_info(dut):
    _wait_ready(dut)
    dut.write("info\n")
    dut.expect("IDF version:", timeout=CMD_TIMEOUT)
    dut.expect(re.compile(r"Free heap:\s+\d+"), timeout=CMD_TIMEOUT)

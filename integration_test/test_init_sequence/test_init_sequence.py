BOOT_TIMEOUT = 30


def test_init_completes(dut):
    dut.expect("INIT_OK", timeout=BOOT_TIMEOUT)


def test_nvs_initializes(dut):
    dut.expect("NVS initialized", timeout=BOOT_TIMEOUT)


def test_filesystem_initializes(dut):
    dut.expect("Filesystem initialized", timeout=BOOT_TIMEOUT)


def test_system_initializes(dut):
    dut.expect("System initialized", timeout=BOOT_TIMEOUT)


def test_shell_initializes(dut):
    dut.expect("Shell initialized", timeout=BOOT_TIMEOUT)

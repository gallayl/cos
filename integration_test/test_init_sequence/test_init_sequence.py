BOOT_TIMEOUT = 30


def test_init_sequence(dut):
    dut.expect("NVS initialized", timeout=BOOT_TIMEOUT)
    dut.expect("Filesystem initialized", timeout=BOOT_TIMEOUT)
    dut.expect("System initialized", timeout=BOOT_TIMEOUT)
    dut.expect("Shell initialized", timeout=BOOT_TIMEOUT)
    dut.expect("INIT_OK", timeout=BOOT_TIMEOUT)

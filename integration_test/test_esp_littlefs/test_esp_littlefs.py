import re


def test_esp_littlefs_unity_suite(dut):
    dut.expect("Running esp_littlefs migrated tests", timeout=60)
    dut.expect(re.compile(r"\d+ Tests \d+ Failures \d+ Ignored"), timeout=2400)
    dut.expect("0 Failures", timeout=10)

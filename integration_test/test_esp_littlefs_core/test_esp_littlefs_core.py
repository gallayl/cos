import re

import pytest


@pytest.mark.timeout(960)
def test_esp_littlefs_core_suite(dut):
    dut.expect("Running esp_littlefs_core tests", timeout=60)
    dut.expect(re.compile(r"(\d+ Tests \d+ Failures \d+ Ignored|main_task: Returned from app_main\(\))"), timeout=900)

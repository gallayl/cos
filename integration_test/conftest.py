import pytest


@pytest.fixture(autouse=True)
def timeout_default(dut):
    """Let each test app boot before assertions."""
    yield dut

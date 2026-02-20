def test_stability(dut):
    dut.expect_unity_test_output(timeout=120)

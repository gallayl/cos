#include "unity.h"
#include "wifi.h"
#include "esp_netif.h"
#include "mock_esp_wifi.h"
#include "mock_esp_event.h"
#include "mock_esp_netif.h"
#include "mock_freertos.h"
#include "mock_console.h"

void setUp(void)
{
    mock_wifi_reset();
    mock_esp_event_reset();
    mock_esp_netif_reset();
    mock_freertos_reset();
    mock_console_reset();
}

void tearDown(void)
{
}

/* --- wifi_signal_strength_str --- */

void test_signal_strength_amazing(void)
{
    TEST_ASSERT_EQUAL_STRING("Amazing", wifi_signal_strength_str(-20));
    TEST_ASSERT_EQUAL_STRING("Amazing", wifi_signal_strength_str(-29));
}

void test_signal_strength_very_good(void)
{
    TEST_ASSERT_EQUAL_STRING("Very good", wifi_signal_strength_str(-30));
    TEST_ASSERT_EQUAL_STRING("Very good", wifi_signal_strength_str(-50));
    TEST_ASSERT_EQUAL_STRING("Very good", wifi_signal_strength_str(-66));
}

void test_signal_strength_okay(void)
{
    TEST_ASSERT_EQUAL_STRING("Okay", wifi_signal_strength_str(-67));
    TEST_ASSERT_EQUAL_STRING("Okay", wifi_signal_strength_str(-69));
}

void test_signal_strength_not_good(void)
{
    TEST_ASSERT_EQUAL_STRING("Not good", wifi_signal_strength_str(-70));
    TEST_ASSERT_EQUAL_STRING("Not good", wifi_signal_strength_str(-79));
}

void test_signal_strength_unusable(void)
{
    TEST_ASSERT_EQUAL_STRING("Unusable", wifi_signal_strength_str(-80));
    TEST_ASSERT_EQUAL_STRING("Unusable", wifi_signal_strength_str(-89));
}

void test_signal_strength_unknown(void)
{
    TEST_ASSERT_EQUAL_STRING("Unknown", wifi_signal_strength_str(-90));
    TEST_ASSERT_EQUAL_STRING("Unknown", wifi_signal_strength_str(-100));
}

/* --- wifi_auth_mode_str --- */

void test_auth_mode_open(void)
{
    TEST_ASSERT_EQUAL_STRING("OPEN", wifi_auth_mode_str(WIFI_AUTH_OPEN));
}

void test_auth_mode_wpa2_psk(void)
{
    TEST_ASSERT_EQUAL_STRING("WPA2_PSK", wifi_auth_mode_str(WIFI_AUTH_WPA2_PSK));
}

void test_auth_mode_wpa3_psk(void)
{
    TEST_ASSERT_EQUAL_STRING("WPA3_PSK", wifi_auth_mode_str(WIFI_AUTH_WPA3_PSK));
}

void test_auth_mode_wep(void)
{
    TEST_ASSERT_EQUAL_STRING("WEP", wifi_auth_mode_str(WIFI_AUTH_WEP));
}

void test_auth_mode_unknown(void)
{
    TEST_ASSERT_EQUAL_STRING("UNKNOWN", wifi_auth_mode_str(WIFI_AUTH_MAX));
}

/* --- wifi_has_stored_credentials --- */

void test_has_stored_credentials_false_when_empty(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, wifi_init());
    TEST_ASSERT_FALSE(wifi_has_stored_credentials());
}

void test_has_stored_credentials_true_when_set(void)
{
    mock_wifi_set_stored_sta_config("TestNetwork", "password123");
    TEST_ASSERT_TRUE(wifi_has_stored_credentials());
}

/* --- wifi_is_connected --- */

void test_is_connected_false_initially(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, wifi_init());
    TEST_ASSERT_FALSE(wifi_is_connected());
}

void test_is_connected_true_after_got_ip(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, wifi_init());

    mock_freertos_set_bits(BIT1); /* WIFI_GOT_IP_BIT */
    TEST_ASSERT_TRUE(wifi_is_connected());
}

void test_is_connected_false_after_disconnect(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, wifi_init());

    mock_freertos_set_bits(BIT0 | BIT1);
    TEST_ASSERT_TRUE(wifi_is_connected());

    mock_freertos_clear_bits(BIT0 | BIT1);
    TEST_ASSERT_FALSE(wifi_is_connected());
}

/* --- wifi_init --- */

void test_init_no_credentials_starts_ap_mode(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, wifi_init());
    TEST_ASSERT_EQUAL(WIFI_MODE_AP, mock_wifi_get_mode());
    TEST_ASSERT_EQUAL(0, mock_wifi_get_connect_count());
}

void test_init_with_credentials_starts_apsta_and_connects(void)
{
    mock_wifi_set_stored_sta_config("MyNetwork", "MyPassword");
    TEST_ASSERT_EQUAL(ESP_OK, wifi_init());
    TEST_ASSERT_EQUAL(WIFI_MODE_APSTA, mock_wifi_get_mode());
    TEST_ASSERT_EQUAL(1, mock_wifi_get_connect_count());
}

void test_init_configures_ap_ssid(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, wifi_init());

    const wifi_config_t *ap_cfg = mock_wifi_get_ap_config();
    TEST_ASSERT_EQUAL_STRING("COS", (const char *)ap_cfg->ap.ssid);
    TEST_ASSERT_EQUAL_STRING("cos12345", (const char *)ap_cfg->ap.password);
    TEST_ASSERT_EQUAL(4, ap_cfg->ap.max_connection);
    TEST_ASSERT_EQUAL(WIFI_AUTH_WPA2_PSK, ap_cfg->ap.authmode);
}

/* --- wifi_connect --- */

void test_connect_sets_sta_config(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, wifi_init());
    TEST_ASSERT_EQUAL(ESP_OK, wifi_connect("NewNet", "NewPass"));

    const wifi_config_t *sta_cfg = mock_wifi_get_sta_config();
    TEST_ASSERT_EQUAL_STRING("NewNet", (const char *)sta_cfg->sta.ssid);
    TEST_ASSERT_EQUAL_STRING("NewPass", (const char *)sta_cfg->sta.password);
}

void test_connect_switches_ap_to_apsta(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, wifi_init());
    TEST_ASSERT_EQUAL(WIFI_MODE_AP, mock_wifi_get_mode());

    TEST_ASSERT_EQUAL(ESP_OK, wifi_connect("Net", "Pass1234"));
    TEST_ASSERT_EQUAL(WIFI_MODE_APSTA, mock_wifi_get_mode());
}

void test_connect_null_ssid_returns_error(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, wifi_init());
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, wifi_connect(NULL, "pass"));
}

void test_connect_empty_ssid_returns_error(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, wifi_init());
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, wifi_connect("", "pass"));
}

/* --- wifi_scan --- */

void test_scan_returns_results(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, wifi_init());

    wifi_ap_record_t mock_aps[2] = {
        {.ssid = "Network1", .rssi = -45, .authmode = WIFI_AUTH_WPA2_PSK},
        {.ssid = "Network2", .rssi = -70, .authmode = WIFI_AUTH_OPEN},
    };
    mock_wifi_set_scan_results(mock_aps, 2);

    wifi_ap_record_t results[10];
    uint16_t count = 0;
    TEST_ASSERT_EQUAL(ESP_OK, wifi_scan(results, &count, 10));
    TEST_ASSERT_EQUAL(2, count);
    TEST_ASSERT_EQUAL_STRING("Network1", (const char *)results[0].ssid);
    TEST_ASSERT_EQUAL(-45, results[0].rssi);
    TEST_ASSERT_EQUAL_STRING("Network2", (const char *)results[1].ssid);
}

void test_scan_null_args_returns_error(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, wifi_init());
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, wifi_scan(NULL, NULL, 0));
}

/* --- wifi_disconnect --- */

void test_disconnect_calls_esp_wifi_disconnect(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, wifi_init());
    int before = mock_wifi_get_disconnect_count();
    TEST_ASSERT_EQUAL(ESP_OK, wifi_disconnect());
    TEST_ASSERT_EQUAL(before + 1, mock_wifi_get_disconnect_count());
}

/* --- Event handling via mock_esp_event_fire --- */

void test_event_handler_sets_bits_on_connect(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, wifi_init());
    TEST_ASSERT_EQUAL(0, mock_freertos_get_bits());

    mock_esp_event_fire(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, NULL);
    TEST_ASSERT_BITS(BIT0, BIT0, mock_freertos_get_bits());
}

void test_event_handler_sets_bits_on_got_ip(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, wifi_init());

    ip_event_got_ip_t ip_event = {.ip_info = {.ip = {.addr = 0x0100A8C0}}}; /* 192.168.0.1 */
    mock_esp_event_fire(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event);
    TEST_ASSERT_BITS(BIT1, BIT1, mock_freertos_get_bits());
}

void test_event_handler_clears_bits_on_disconnect(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, wifi_init());

    mock_freertos_set_bits(BIT0 | BIT1);
    mock_esp_event_fire(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, NULL);
    TEST_ASSERT_EQUAL(0, mock_freertos_get_bits());
}

/* --- Shell command registration --- */

void test_wifi_command_registered(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, wifi_init());

    /* "wifi" with no args should print info and return 0 */
    char *argv[] = {"wifi"};
    int ret = mock_console_run_cmd("wifi", 1, argv);
    TEST_ASSERT_EQUAL(0, ret);
}

int main(void)
{
    UNITY_BEGIN();

    /* Signal strength */
    RUN_TEST(test_signal_strength_amazing);
    RUN_TEST(test_signal_strength_very_good);
    RUN_TEST(test_signal_strength_okay);
    RUN_TEST(test_signal_strength_not_good);
    RUN_TEST(test_signal_strength_unusable);
    RUN_TEST(test_signal_strength_unknown);

    /* Auth mode */
    RUN_TEST(test_auth_mode_open);
    RUN_TEST(test_auth_mode_wpa2_psk);
    RUN_TEST(test_auth_mode_wpa3_psk);
    RUN_TEST(test_auth_mode_wep);
    RUN_TEST(test_auth_mode_unknown);

    /* Credentials */
    RUN_TEST(test_has_stored_credentials_false_when_empty);
    RUN_TEST(test_has_stored_credentials_true_when_set);

    /* Connection state */
    RUN_TEST(test_is_connected_false_initially);
    RUN_TEST(test_is_connected_true_after_got_ip);
    RUN_TEST(test_is_connected_false_after_disconnect);

    /* Init */
    RUN_TEST(test_init_no_credentials_starts_ap_mode);
    RUN_TEST(test_init_with_credentials_starts_apsta_and_connects);
    RUN_TEST(test_init_configures_ap_ssid);

    /* Connect */
    RUN_TEST(test_connect_sets_sta_config);
    RUN_TEST(test_connect_switches_ap_to_apsta);
    RUN_TEST(test_connect_null_ssid_returns_error);
    RUN_TEST(test_connect_empty_ssid_returns_error);

    /* Scan */
    RUN_TEST(test_scan_returns_results);
    RUN_TEST(test_scan_null_args_returns_error);

    /* Disconnect */
    RUN_TEST(test_disconnect_calls_esp_wifi_disconnect);

    /* Event handling */
    RUN_TEST(test_event_handler_sets_bits_on_connect);
    RUN_TEST(test_event_handler_sets_bits_on_got_ip);
    RUN_TEST(test_event_handler_clears_bits_on_disconnect);

    /* Shell command */
    RUN_TEST(test_wifi_command_registered);

    return UNITY_END();
}

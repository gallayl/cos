#include "berry.h"
#include "network.h"

#include "esp_err.h"

#include <cstdio>
#include <cstring>

static void map_set_bool(bvm *vm, const char *key, bool value)
{
    be_pushstring(vm, key);
    be_pushbool(vm, value);
    be_data_insert(vm, -3);
    be_pop(vm, 2);
}

static void map_set_string(bvm *vm, const char *key, const char *value)
{
    be_pushstring(vm, key);
    be_pushstring(vm, value != nullptr ? value : "");
    be_data_insert(vm, -3);
    be_pop(vm, 2);
}

static void map_set_int(bvm *vm, const char *key, bint value)
{
    be_pushstring(vm, key);
    be_pushint(vm, value);
    be_data_insert(vm, -3);
    be_pop(vm, 2);
}

static void map_set_bytes(bvm *vm, const char *key, const char *value, size_t length)
{
    be_pushstring(vm, key);
    if (value == nullptr || length == 0)
    {
        be_pushstring(vm, "");
    }
    else
    {
        be_pushnstring(vm, value, length);
    }
    be_data_insert(vm, -3);
    be_pop(vm, 2);
}

static int push_status(bvm *vm, esp_err_t err)
{
    if (err == ESP_OK)
    {
        be_pushbool(vm, true);
    }
    else
    {
        be_pushstring(vm, esp_err_to_name(err));
    }
    be_return(vm);
}

static bool berry_json_value(bvm *vm, int index)
{
    return be_isnil(vm, index) || be_isbool(vm, index) || be_isnumber(vm, index) || be_isstring(vm, index) ||
           be_islist(vm, index) || be_islistinstance(vm, index) || be_ismap(vm, index) || be_ismapinstance(vm, index);
}

static bool call_json_method(bvm *vm, const char *method, int value_index)
{
    value_index = be_absindex(vm, value_index);
    const int base = be_top(vm);
    if (!be_getglobal(vm, "json") || !be_getmember(vm, -1, method) || be_isnil(vm, -1))
    {
        be_pop(vm, be_top(vm) - base);
        return false;
    }

    be_remove(vm, -2);
    be_pushvalue(vm, value_index);
    if (be_pcall(vm, 1) != 0)
    {
        be_pop(vm, be_top(vm) - base);
        return false;
    }
    if (be_top(vm) == base + 2)
    {
        be_remove(vm, base + 2);
    }
    if (be_top(vm) != base + 1)
    {
        be_pop(vm, be_top(vm) - base);
        return false;
    }
    return true;
}

static bool json_space(char character)
{
    return character == ' ' || character == '\t' || character == '\r' || character == '\n';
}

static bool known_fetch_field(const char *key)
{
    return std::strcmp(key, "method") == 0 || std::strcmp(key, "headers") == 0 || std::strcmp(key, "body") == 0 ||
           std::strcmp(key, "response_type") == 0 || std::strcmp(key, "timeout_ms") == 0 ||
           std::strcmp(key, "total_timeout_ms") == 0;
}

static bool push_field(bvm *vm, int map_index, const char *key)
{
    const int base = be_top(vm);
    map_index = be_absindex(vm, map_index);
    if (!be_ismapinstance(vm, map_index) || !be_getmember(vm, map_index, ".p") || !be_ismap(vm, -1))
    {
        be_pop(vm, be_top(vm) - base);
        return false;
    }
    be_pushstring(vm, key);
    const bool found = be_getindex(vm, -2) && !be_isnil(vm, -1);
    if (!found)
    {
        be_pop(vm, be_top(vm) - base);
        return false;
    }
    be_remove(vm, -2);
    be_remove(vm, -2);
    return true;
}

static esp_err_t reject_unknown_fields(bvm *vm, int map_index)
{
    const int base = be_top(vm);
    map_index = be_absindex(vm, map_index);
    if (!be_getmember(vm, map_index, ".p") || !be_ismap(vm, -1) || !be_pushiter(vm, -1))
    {
        be_pop(vm, be_top(vm) - base);
        return ESP_ERR_INVALID_ARG;
    }
    const int internal = be_absindex(vm, -2);
    while (be_iter_hasnext(vm, internal))
    {
        if (be_iter_next(vm, internal) != 2 || !be_isstring(vm, -2))
        {
            be_pop(vm, be_top(vm) - base);
            return ESP_ERR_INVALID_ARG;
        }
        const char *raw = be_tostring(vm, -2);
        char key[32];
        if (raw == nullptr || std::strlen(raw) >= sizeof(key))
        {
            be_pop(vm, be_top(vm) - base);
            return ESP_ERR_INVALID_ARG;
        }
        std::memcpy(key, raw, std::strlen(raw) + 1);
        be_pop(vm, 2);
        if (!known_fetch_field(key))
        {
            be_pop(vm, be_top(vm) - base);
            return ESP_ERR_INVALID_ARG;
        }
    }
    be_pop(vm, be_top(vm) - base);
    return ESP_OK;
}

static esp_err_t read_timeout(bvm *vm, int map_index, const char *key, bool *present, uint32_t *value)
{
    if (!push_field(vm, map_index, key))
    {
        *present = false;
        return ESP_OK;
    }
    *present = true;
    if (!be_isint(vm, -1))
    {
        be_pop(vm, 1);
        return ESP_ERR_INVALID_ARG;
    }
    const bint raw = be_toint(vm, -1);
    be_pop(vm, 1);
    if (!network_http_timeout_valid(static_cast<uint32_t>(raw)) || raw < 0)
    {
        return ESP_ERR_INVALID_ARG;
    }
    *value = static_cast<uint32_t>(raw);
    return ESP_OK;
}

static void push_string_list(bvm *vm, const char values[][16], uint8_t count)
{
    be_newobject(vm, "list");
    for (uint8_t index = 0; index < count; ++index)
    {
        be_pushstring(vm, values[index]);
        be_data_push(vm, -2);
        be_pop(vm, 1);
    }
    be_pop(vm, 1);
}

static void insert_top_list(bvm *vm, const char *key)
{
    be_pushstring(vm, key);
    be_pushvalue(vm, -2);
    be_data_insert(vm, -4);
    be_pop(vm, 2);
}

static int push_host_error(bvm *vm, esp_err_t err)
{
    be_newobject(vm, "map");
    map_set_bool(vm, "ok", false);
    map_set_string(vm, "error", esp_err_to_name(err));
    be_pop(vm, 1);
    be_return(vm);
}

static int native_net_host(bvm *vm)
{
    if (be_top(vm) != 0)
    {
        return push_host_error(vm, ESP_ERR_INVALID_ARG);
    }

    network_host_info_t info = {};
    const esp_err_t err = network_get_host_info(&info);
    if (err != ESP_OK)
    {
        return push_host_error(vm, err);
    }

    be_newobject(vm, "map");
    map_set_bool(vm, "ok", true);
    map_set_bool(vm, "connected", info.connected);
    map_set_string(vm, "ip", info.ip);
    map_set_string(vm, "netmask", info.netmask);
    map_set_string(vm, "gateway", info.gateway);
    push_string_list(vm, info.dns, info.dns_count);
    insert_top_list(vm, "dns");
    be_pop(vm, 1);
    map_set_string(vm, "mac", info.mac);
    map_set_string(vm, "error", "");
    be_pop(vm, 1);
    be_return(vm);
}

static int native_net_wifi(bvm *vm)
{
    if (be_top(vm) != 0)
    {
        return push_host_error(vm, ESP_ERR_INVALID_ARG);
    }

    network_wifi_info_t info = {};
    const esp_err_t err = network_get_wifi_info(&info);
    if (err != ESP_OK)
    {
        return push_host_error(vm, err);
    }

    be_newobject(vm, "map");
    map_set_bool(vm, "ok", true);
    map_set_string(vm, "mode", info.mode);
    map_set_bool(vm, "connected", info.connected);
    map_set_string(vm, "ssid", info.ssid);
    map_set_int(vm, "rssi", info.rssi);
    map_set_string(vm, "signal", info.signal);
    map_set_string(vm, "auth", info.auth);
    map_set_string(vm, "ip", info.ip);
    map_set_string(vm, "ap_ip", info.ap_ip);
    map_set_string(vm, "mac", info.mac);
    map_set_string(vm, "error", "");
    be_pop(vm, 1);
    be_return(vm);
}

static int native_net_wifi_connect(bvm *vm)
{
    const int argc = be_top(vm);
    if (argc < 1 || argc > 2 || !be_isstring(vm, 1) || (argc == 2 && !be_isstring(vm, 2)))
    {
        return push_status(vm, ESP_ERR_INVALID_ARG);
    }
    const char *password = argc == 2 ? be_tostring(vm, 2) : nullptr;
    return push_status(vm, network_wifi_connect(be_tostring(vm, 1), password));
}

static int native_net_wifi_disconnect(bvm *vm)
{
    if (be_top(vm) != 0)
    {
        return push_status(vm, ESP_ERR_INVALID_ARG);
    }
    return push_status(vm, network_wifi_disconnect());
}

static int native_net_set_base_url(bvm *vm)
{
    if (be_top(vm) != 1 || !be_isstring(vm, 1))
    {
        return push_status(vm, ESP_ERR_INVALID_ARG);
    }
    return push_status(vm, network_set_base_url(be_tostring(vm, 1)));
}

static int native_net_base_url(bvm *vm)
{
    if (be_top(vm) != 0)
    {
        be_pushstring(vm, "");
        be_return(vm);
    }
    char url[NETWORK_URL_MAX + 1];
    if (network_get_base_url(url, sizeof(url)) != ESP_OK)
    {
        url[0] = '\0';
    }
    be_pushstring(vm, url);
    be_return(vm);
}

static network_http_header_t s_headers[NETWORK_HTTP_HEADER_MAX];
static char s_body[NETWORK_HTTP_BODY_MAX + 1];

static esp_err_t read_headers(bvm *vm, int headers_index, size_t *count)
{
    const int base = be_top(vm);
    headers_index = be_absindex(vm, headers_index);
    if (!be_ismapinstance(vm, headers_index) || !be_getmember(vm, headers_index, ".p") || !be_ismap(vm, -1) ||
        !be_pushiter(vm, -1))
    {
        be_pop(vm, be_top(vm) - base);
        return ESP_ERR_INVALID_ARG;
    }
    const int internal = be_absindex(vm, -2);
    *count = 0;
    while (be_iter_hasnext(vm, internal))
    {
        if (be_iter_next(vm, internal) != 2 || !be_isstring(vm, -2) || !be_isstring(vm, -1))
        {
            be_pop(vm, be_top(vm) - base);
            return ESP_ERR_INVALID_ARG;
        }
        if (*count >= NETWORK_HTTP_HEADER_MAX)
        {
            be_pop(vm, be_top(vm) - base);
            return ESP_ERR_INVALID_SIZE;
        }
        const char *name = be_tostring(vm, -2);
        const char *value = be_tostring(vm, -1);
        if (!network_http_header_valid(name, value))
        {
            be_pop(vm, be_top(vm) - base);
            return ESP_ERR_INVALID_ARG;
        }
        std::memcpy(s_headers[*count].name, name, std::strlen(name) + 1);
        std::memcpy(s_headers[*count].value, value, std::strlen(value) + 1);
        ++(*count);
        be_pop(vm, 2);
    }
    be_pop(vm, be_top(vm) - base);
    return ESP_OK;
}

static void push_response_headers(bvm *vm, const network_http_response_t *response)
{
    be_newobject(vm, "map");
    if (response != nullptr)
    {
        for (size_t index = 0; index < response->header_count; ++index)
        {
            map_set_string(vm, response->headers[index].name, response->headers[index].value);
        }
    }
    be_pop(vm, 1);
}

static int push_fetch_result(bvm *vm, bool ok, int status, const char *url, const network_http_response_t *response,
                             const char *body, size_t body_len, bool json_ok, int json_index, const char *error)
{
    be_newobject(vm, "map");
    map_set_bool(vm, "ok", ok);
    map_set_int(vm, "status", status);
    map_set_string(vm, "url", url != nullptr ? url : "");
    push_response_headers(vm, response);
    insert_top_list(vm, "headers");
    be_pop(vm, 1);
    map_set_bytes(vm, "body", body, body_len);
    map_set_bool(vm, "json_ok", json_ok);
    be_pushstring(vm, "json");
    if (json_ok && json_index > 0)
    {
        be_pushvalue(vm, json_index);
    }
    else
    {
        be_pushnil(vm);
    }
    be_data_insert(vm, -3);
    be_pop(vm, 2);
    map_set_string(vm, "error", error != nullptr ? error : "");
    be_pop(vm, 1);
    if (json_index > 0)
    {
        const int source_index = json_index - 1;
        be_remove(vm, json_index);
        if (source_index > 0)
        {
            be_remove(vm, source_index);
        }
    }
    be_return(vm);
}

static int native_net_fetch(bvm *vm)
{
    const int argc = be_top(vm);
    if (argc < 1 || argc > 2 || !be_isstring(vm, 1) || (argc == 2 && !be_isnil(vm, 2) && !be_ismapinstance(vm, 2)))
    {
        return push_fetch_result(vm, false, 0, "", nullptr, "", 0, false, 0, esp_err_to_name(ESP_ERR_INVALID_ARG));
    }

    const char *url = be_tostring(vm, 1);
    network_http_request_t request = {
        .method = nullptr,
        .body = nullptr,
        .body_len = 0,
        .headers = nullptr,
        .header_count = 0,
        .timeout_ms = NETWORK_HTTP_TIMEOUT_DEFAULT_MS,
        .total_timeout_ms = NETWORK_HTTP_TIMEOUT_DEFAULT_MS,
    };
    bool want_json = false;
    if (argc == 2 && be_ismapinstance(vm, 2))
    {
        const esp_err_t fields = reject_unknown_fields(vm, 2);
        if (fields != ESP_OK)
        {
            return push_fetch_result(vm, false, 0, url, nullptr, "", 0, false, 0, esp_err_to_name(fields));
        }
        if (push_field(vm, 2, "method"))
        {
            if (!be_isstring(vm, -1) || !network_http_method_valid(be_tostring(vm, -1)))
            {
                be_pop(vm, 1);
                return push_fetch_result(vm, false, 0, url, nullptr, "", 0, false, 0,
                                         esp_err_to_name(ESP_ERR_INVALID_ARG));
            }
            static char method[8];
            const char *method_name = be_tostring(vm, -1);
            std::memcpy(method, method_name, std::strlen(method_name) + 1);
            be_pop(vm, 1);
            request.method = method;
        }
        if (push_field(vm, 2, "headers"))
        {
            if (!be_ismapinstance(vm, -1))
            {
                be_pop(vm, 1);
                return push_fetch_result(vm, false, 0, url, nullptr, "", 0, false, 0,
                                         esp_err_to_name(ESP_ERR_INVALID_ARG));
            }
            const int headers_index = be_absindex(vm, -1);
            size_t header_count = 0;
            const esp_err_t header_err = read_headers(vm, headers_index, &header_count);
            be_pop(vm, 1);
            if (header_err != ESP_OK)
            {
                return push_fetch_result(vm, false, 0, url, nullptr, "", 0, false, 0, esp_err_to_name(header_err));
            }
            request.headers = s_headers;
            request.header_count = header_count;
        }
        if (push_field(vm, 2, "body"))
        {
            if (!be_isstring(vm, -1))
            {
                be_pop(vm, 1);
                return push_fetch_result(vm, false, 0, url, nullptr, "", 0, false, 0,
                                         esp_err_to_name(ESP_ERR_INVALID_ARG));
            }
            const int length = be_strlen(vm, -1);
            if (length < 0 || static_cast<size_t>(length) > NETWORK_HTTP_BODY_MAX)
            {
                be_pop(vm, 1);
                return push_fetch_result(vm, false, 0, url, nullptr, "", 0, false, 0,
                                         esp_err_to_name(ESP_ERR_INVALID_SIZE));
            }
            const char *body = be_tostring(vm, -1);
            if (body != nullptr && length > 0)
            {
                std::memcpy(s_body, body, static_cast<size_t>(length));
            }
            s_body[length > 0 ? length : 0] = '\0';
            be_pop(vm, 1);
            request.body = s_body;
            request.body_len = static_cast<size_t>(length);
        }
        if (push_field(vm, 2, "response_type"))
        {
            if (!be_isstring(vm, -1))
            {
                be_pop(vm, 1);
                return push_fetch_result(vm, false, 0, url, nullptr, "", 0, false, 0,
                                         esp_err_to_name(ESP_ERR_INVALID_ARG));
            }
            const char *kind = be_tostring(vm, -1);
            if (std::strcmp(kind, "text") == 0)
            {
                want_json = false;
            }
            else if (std::strcmp(kind, "json") == 0)
            {
                want_json = true;
            }
            else
            {
                be_pop(vm, 1);
                return push_fetch_result(vm, false, 0, url, nullptr, "", 0, false, 0,
                                         esp_err_to_name(ESP_ERR_INVALID_ARG));
            }
            be_pop(vm, 1);
        }
        bool has_timeout = false;
        bool has_total = false;
        uint32_t timeout_ms = NETWORK_HTTP_TIMEOUT_DEFAULT_MS;
        uint32_t total_timeout_ms = NETWORK_HTTP_TIMEOUT_DEFAULT_MS;
        const esp_err_t timeout_err = read_timeout(vm, 2, "timeout_ms", &has_timeout, &timeout_ms);
        if (timeout_err != ESP_OK)
        {
            return push_fetch_result(vm, false, 0, url, nullptr, "", 0, false, 0, esp_err_to_name(timeout_err));
        }
        const esp_err_t total_err = read_timeout(vm, 2, "total_timeout_ms", &has_total, &total_timeout_ms);
        if (total_err != ESP_OK)
        {
            return push_fetch_result(vm, false, 0, url, nullptr, "", 0, false, 0, esp_err_to_name(total_err));
        }
        if (has_timeout && !has_total)
        {
            total_timeout_ms = timeout_ms;
        }
        else if (!has_timeout && has_total)
        {
            timeout_ms = total_timeout_ms;
        }
        else if (has_timeout && has_total && total_timeout_ms < timeout_ms)
        {
            return push_fetch_result(vm, false, 0, url, nullptr, "", 0, false, 0, esp_err_to_name(ESP_ERR_INVALID_ARG));
        }
        request.timeout_ms = timeout_ms;
        request.total_timeout_ms = total_timeout_ms;
    }

    const network_http_response_t *response = nullptr;
    const esp_err_t err = network_http_fetch(url, &request, &response);
    const char *reported_url = url != nullptr ? url : "";
    const char *body = "";
    size_t body_len = 0;
    int status = 0;
    if (response != nullptr && response->url[0] != '\0')
    {
        reported_url = response->url;
    }
    char url_copy[NETWORK_URL_MAX + 1];
    std::snprintf(url_copy, sizeof(url_copy), "%s", reported_url);
    if (err == ESP_OK && response != nullptr)
    {
        status = response->status;
        body = response->body != nullptr ? response->body : "";
        body_len = response->body_len;
    }

    char status_error[16];
    const char *error = "";
    bool http_ok = false;
    if (err != ESP_OK)
    {
        error = esp_err_to_name(err);
    }
    else if (status < 200 || status >= 300)
    {
        std::snprintf(status_error, sizeof(status_error), "HTTP_%d", status);
        error = status_error;
    }
    else
    {
        http_ok = true;
    }

    int json_index = 0;
    bool json_ok = false;
    if (want_json && err == ESP_OK)
    {
        const char *begin = body;
        const char *end = body + body_len;
        while (begin < end && json_space(*begin))
        {
            ++begin;
        }
        while (end > begin && json_space(end[-1]))
        {
            --end;
        }
        const size_t trimmed = static_cast<size_t>(end - begin);
        const bool json_null = trimmed == 4 && std::memcmp(begin, "null", 4) == 0;
        if (trimmed == 0)
        {
            if (http_ok)
            {
                http_ok = false;
                error = esp_err_to_name(ESP_ERR_INVALID_ARG);
            }
        }
        else
        {
            be_pushnstring(vm, begin, trimmed);
            const int source_index = be_absindex(vm, -1);
            const bool parsed = call_json_method(vm, "load", source_index) && berry_json_value(vm, -1) &&
                                (json_null || !be_isnil(vm, -1));
            if (!parsed)
            {
                while (be_top(vm) > source_index)
                {
                    be_pop(vm, 1);
                }
                be_remove(vm, source_index);
                if (http_ok)
                {
                    http_ok = false;
                    error = esp_err_to_name(ESP_ERR_INVALID_ARG);
                }
            }
            else
            {
                json_ok = true;
                json_index = be_absindex(vm, -1);
            }
        }
    }

    return push_fetch_result(vm, http_ok, status, url_copy, err == ESP_OK ? response : nullptr, body, body_len, json_ok,
                             json_index, error);
}

void berry_network_register(bvm *vm)
{
    be_pushntvfunction(vm, native_net_host);
    be_setmember(vm, -2, "host");
    be_pop(vm, 1);
    be_pushntvfunction(vm, native_net_wifi);
    be_setmember(vm, -2, "wifi");
    be_pop(vm, 1);
    be_pushntvfunction(vm, native_net_wifi_connect);
    be_setmember(vm, -2, "wifi_connect");
    be_pop(vm, 1);
    be_pushntvfunction(vm, native_net_wifi_disconnect);
    be_setmember(vm, -2, "wifi_disconnect");
    be_pop(vm, 1);
    be_pushntvfunction(vm, native_net_set_base_url);
    be_setmember(vm, -2, "set_base_url");
    be_pop(vm, 1);
    be_pushntvfunction(vm, native_net_base_url);
    be_setmember(vm, -2, "base_url");
    be_pop(vm, 1);
    be_pushntvfunction(vm, native_net_fetch);
    be_setmember(vm, -2, "fetch");
    be_pop(vm, 1);
}

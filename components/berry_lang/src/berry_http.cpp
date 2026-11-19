#include "berry.h"
#include "berry_json_internal.h"
#include "berry_runtime.h"

#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

void berry_runtime_lock();
void berry_runtime_unlock();
struct bvm *berry_runtime_get_vm();

namespace
{
constexpr char TAG[] = "berry_http";

constexpr int DEFAULT_TIMEOUT_MS = 5000;
constexpr int MIN_TIMEOUT_MS = 1;
constexpr int MAX_TIMEOUT_MS = 30000;
constexpr size_t MAX_URL_LEN = 512;
constexpr size_t MAX_HEADER_COUNT = 16;
constexpr size_t MAX_TOTAL_HEADER_BYTES = 4096;
constexpr size_t MAX_BODY_BYTES = 4096;
constexpr size_t MAX_ROUTES = 16;
constexpr uint16_t CORE_HTTP_PORT = 80;

struct Route
{
    uint32_t id = 0;
    std::string kind; // "GET","POST","PUT","DELETE","PATCH","HEAD","OPTIONS","ALL","USE"
    std::string path;
    int cb = -1;
};

struct HttpUserServer
{
    uint32_t magic = 0x48545450;
    httpd_handle_t hd = nullptr;
    bool listening = false;
    bool closing = false;
    std::string host = "0.0.0.0";
    uint16_t port = 0;

    std::mutex mu;
    std::vector<Route> routes;
    uint32_t next_route_id = 1;
    bool in_flight = false;

    // request/response exchange (single in-flight)
    bool has_exchange = false;
    bool exchange_done = false;
    bool exchange_timed_out = false;
    std::string req_method;
    std::string req_uri_path;
    std::string req_query_raw;
    std::string req_body;
    std::map<std::string, std::string> req_headers;
    std::map<std::string, std::string> req_params;
    std::map<std::string, std::string> req_query;
    std::vector<Route> matched_chain; // middleware/handlers to invoke in order
    std::string req_allow;            // allowed methods for 405 responses
    size_t chain_index = 0;
    bool handler_error = false;
    bool next_requested = false;

    int resp_status = 200;
    std::map<std::string, std::string> resp_headers;
    std::string resp_body;
    bool resp_sent = false;

    SemaphoreHandle_t done_sem = nullptr;
};

std::mutex g_servers_mu;
std::vector<HttpUserServer *> g_servers;
int g_next_cb = 1;
HttpUserServer *g_dispatch_server = nullptr;

void cb_global_name(int id, char *buf, size_t len)
{
    std::snprintf(buf, len, "_cos_http_cb_%d", id);
}

void store_callback(bvm *vm, int idx, int &id)
{
    id = g_next_cb++;
    char name[48];
    cb_global_name(id, name, sizeof(name));
    be_pushvalue(vm, idx);
    be_setglobal(vm, name);
    be_pop(vm, 1);
}

void remove_callback_vm(bvm *vm, int id)
{
    if (vm == nullptr || id < 0)
        return;
    char name[48];
    cb_global_name(id, name, sizeof(name));
    be_pushnil(vm);
    be_setglobal(vm, name);
    be_pop(vm, 1);
}

bool is_alive(HttpUserServer *s)
{
    if (s == nullptr)
        return false;
    std::lock_guard<std::mutex> lock(g_servers_mu);
    return std::find(g_servers.begin(), g_servers.end(), s) != g_servers.end();
}

HttpUserServer *get_server(bvm *vm, int idx)
{
    if (!be_iscomptr(vm, idx))
        return nullptr;
    auto *s = static_cast<HttpUserServer *>(be_tocomptr(vm, idx));
    if (!is_alive(s) || s->magic != 0x48545450)
        return nullptr;
    return s;
}

std::string to_lower(std::string s)
{
    for (char &c : s)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

bool parse_path_query(const char *uri, std::string &path, std::string &query)
{
    if (uri == nullptr)
        return false;
    const char *q = std::strchr(uri, '?');
    if (q == nullptr)
    {
        path = uri;
        query.clear();
    }
    else
    {
        path.assign(uri, static_cast<size_t>(q - uri));
        query.assign(q + 1);
    }
    if (path.empty())
        path = "/";
    return true;
}

int hexval(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return 10 + c - 'a';
    if (c >= 'A' && c <= 'F')
        return 10 + c - 'A';
    return -1;
}

std::string url_decode(const std::string &in, bool plus_space)
{
    std::string out;
    out.reserve(in.size());
    for (size_t i = 0; i < in.size(); ++i)
    {
        if (in[i] == '%' && i + 2 < in.size())
        {
            int h = hexval(in[i + 1]);
            int l = hexval(in[i + 2]);
            if (h >= 0 && l >= 0)
            {
                out.push_back(static_cast<char>((h << 4) | l));
                i += 2;
                continue;
            }
        }
        out.push_back((plus_space && in[i] == '+') ? ' ' : in[i]);
    }
    return out;
}

void parse_query_map(const std::string &q, std::map<std::string, std::string> &out)
{
    out.clear();
    size_t i = 0;
    while (i <= q.size())
    {
        size_t amp = q.find('&', i);
        if (amp == std::string::npos)
            amp = q.size();
        size_t eq = q.find('=', i);
        if (eq == std::string::npos || eq > amp)
            eq = amp;
        std::string k = q.substr(i, eq - i);
        std::string v = (eq < amp) ? q.substr(eq + 1, amp - eq - 1) : std::string();
        std::string dk = url_decode(k, true);
        std::string dv = url_decode(v, true);
        if (!dk.empty() && out.find(dk) == out.end())
            out[dk] = dv;
        if (amp == q.size())
            break;
        i = amp + 1;
    }
}

std::vector<std::string> split_segments(const std::string &p)
{
    std::vector<std::string> segs;
    size_t i = 0;
    while (i <= p.size())
    {
        size_t j = p.find('/', i);
        if (j == std::string::npos)
            j = p.size();
        if (j > i)
            segs.push_back(p.substr(i, j - i));
        if (j == p.size())
            break;
        i = j + 1;
    }
    return segs;
}

bool match_route_pattern(const std::string &pattern, const std::string &path,
                         std::map<std::string, std::string> &params)
{
    params.clear();
    auto a = split_segments(pattern);
    auto b = split_segments(path);
    if (a.size() != b.size())
        return false;
    for (size_t i = 0; i < a.size(); ++i)
    {
        if (!a[i].empty() && a[i][0] == ':')
        {
            std::string name = a[i].substr(1);
            if (name.empty())
                return false;
            params[name] = b[i];
        }
        else if (a[i] != b[i])
        {
            return false;
        }
    }
    return true;
}

const char *reason_phrase(int status)
{
    switch (status)
    {
    case 200:
        return "OK";
    case 201:
        return "Created";
    case 204:
        return "No Content";
    case 400:
        return "Bad Request";
    case 404:
        return "Not Found";
    case 405:
        return "Method Not Allowed";
    case 413:
        return "Payload Too Large";
    case 431:
        return "Request Header Fields Too Large";
    case 500:
        return "Internal Server Error";
    case 503:
        return "Service Unavailable";
    case 504:
        return "Gateway Timeout";
    default:
        return "Status";
    }
}

esp_err_t send_resp(httpd_req_t *req, int status, const std::map<std::string, std::string> &hdrs,
                    const std::string &body, bool head_only)
{
    char st[64];
    std::snprintf(st, sizeof(st), "%d %s", status, reason_phrase(status));
    httpd_resp_set_status(req, st);
    bool has_ct = false;
    for (const auto &kv : hdrs)
    {
        std::string lk = to_lower(kv.first);
        if (lk == "content-type")
            has_ct = true;
        httpd_resp_set_hdr(req, kv.first.c_str(), kv.second.c_str());
    }
    if (!has_ct)
        httpd_resp_set_type(req, "text/plain; charset=utf-8");
    if (head_only)
        return httpd_resp_send(req, nullptr, 0);
    return httpd_resp_send(req, body.data(), body.size());
}

bool read_body(httpd_req_t *req, std::string &body, bool &too_large, bool &recv_err)
{
    body.clear();
    too_large = false;
    recv_err = false;
    const size_t remain = req->content_len;
    if (remain > MAX_BODY_BYTES)
    {
        too_large = true;
        return false;
    }
    if (remain == 0)
        return true;
    body.resize(remain);
    size_t off = 0;
    while (off < remain)
    {
        const size_t to_read = remain - off;
        const int r = httpd_req_recv(req, &body[off], static_cast<int>(to_read));
        if (r <= 0)
        {
            if (r == HTTPD_SOCK_ERR_TIMEOUT)
            {
                continue;
            }
            recv_err = true;
            return false;
        }
        off += static_cast<size_t>(r);
    }
    return true;
}

void drain_body(httpd_req_t *req, size_t max_drain)
{
    size_t remain = req->content_len;
    if (remain == 0 || max_drain == 0)
        return;
    if (remain > max_drain)
        remain = max_drain;
    char buf[256];
    while (remain > 0)
    {
        const size_t chunk = remain < sizeof(buf) ? remain : sizeof(buf);
        const int r = httpd_req_recv(req, buf, static_cast<int>(chunk));
        if (r <= 0)
        {
            if (r == HTTPD_SOCK_ERR_TIMEOUT)
            {
                continue;
            }
            return;
        }
        remain -= static_cast<size_t>(r);
    }
}

void collect_req_headers(httpd_req_t *req, std::map<std::string, std::string> &out, bool &too_many, bool &too_large)
{
    out.clear();
    too_many = false;
    too_large = false;
    static const char *names[] = {
        "Host",   "User-Agent",   "Accept",  "Content-Type",     "Content-Length",  "Authorization",   "Connection",
        "Cookie", "Origin",       "Referer", "X-Requested-With", "Accept-Encoding", "Accept-Language", "Cache-Control",
        "Pragma", "If-None-Match"};
    size_t total = 0;
    for (const char *n : names)
    {
        size_t len = httpd_req_get_hdr_value_len(req, n);
        if (len == 0)
            continue;
        if (out.size() >= MAX_HEADER_COUNT)
        {
            too_many = true;
            return;
        }
        std::string val;
        val.resize(len + 1);
        if (httpd_req_get_hdr_value_str(req, n, &val[0], val.size()) != ESP_OK)
            continue;
        val.resize(std::strlen(val.c_str()));
        total += std::strlen(n) + val.size() + 4;
        if (total > MAX_TOTAL_HEADER_BYTES)
        {
            too_large = true;
            return;
        }
        out[to_lower(n)] = val;
    }
}

void push_string_map(bvm *vm, const std::map<std::string, std::string> &m)
{
    be_newobject(vm, "map");
    for (const auto &kv : m)
    {
        be_pushstring(vm, kv.first.c_str());
        be_pushstring(vm, kv.second.c_str());
        be_data_insert(vm, -3);
        be_pop(vm, 2);
    }
}

bool map_get_field(bvm *vm, int map_idx, const char *key)
{
    // pushes value on success, leaves stack unchanged on failure
    const int base = be_top(vm);
    map_idx = be_absindex(vm, map_idx);
    if (!be_ismapinstance(vm, map_idx) || !be_getmember(vm, map_idx, ".p") || !be_ismap(vm, -1))
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
    // stack: [internal_map, value] -> keep value
    be_remove(vm, -2);
    return true;
}

void stop_and_clear_server(HttpUserServer *s)
{
    if (s == nullptr)
        return;
    if (s->hd != nullptr)
    {
        httpd_stop(s->hd);
        s->hd = nullptr;
    }
    s->listening = false;
}

void destroy_server(HttpUserServer *s, bvm *vm)
{
    if (s == nullptr)
        return;
    stop_and_clear_server(s);
    if (vm != nullptr)
    {
        for (const auto &r : s->routes)
            remove_callback_vm(vm, r.cb);
    }
    if (s->done_sem != nullptr)
    {
        vSemaphoreDelete(s->done_sem);
        s->done_sem = nullptr;
    }
    s->magic = 0;
    delete s;
}

bool match_use_prefix(const std::string &pattern, const std::string &path)
{
    if (pattern == "/" || pattern.empty())
        return true;
    auto a = split_segments(pattern);
    auto b = split_segments(path);
    if (a.size() > b.size())
        return false;
    for (size_t i = 0; i < a.size(); ++i)
    {
        if (!a[i].empty() && a[i][0] == ':')
            continue;
        if (a[i] != b[i])
            return false;
    }
    return true;
}

std::string allow_header_for(const std::vector<std::string> &methods)
{
    static const char *order[] = {"GET", "HEAD", "POST", "PUT", "DELETE", "PATCH", "OPTIONS"};
    std::string out;
    for (const char *m : order)
    {
        if (std::find(methods.begin(), methods.end(), std::string(m)) != methods.end())
        {
            if (!out.empty())
                out += ", ";
            out += m;
        }
    }
    return out;
}

// Build chain for request under server mutex assumptions (routes snapshot used).
// Returns: 0 = no path match, 1 = handler chain built, 2 = path matched by endpoint but method not allowed.
int build_chain(HttpUserServer *s, const std::string &method, const std::string &path)
{
    s->matched_chain.clear();
    s->req_params.clear();
    s->req_allow.clear();

    bool endpoint_path_matched = false;
    bool handler_added = false;
    std::map<std::string, std::string> endpoint_params;
    std::vector<std::string> allowed;

    for (const auto &r : s->routes)
    {
        std::map<std::string, std::string> p;
        if (r.kind == "USE")
            continue;
        if (!match_route_pattern(r.path, path, p))
            continue;
        endpoint_path_matched = true;
        if (r.kind == "ALL")
        {
            static const char *all_methods[] = {"GET", "POST", "PUT", "DELETE", "PATCH", "HEAD", "OPTIONS"};
            for (const char *m : all_methods)
            {
                if (std::find(allowed.begin(), allowed.end(), std::string(m)) == allowed.end())
                    allowed.push_back(m);
            }
        }
        else if (std::find(allowed.begin(), allowed.end(), r.kind) == allowed.end())
        {
            allowed.push_back(r.kind);
        }

        if (!handler_added && (r.kind == "ALL" || r.kind == method))
        {
            endpoint_params = p;
            handler_added = true;
        }
    }

    // collect applicable USE middleware in registration order (prefix match)
    for (const auto &r : s->routes)
    {
        if (r.kind != "USE")
            continue;
        if (!match_use_prefix(r.path, path))
            continue;
        std::map<std::string, std::string> p;
        if (match_route_pattern(r.path, path, p) && s->req_params.empty())
            s->req_params = p;
        s->matched_chain.push_back(r);
    }

    // append first matching endpoint handler after middleware
    if (handler_added)
    {
        for (const auto &r : s->routes)
        {
            if (r.kind == "USE")
                continue;
            std::map<std::string, std::string> p;
            if (!match_route_pattern(r.path, path, p))
                continue;
            if (r.kind == "ALL" || r.kind == method)
            {
                s->matched_chain.push_back(r);
                endpoint_params = p;
                break;
            }
        }
    }

    if (!endpoint_params.empty())
        s->req_params = endpoint_params;
    if (handler_added)
        return 1;
    if (endpoint_path_matched)
    {
        s->req_allow = allow_header_for(allowed);
        return 2;
    }
    // USE-only prefix match without endpoint => 404 (no route)
    s->matched_chain.clear();
    s->req_params.clear();
    return 0;
}

// Execute matched chain in Berry under runtime lock. Returns true if a response was produced.
bool execute_chain_berry(HttpUserServer *s)
{
    bvm *vm = berry_runtime_get_vm();
    if (vm == nullptr)
        return false;

    s->resp_status = 200;
    s->resp_headers.clear();
    s->resp_body.clear();
    s->resp_sent = false;
    s->handler_error = false;

    // Create req map
    push_string_map(vm, s->req_headers);
    int req_headers_idx = be_top(vm); // map on stack
    push_string_map(vm, s->req_params);
    int req_params_idx = be_top(vm);
    push_string_map(vm, s->req_query);
    int req_query_idx = be_top(vm);

    be_newobject(vm, "map");
    int req_idx = be_top(vm);
    auto set_str = [&](const char *k, const std::string &v)
    {
        be_pushstring(vm, k);
        be_pushstring(vm, v.c_str());
        be_data_insert(vm, req_idx);
        be_pop(vm, 2);
    };
    set_str("method", s->req_method);
    set_str("path", s->req_uri_path);
    set_str("url", s->req_uri_path + (s->req_query_raw.empty() ? "" : ("?" + s->req_query_raw)));
    set_str("body", s->req_body);
    set_str("text", s->req_body);
    be_pushstring(vm, "headers");
    be_pushvalue(vm, req_headers_idx);
    be_data_insert(vm, req_idx);
    be_pop(vm, 2);
    be_pushstring(vm, "params");
    be_pushvalue(vm, req_params_idx);
    be_data_insert(vm, req_idx);
    be_pop(vm, 2);
    be_pushstring(vm, "query");
    be_pushvalue(vm, req_query_idx);
    be_data_insert(vm, req_idx);
    be_pop(vm, 2);

    // req.json: best-effort parse of body when present
    be_pushstring(vm, "json");
    bool json_pushed = false;
    if (!s->req_body.empty())
    {
        const char *begin = s->req_body.data();
        const char *end = begin + s->req_body.size();
        while (begin < end && berry_json_whitespace(*begin))
            ++begin;
        while (end > begin && berry_json_whitespace(*(end - 1)))
            --end;
        size_t trimmed = static_cast<size_t>(end - begin);
        bool is_null = (trimmed == 4 && std::memcmp(begin, "null", 4) == 0);
        if (trimmed > 0)
        {
            be_pushnstring(vm, begin, trimmed);
            int src_idx = be_top(vm);
            if (berry_json_call_method(vm, "load", src_idx) && (is_null || !be_isnil(vm, -1)))
            {
                // parsed value on top; remove source string below it
                be_remove(vm, src_idx);
                json_pushed = true;
            }
            else
            {
                // cleanup stack to remove parse result/source
                while (be_top(vm) > req_idx)
                {
                    be_pop(vm, 1);
                }
                be_pushnil(vm);
                json_pushed = true;
            }
        }
    }
    if (!json_pushed)
    {
        be_pushnil(vm);
    }
    be_data_insert(vm, req_idx);
    be_pop(vm, 2);

    // Create res map with control fields
    be_newobject(vm, "map");
    int res_idx = be_top(vm);
    be_pushstring(vm, "_status");
    be_pushint(vm, 200);
    be_data_insert(vm, res_idx);
    be_pop(vm, 2);
    be_pushstring(vm, "_sent");
    be_pushbool(vm, 0);
    be_data_insert(vm, res_idx);
    be_pop(vm, 2);
    be_pushstring(vm, "_body");
    be_pushstring(vm, "");
    be_data_insert(vm, res_idx);
    be_pop(vm, 2);
    be_pushstring(vm, "_headers");
    be_newobject(vm, "map");
    be_data_insert(vm, res_idx);
    be_pop(vm, 2);

    bool handled = false;
    g_dispatch_server = s;
    for (const auto &r : s->matched_chain)
    {
        if (r.cb < 0)
            continue;
        s->next_requested = false;
        char name[48];
        cb_global_name(r.cb, name, sizeof(name));
        be_getglobal(vm, name);
        if (!be_isfunction(vm, -1))
        {
            be_pop(vm, 1);
            continue;
        }
        // call handler(req, res, next)
        be_pushvalue(vm, req_idx);
        be_pushvalue(vm, res_idx);
        be_getglobal(vm, "http");
        if (be_ismodule(vm, -1) || be_isinstance(vm, -1))
            be_getmethod(vm, -1, "next");
        else
        {
            be_pop(vm, 1);
            be_pushnil(vm);
        }
        if (be_top(vm) > 0 && (be_ismodule(vm, -2) || be_isinstance(vm, -2)))
            be_remove(vm, -2); // remove module/instance, keep next function or nil
        if (be_pcall(vm, 3) != 0)
        {
            const char *err = be_tostring(vm, -1);
            ESP_LOGE(TAG, "handler error: %s", err ? err : "(unknown)");
            be_pop(vm, 1); // error
            s->handler_error = true;
            handled = true;
            break;
        }
        be_pop(vm, 1); // result
        bool sent = false;
        if (map_get_field(vm, res_idx, "_sent"))
        {
            if (be_isbool(vm, -1))
                sent = be_tobool(vm, -1);
            be_pop(vm, 1);
        }
        if (sent || s->resp_sent)
        {
            handled = true;
            break;
        }
        if (r.kind == "USE")
        {
            // middleware continues unless it errored/sent; next() is supported but not required for v1
            continue;
        }
        // endpoint handler returned without sending
        handled = true;
        break;
    }
    g_dispatch_server = nullptr;

    // Extract response from res map (instance-safe access)
    if (map_get_field(vm, res_idx, "_status"))
    {
        if (be_isint(vm, -1))
            s->resp_status = static_cast<int>(be_toint(vm, -1));
        be_pop(vm, 1);
    }
    if (s->resp_status < 100 || s->resp_status > 599)
        s->resp_status = 500;

    if (map_get_field(vm, res_idx, "_sent"))
    {
        if (be_isbool(vm, -1))
            s->resp_sent = be_tobool(vm, -1);
        be_pop(vm, 1);
    }

    if (map_get_field(vm, res_idx, "_body"))
    {
        if (be_isstring(vm, -1))
        {
            const char *b = be_tostring(vm, -1);
            s->resp_body = b ? b : "";
        }
        be_pop(vm, 1);
    }

    if (map_get_field(vm, res_idx, "_headers"))
    {
        if (be_ismapinstance(vm, -1))
        {
            int hdr_map_idx = be_top(vm);
            if (be_getmember(vm, hdr_map_idx, ".p") && be_ismap(vm, -1) && be_pushiter(vm, -1))
            {
                const int internal = be_absindex(vm, -2);
                while (be_iter_hasnext(vm, internal))
                {
                    if (be_iter_next(vm, internal) == 2 && be_isstring(vm, -2) && be_isstring(vm, -1))
                    {
                        const char *k = be_tostring(vm, -2);
                        const char *v = be_tostring(vm, -1);
                        if (k && v)
                            s->resp_headers[k] = v;
                    }
                    be_pop(vm, 2);
                }
                be_pop(vm, 2); // iter, internal map
            }
            else
            {
                // cleanup partial stack from failed access attempts
                while (be_top(vm) > hdr_map_idx)
                    be_pop(vm, 1);
            }
        }
        be_pop(vm, 1); // headers value
    }

    // cleanup stack (res, req, req_query, req_params, req_headers)
    be_pop(vm, 5);

    if (s->handler_error)
    {
        s->resp_status = 500;
        s->resp_headers.clear();
        s->resp_headers["Content-Type"] = "application/json";
        s->resp_body = "{\"error\":\"Internal Server Error\"}";
        s->resp_sent = true;
        return true;
    }
    if (!handled)
        return false;
    if (!s->resp_sent)
    {
        // handler matched but did not send -> 500 per safe behavior
        s->resp_status = 500;
        s->resp_headers.clear();
        s->resp_headers["Content-Type"] = "application/json";
        s->resp_body = "{\"error\":\"No response\"}";
        s->resp_sent = true;
    }
    return true;
}

esp_err_t generic_handler(httpd_req_t *req)
{
    auto *s = static_cast<HttpUserServer *>(req->user_ctx);
    if (s == nullptr || !is_alive(s))
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Server unavailable");

    if (std::strlen(req->uri) > MAX_URL_LEN)
    {
        return send_resp(req, 414, {{"Content-Type", "application/json"}}, "{\"error\":\"URI too long\"}",
                         req->method == HTTP_HEAD);
    }

    std::string method;
    switch (req->method)
    {
    case HTTP_GET:
        method = "GET";
        break;
    case HTTP_POST:
        method = "POST";
        break;
    case HTTP_PUT:
        method = "PUT";
        break;
    case HTTP_DELETE:
        method = "DELETE";
        break;
    case HTTP_PATCH:
        method = "PATCH";
        break;
    case HTTP_HEAD:
        method = "HEAD";
        break;
    case HTTP_OPTIONS:
        method = "OPTIONS";
        break;
    default:
        method = "GET";
        break;
    }

    std::string path, query_raw;
    if (!parse_path_query(req->uri, path, query_raw))
    {
        return send_resp(req, 400, {{"Content-Type", "application/json"}}, "{\"error\":\"Bad Request\"}",
                         method == "HEAD");
    }

    bool too_many_hdr = false, too_large_hdr = false;
    std::map<std::string, std::string> headers;
    collect_req_headers(req, headers, too_many_hdr, too_large_hdr);
    if (too_many_hdr)
    {
        return send_resp(req, 431, {{"Content-Type", "application/json"}},
                         "{\"error\":\"Request Header Fields Too Large\"}", method == "HEAD");
    }
    if (too_large_hdr)
    {
        return send_resp(req, 431, {{"Content-Type", "application/json"}},
                         "{\"error\":\"Request Header Fields Too Large\"}", method == "HEAD");
    }

    bool too_large_body = false, recv_err = false;
    std::string body;
    // read body for methods that may carry it; drain oversized bodies best-effort to keep connection usable
    if (method == "POST" || method == "PUT" || method == "PATCH" || method == "DELETE")
    {
        if (!read_body(req, body, too_large_body, recv_err))
        {
            if (too_large_body)
            {
                drain_body(req, MAX_BODY_BYTES + 512);
                return send_resp(req, 413, {{"Content-Type", "application/json"}}, "{\"error\":\"Payload Too Large\"}",
                                 method == "HEAD");
            }
            return send_resp(req, 400, {{"Content-Type", "application/json"}}, "{\"error\":\"Bad Request\"}",
                             method == "HEAD");
        }
    }
    else if (req->content_len > 0)
    {
        if (req->content_len > MAX_BODY_BYTES)
        {
            drain_body(req, MAX_BODY_BYTES + 512);
            return send_resp(req, 413, {{"Content-Type", "application/json"}}, "{\"error\":\"Payload Too Large\"}",
                             method == "HEAD");
        }
        std::string tmp;
        bool tl = false, re = false;
        if (!read_body(req, tmp, tl, re))
        {
            if (tl)
            {
                drain_body(req, MAX_BODY_BYTES + 512);
                return send_resp(req, 413, {{"Content-Type", "application/json"}}, "{\"error\":\"Payload Too Large\"}",
                                 method == "HEAD");
            }
            return send_resp(req, 400, {{"Content-Type", "application/json"}}, "{\"error\":\"Bad Request\"}",
                             method == "HEAD");
        }
        body = tmp;
    }

    // single request at a time gate + prepare exchange
    int chain_state = 0;
    {
        std::lock_guard<std::mutex> lock(s->mu);
        if (s->closing || !s->listening)
        {
            return send_resp(req, 503, {{"Content-Type", "application/json"}}, "{\"error\":\"Service Unavailable\"}",
                             method == "HEAD");
        }
        if (s->in_flight)
        {
            return send_resp(req, 503, {{"Content-Type", "application/json"}}, "{\"error\":\"Busy\"}",
                             method == "HEAD");
        }
        s->in_flight = true;
        s->req_method = method;
        s->req_uri_path = path;
        s->req_query_raw = query_raw;
        s->req_body = body;
        s->req_headers = headers;
        parse_query_map(query_raw, s->req_query);
        chain_state = build_chain(s, method, path);
    }

    if (chain_state == 0)
    {
        std::lock_guard<std::mutex> lock(s->mu);
        s->in_flight = false;
        s->matched_chain.clear();
        return send_resp(req, 404, {{"Content-Type", "application/json"}}, "{\"error\":\"Not Found\"}",
                         method == "HEAD");
    }
    if (chain_state == 2)
    {
        std::string allow;
        {
            std::lock_guard<std::mutex> lock(s->mu);
            allow = s->req_allow;
            s->in_flight = false;
            s->matched_chain.clear();
            s->req_allow.clear();
        }
        std::map<std::string, std::string> hdrs = {{"Content-Type", "application/json"}};
        if (!allow.empty())
            hdrs["Allow"] = allow;
        return send_resp(req, 405, hdrs, "{\"error\":\"Method Not Allowed\"}", method == "HEAD");
    }

    // Execute Berry handlers synchronously under runtime lock on this (httpd) thread.
    // NOTE: cooperative timeout only - if handler never returns, VM remains locked (documented limitation).
    berry_runtime_lock();
    bool ok = false;
    if (is_alive(s))
    {
        ok = execute_chain_berry(s);
    }
    int status = s->resp_status;
    auto rh = s->resp_headers;
    std::string rb = s->resp_body;
    bool sent = s->resp_sent;
    berry_runtime_unlock();

    {
        std::lock_guard<std::mutex> lock(s->mu);
        s->in_flight = false;
        s->matched_chain.clear();
    }

    if (!ok || !sent)
    {
        return send_resp(req, 500, {{"Content-Type", "application/json"}}, "{\"error\":\"Internal Server Error\"}",
                         method == "HEAD");
    }

    return send_resp(req, status, rh, rb, method == "HEAD");
}

bool register_handlers(HttpUserServer *s)
{
    // Register catch-all for supported methods
    const http_method methods[] = {HTTP_GET, HTTP_POST, HTTP_PUT, HTTP_DELETE, HTTP_PATCH, HTTP_HEAD, HTTP_OPTIONS};
    for (http_method m : methods)
    {
        httpd_uri_t uri = {};
        uri.uri = "/*";
        uri.method = m;
        uri.handler = generic_handler;
        uri.user_ctx = s;
        esp_err_t err = httpd_register_uri_handler(s->hd, &uri);
        if (err != ESP_OK && err != ESP_ERR_HTTPD_HANDLER_EXISTS)
        {
            ESP_LOGE(TAG, "register handler failed: %s", esp_err_to_name(err));
            return false;
        }
    }
    return true;
}

// ---- Berry native API ----

int http_create_server(bvm *vm)
{
    auto *s = new (std::nothrow) HttpUserServer();
    if (s == nullptr)
    {
        be_raise(vm, "runtime_error", "out of memory");
    }
    s->done_sem = xSemaphoreCreateBinary();
    if (s->done_sem == nullptr)
    {
        delete s;
        be_raise(vm, "runtime_error", "out of memory");
    }
    {
        std::lock_guard<std::mutex> lock(g_servers_mu);
        g_servers.push_back(s);
    }
    be_pushcomptr(vm, s);
    be_return(vm);
}

int http_listen(bvm *vm)
{
    int argc = be_top(vm);
    if (argc < 2)
        be_raise(vm, "value_error", "listen(server, port[, host])");
    HttpUserServer *s = get_server(vm, 1);
    if (s == nullptr)
        be_raise(vm, "value_error", "invalid server");
    if (!be_isint(vm, 2))
        be_raise(vm, "value_error", "port must be integer");
    int port = static_cast<int>(be_toint(vm, 2));
    std::string host = "0.0.0.0";
    if (argc >= 3 && !be_isnil(vm, 3))
    {
        if (!be_isstring(vm, 3))
            be_raise(vm, "value_error", "host must be string");
        const char *h = be_tostring(vm, 3);
        host = h ? h : "0.0.0.0";
    }
    if (port <= 0 || port > 65535)
        be_raise(vm, "value_error", "invalid port");
    if (static_cast<uint16_t>(port) == CORE_HTTP_PORT)
        be_raise(vm, "value_error", "port conflicts with core server");
    if (host != "0.0.0.0" && host != "127.0.0.1")
    {
        // allow explicit host strings; validation minimal
    }

    std::lock_guard<std::mutex> lock(s->mu);
    if (s->listening || s->hd != nullptr)
        be_raise(vm, "runtime_error", "already listening");

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = static_cast<uint16_t>(port);
    config.ctrl_port = static_cast<uint16_t>(port + 10000); // avoid clash with core control port
    config.max_uri_handlers = 8;
    config.stack_size = 6144;
    config.max_open_sockets = 4;
    config.lru_purge_enable = true;
    config.uri_match_fn = httpd_uri_match_wildcard;

    httpd_handle_t hd = nullptr;
    esp_err_t err = httpd_start(&hd, &config);
    if (err != ESP_OK)
    {
        char msg[96];
        std::snprintf(msg, sizeof(msg), "listen failed: %s", esp_err_to_name(err));
        be_raise(vm, "runtime_error", msg);
    }
    s->hd = hd;
    s->port = static_cast<uint16_t>(port);
    s->host = host;
    s->listening = true;
    s->closing = false;
    if (!register_handlers(s))
    {
        httpd_stop(hd);
        s->hd = nullptr;
        s->listening = false;
        be_raise(vm, "runtime_error", "failed to register handlers");
    }

    be_pushbool(vm, 1);
    be_return(vm);
}

int http_close(bvm *vm)
{
    if (be_top(vm) < 1)
        be_raise(vm, "value_error", "close(server)");
    HttpUserServer *s = get_server(vm, 1);
    if (s == nullptr)
    {
        be_pushbool(vm, 1); // idempotent on invalid/already-destroyed
        be_return(vm);
    }
    {
        std::lock_guard<std::mutex> lock(s->mu);
        if (s->in_flight)
            be_raise(vm, "runtime_error", "cannot close during request dispatch");
        s->closing = true;
        s->listening = false;
    }
    stop_and_clear_server(s);
    berry_runtime_lock();
    bvm *rvm = berry_runtime_get_vm();
    {
        std::lock_guard<std::mutex> lock(g_servers_mu);
        auto it = std::find(g_servers.begin(), g_servers.end(), s);
        if (it != g_servers.end())
            g_servers.erase(it);
    }
    if (rvm != nullptr)
    {
        for (const auto &r : s->routes)
            remove_callback_vm(rvm, r.cb);
    }
    if (s->done_sem)
        vSemaphoreDelete(s->done_sem);
    s->magic = 0;
    delete s;
    berry_runtime_unlock();
    be_pushbool(vm, 1);
    be_return(vm);
}

bool valid_kind(const char *k)
{
    return std::strcmp(k, "GET") == 0 || std::strcmp(k, "POST") == 0 || std::strcmp(k, "PUT") == 0 ||
           std::strcmp(k, "DELETE") == 0 || std::strcmp(k, "PATCH") == 0 || std::strcmp(k, "HEAD") == 0 ||
           std::strcmp(k, "OPTIONS") == 0 || std::strcmp(k, "ALL") == 0 || std::strcmp(k, "USE") == 0;
}

int http_add_route(bvm *vm, const char *kind)
{
    int argc = be_top(vm);
    if (argc < 3)
        be_raise(vm, "value_error", "route(server, path, handler)");
    HttpUserServer *s = get_server(vm, 1);
    if (s == nullptr)
        be_raise(vm, "value_error", "invalid server");
    if (!be_isstring(vm, 2) || !be_isfunction(vm, 3))
        be_raise(vm, "value_error", "path must be string and handler must be function");
    const char *path = be_tostring(vm, 2);
    if (path == nullptr || path[0] != '/')
        be_raise(vm, "value_error", "path must start with '/'");

    std::lock_guard<std::mutex> lock(s->mu);
    if (s->in_flight)
        be_raise(vm, "runtime_error", "cannot modify routes during request dispatch");
    // count non-removed routes (all in vector are active)
    if (s->routes.size() >= MAX_ROUTES)
        be_raise(vm, "runtime_error", "route limit exceeded");

    int cb = -1;
    store_callback(vm, 3, cb);
    Route r;
    r.id = s->next_route_id++;
    r.kind = kind;
    r.path = path;
    r.cb = cb;
    s->routes.push_back(r);
    be_pushint(vm, static_cast<bint>(r.id));
    be_return(vm);
}

int http_get(bvm *vm)
{
    return http_add_route(vm, "GET");
}
int http_post(bvm *vm)
{
    return http_add_route(vm, "POST");
}
int http_put(bvm *vm)
{
    return http_add_route(vm, "PUT");
}
int http_delete(bvm *vm)
{
    return http_add_route(vm, "DELETE");
}
int http_patch(bvm *vm)
{
    return http_add_route(vm, "PATCH");
}
int http_head(bvm *vm)
{
    return http_add_route(vm, "HEAD");
}
int http_options(bvm *vm)
{
    return http_add_route(vm, "OPTIONS");
}
int http_all(bvm *vm)
{
    return http_add_route(vm, "ALL");
}
int http_use(bvm *vm)
{
    return http_add_route(vm, "USE");
}

int http_remove(bvm *vm)
{
    if (be_top(vm) < 2)
        be_raise(vm, "value_error", "remove(server, route_id)");
    HttpUserServer *s = get_server(vm, 1);
    if (s == nullptr)
        be_raise(vm, "value_error", "invalid server");
    if (!be_isint(vm, 2))
        be_raise(vm, "value_error", "route_id must be integer");
    uint32_t rid = static_cast<uint32_t>(be_toint(vm, 2));
    std::lock_guard<std::mutex> lock(s->mu);
    if (s->in_flight)
        be_raise(vm, "runtime_error", "cannot modify routes during request dispatch");
    for (auto it = s->routes.begin(); it != s->routes.end(); ++it)
    {
        if (it->id == rid)
        {
            int cb = it->cb;
            s->routes.erase(it);
            remove_callback_vm(vm, cb);
            be_pushbool(vm, 1);
            be_return(vm);
        }
    }
    be_pushbool(vm, 0);
    be_return(vm);
}

// Response helpers called from Berry handlers: res_status(res, code), res_set(res,k,v), res_send(res,body),
// res_json(res, value), res_end(res[,body])
int http_res_status(bvm *vm)
{
    if (be_top(vm) < 2 || !be_ismapinstance(vm, 1) || !be_isint(vm, 2))
        be_raise(vm, "value_error", "res_status(res, code)");
    int code = static_cast<int>(be_toint(vm, 2));
    if (code < 100 || code > 599)
        be_raise(vm, "value_error", "invalid status");
    be_pushstring(vm, "_status");
    be_pushint(vm, code);
    be_data_insert(vm, 1);
    be_pop(vm, 2);
    be_pushvalue(vm, 1);
    be_return(vm);
}

bool res_is_sent(bvm *vm, int res_idx)
{
    bool sent = false;
    if (map_get_field(vm, res_idx, "_sent"))
    {
        if (be_isbool(vm, -1))
            sent = be_tobool(vm, -1);
        be_pop(vm, 1);
    }
    return sent;
}

int http_res_set(bvm *vm)
{
    if (be_top(vm) < 3 || !be_ismapinstance(vm, 1) || !be_isstring(vm, 2) || !be_isstring(vm, 3))
        be_raise(vm, "value_error", "res_set(res, name, value)");
    if (!map_get_field(vm, 1, "_headers") || !be_ismapinstance(vm, -1))
    {
        // map_get_field either pushed nothing or pushed a non-instance value
        if (be_top(vm) > 3)
            be_pop(vm, 1);
        be_raise(vm, "runtime_error", "invalid response");
    }
    int hdr_idx = be_top(vm);
    be_pushvalue(vm, 2);
    be_pushvalue(vm, 3);
    be_data_insert(vm, hdr_idx);
    be_pop(vm, 3); // key, value, headers map
    be_pushvalue(vm, 1);
    be_return(vm);
}

void mark_sent(bvm *vm, int res_idx)
{
    be_pushstring(vm, "_sent");
    be_pushbool(vm, 1);
    be_data_insert(vm, res_idx);
    be_pop(vm, 2);
}

int http_res_send(bvm *vm)
{
    if (be_top(vm) < 2 || !be_ismapinstance(vm, 1))
        be_raise(vm, "value_error", "res_send(res, body)");
    if (res_is_sent(vm, 1))
    {
        be_pushbool(vm, 0);
        be_return(vm);
    }
    const char *body = nullptr;
    if (be_isstring(vm, 2))
    {
        body = be_tostring(vm, 2);
    }
    else if (be_isnil(vm, 2))
    {
        body = "";
    }
    else
    {
        be_raise(vm, "value_error", "body must be string");
    }
    be_pushstring(vm, "_body");
    be_pushstring(vm, body ? body : "");
    be_data_insert(vm, 1);
    be_pop(vm, 2);
    mark_sent(vm, 1);
    be_pushbool(vm, 1);
    be_return(vm);
}

int http_res_json(bvm *vm)
{
    if (be_top(vm) < 2 || !be_ismapinstance(vm, 1))
        be_raise(vm, "value_error", "res_json(res, value)");
    if (res_is_sent(vm, 1))
    {
        be_pushbool(vm, 0);
        be_return(vm);
    }
    // use Berry json.dump if available
    if (!berry_json_call_method(vm, "dump", 2) || !be_isstring(vm, -1))
    {
        be_raise(vm, "value_error", "failed to encode json");
    }
    int json_str_idx = be_top(vm);
    be_pushstring(vm, "_body");
    be_pushvalue(vm, json_str_idx);
    be_data_insert(vm, 1);
    be_pop(vm, 2);
    be_pop(vm, 1); // json string
    // set content-type
    if (map_get_field(vm, 1, "_headers") && be_ismapinstance(vm, -1))
    {
        int h = be_top(vm);
        be_pushstring(vm, "Content-Type");
        be_pushstring(vm, "application/json");
        be_data_insert(vm, h);
        be_pop(vm, 3); // key, value, headers map
    }
    else if (be_top(vm) > 2)
    {
        be_pop(vm, 1);
    }
    mark_sent(vm, 1);
    be_pushbool(vm, 1);
    be_return(vm);
}

int http_res_end(bvm *vm)
{
    int argc = be_top(vm);
    if (argc < 1 || !be_ismapinstance(vm, 1))
        be_raise(vm, "value_error", "res_end(res[, body])");
    if (argc >= 2 && !be_isnil(vm, 2))
        return http_res_send(vm);
    if (res_is_sent(vm, 1))
    {
        be_pushbool(vm, 0);
        be_return(vm);
    }
    be_pushstring(vm, "_body");
    be_pushstring(vm, "");
    be_data_insert(vm, 1);
    be_pop(vm, 2);
    mark_sent(vm, 1);
    be_pushbool(vm, 1);
    be_return(vm);
}

int http_next(bvm *vm)
{
    (void)vm;
    if (g_dispatch_server != nullptr)
    {
        g_dispatch_server->next_requested = true;
    }
    be_pushbool(vm, 1);
    be_return(vm);
}

} // namespace

extern "C" void berry_http_runtime_teardown(void)
{
    // Best-effort teardown. If a request is in-flight, avoid UAF/deadlock by leaving server state
    // allocated/unregistered from new dispatch (cannot safely preempt handler/stop httpd worker).
    bvm *vm = berry_runtime_get_vm();
    std::vector<HttpUserServer *> servers;
    {
        std::lock_guard<std::mutex> lock(g_servers_mu);
        servers = g_servers;
        g_servers.clear();
    }
    for (auto *s : servers)
    {
        bool inflight = false;
        {
            std::lock_guard<std::mutex> lock(s->mu);
            inflight = s->in_flight;
            s->closing = true;
            s->listening = false;
        }
        if (inflight)
        {
            // shortcut: skip destructive cleanup while handler may still reference state; may leak until reset.
            continue;
        }
        stop_and_clear_server(s);
        if (vm != nullptr)
        {
            for (const auto &r : s->routes)
                remove_callback_vm(vm, r.cb);
        }
        if (s->done_sem)
            vSemaphoreDelete(s->done_sem);
        s->magic = 0;
        delete s;
    }
}

// Module definition
// NOLINTNEXTLINE(readability-identifier-naming)
static bntvmodobj_t const http_attrs[] = {
    be_native_module_function("create_server", http_create_server),
    be_native_module_function("listen", http_listen),
    be_native_module_function("close", http_close),
    be_native_module_function("get", http_get),
    be_native_module_function("post", http_post),
    be_native_module_function("put", http_put),
    be_native_module_function("delete", http_delete),
    be_native_module_function("del", http_delete),
    be_native_module_function("patch", http_patch),
    be_native_module_function("head", http_head),
    be_native_module_function("options", http_options),
    be_native_module_function("all", http_all),
    be_native_module_function("any", http_all),
    be_native_module_function("use", http_use),
    be_native_module_function("remove", http_remove),
    // response helpers (used by handlers)
    be_native_module_function("res_status", http_res_status),
    be_native_module_function("res_set", http_res_set),
    be_native_module_function("res_header", http_res_set),
    be_native_module_function("res_send", http_res_send),
    be_native_module_function("res_json", http_res_json),
    be_native_module_function("res_end", http_res_end),
    be_native_module_function("next", http_next),
};

// NOLINTNEXTLINE(readability-identifier-naming)
extern "C" const bntvmodule_t be_native_module_http = {
    "http",
    http_attrs,
    sizeof(http_attrs) / sizeof(http_attrs[0]),
    nullptr,
};

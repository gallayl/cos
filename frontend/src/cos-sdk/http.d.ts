/**
 * `http` module (R24 v1).
 * Script-facing user HTTP server API for Berry/TypeScript.
 * Separate from the core device HTTP server (different instance/port, no core route/auth coupling).
 *
 * v1 limits: single in-flight request per server, max URL 512 B, max 16 headers / 4 KB total,
 * max body 4 KB (text/raw and application/json), max 16 routes+middleware, handler timeout enforced
 * at 5000 ms default (504 returned if the handler does not complete in time).
 * No built-in auth, no TLS/HTTPS, buffered bodies only. LAN-reachable by default (0.0.0.0).
 */

declare global {
  namespace http {
    /** Opaque handle to a user HTTP server instance. */
    type Server = unknown;

    /** Route/middleware registration id returned by route helpers; pass to `remove()`. */
    type RouteId = number;

    /** HTTP request object passed to handlers (v1). */
    interface Request {
      method: string;
      url: string;
      path: string;
      params: { [name: string]: string };
      query: { [name: string]: string };
      headers: { [name: string]: string };
      body: string;
      text: string;
      json: unknown;
    }

    /** HTTP response control object passed to handlers (v1). Use helper functions to send a response. */
    interface Response {
      _status?: number;
      _sent?: boolean;
      _body?: string;
      _headers?: { [name: string]: string };
    }

    /** Route/middleware handler. Call response helpers (e.g. `http.res_json`) to send a response. */
    type Handler = (req: Request, res: Response, next?: () => void) => void;

    /** Create a separate user HTTP server instance. */
    function create_server(): Server;

    /**
     * Start listening on a user-defined non-core port.
     * @param port TCP port (must not conflict with core server port; core default is 80).
     * @param host Bind host (default `0.0.0.0` if omitted; `127.0.0.1` allowed).
     * @returns `true` on success; throws/raises on error with no partial listener/start.
     */
    function listen(server: Server, port: number, host?: string): true;

    /** Stop the server, unregister routes/handlers, and release resources (idempotent). */
    function close(server: Server): boolean;

    /** Register route handlers (exact path and parameterized `:param` segments). */
    function get(server: Server, path: string, handler: Handler): RouteId;
    function post(server: Server, path: string, handler: Handler): RouteId;
    function put(server: Server, path: string, handler: Handler): RouteId;
    // function delete(server: Server, path: string, handler: Handler): RouteId; // Berry binding name; reserved in TS declarations
    /** HTTP DELETE route registration. */
    function del(server: Server, path: string, handler: Handler): RouteId;
    function patch(server: Server, path: string, handler: Handler): RouteId;
    function head(server: Server, path: string, handler: Handler): RouteId;
    function options(server: Server, path: string, handler: Handler): RouteId;
    function all(server: Server, path: string, handler: Handler): RouteId;
    function any(server: Server, path: string, handler: Handler): RouteId;
    function use(server: Server, path: string, handler: Handler): RouteId;

    /** Remove a previously registered route/middleware by id. */
    function remove(server: Server, route_id: RouteId): boolean;

    /** Response helpers (v1). */
    function res_status(res: Response, code: number): Response;
    function res_set(res: Response, name: string, value: string): Response;
    function res_header(res: Response, name: string, value: string): Response;
    function res_send(res: Response, body: string): boolean;
    function res_json(res: Response, value: unknown): boolean;
    function res_end(res: Response, body?: string): boolean;

    /** Continue to the next middleware/handler in the current dispatch chain (v1). */
    function next(): boolean;
  }
}

export {};

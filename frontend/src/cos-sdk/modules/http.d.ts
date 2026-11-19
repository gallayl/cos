declare module '@cos/http' {
  export type Server = unknown;
  export type RouteId = number;

  export interface Request {
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

  export interface Response {
    _status?: number;
    _sent?: boolean;
    _body?: string;
    _headers?: { [name: string]: string };
  }

  export type Handler = (req: Request, res: Response, next?: () => void) => void;

  export function create_server(): Server;
  export function listen(server: Server, port: number, host?: string): true;
  export function close(server: Server): boolean;

  export function get(server: Server, path: string, handler: Handler): RouteId;
  export function post(server: Server, path: string, handler: Handler): RouteId;
  export function put(server: Server, path: string, handler: Handler): RouteId;
  export function del(server: Server, path: string, handler: Handler): RouteId;
  export { del as delete };
  export function patch(server: Server, path: string, handler: Handler): RouteId;
  export function head(server: Server, path: string, handler: Handler): RouteId;
  export function options(server: Server, path: string, handler: Handler): RouteId;
  export function all(server: Server, path: string, handler: Handler): RouteId;
  export function any(server: Server, path: string, handler: Handler): RouteId;
  export function use(server: Server, path: string, handler: Handler): RouteId;
  export function remove(server: Server, route_id: RouteId): boolean;

  export function res_status(res: Response, code: number): Response;
  export function res_set(res: Response, name: string, value: string): Response;
  export function res_header(res: Response, name: string, value: string): Response;
  export function res_send(res: Response, body: string): boolean;
  export function res_json(res: Response, value: unknown): boolean;
  export function res_end(res: Response, body?: string): boolean;
  export function next(): boolean;

  const http: {
    create_server: typeof create_server;
    listen: typeof listen;
    close: typeof close;
    get: typeof get;
    post: typeof post;
    put: typeof put;
    delete: typeof del;
    del: typeof del;
    patch: typeof patch;
    head: typeof head;
    options: typeof options;
    all: typeof all;
    any: typeof any;
    use: typeof use;
    remove: typeof remove;
    res_status: typeof res_status;
    res_set: typeof res_set;
    res_header: typeof res_header;
    res_send: typeof res_send;
    res_json: typeof res_json;
    res_end: typeof res_end;
    next: typeof next;
  };
  export default http;
}

declare module '@berry/http' {
  export * from '@cos/http';
  import _default from '@cos/http';
  export default _default;
}

export class NotImplementedError extends Error {
  constructor(apiName: string) {
    super(`${apiName} has not been connected to the C++ backend yet.`)
    this.name = 'NotImplementedError'
  }
}
export function notImplemented<T>(apiName: string): Promise<T> { return Promise.reject(new NotImplementedError(apiName)) }
export interface RequestOptions { method?: 'GET' | 'POST' | 'PUT' | 'DELETE'; query?: Record<string, string | number | boolean | undefined>; body?: unknown; auth?: boolean }
export interface ApiClientConfig { baseUrl: string; token?: string }

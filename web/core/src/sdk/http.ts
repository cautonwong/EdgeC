/**
 * 请求层（§3.2 / §7.1）。
 *
 * 统一做四件事：带 requestId、带会话 Cookie、状态变更带 CSRF 头、把错误规范化成
 * `{ code, message, details, requestId }`。写操作**不自动重试**——重试一个有副作用
 * 的请求是另一类 bug（幂等键不在这一层解决）。
 */
import type { ApiError } from './types';

export class ApiRequestError extends Error {
  readonly status: number;
  readonly code: string;
  readonly requestId: string;
  readonly details?: Record<string, unknown>;

  constructor(status: number, error: ApiError) {
    super(error.message);
    this.name = 'ApiRequestError';
    this.status = status;
    this.code = error.code;
    this.requestId = error.requestId;
    this.details = error.details;
  }
}

let csrfToken = '';

export function setCsrfToken(token: string): void {
  csrfToken = token;
}

function newRequestId(): string {
  const cryptoObj = globalThis.crypto;
  if (cryptoObj && 'randomUUID' in cryptoObj) return cryptoObj.randomUUID();
  return `req-${Date.now()}-${Math.random().toString(16).slice(2)}`;
}

const MUTATING = new Set(['POST', 'PUT', 'PATCH', 'DELETE']);

export interface RequestOptions {
  method?: 'GET' | 'POST' | 'PUT' | 'PATCH' | 'DELETE';
  body?: unknown;
  signal?: AbortSignal;
  /** 幂等键：升级、备份、配置 apply 等创建型副作用操作必须带 */
  idempotencyKey?: string;
}

export async function request<T>(path: string, options: RequestOptions = {}): Promise<T> {
  const method = options.method ?? 'GET';
  const requestId = newRequestId();
  const headers: Record<string, string> = {
    'X-Request-Id': requestId,
    Accept: 'application/json',
  };
  if (options.body !== undefined) headers['Content-Type'] = 'application/json';
  if (MUTATING.has(method)) {
    headers['X-CSRF-Token'] = csrfToken;
  }
  if (options.idempotencyKey) headers['Idempotency-Key'] = options.idempotencyKey;

  const response = await fetch(path, {
    method,
    headers,
    credentials: 'same-origin',
    body: options.body === undefined ? undefined : JSON.stringify(options.body),
    signal: options.signal,
  });

  if (response.status === 204) return undefined as T;

  const text = await response.text();
  const payload = text ? safeParse(text) : null;

  if (!response.ok) {
    const error: ApiError = {
      code: payload?.code ?? `http_${response.status}`,
      message: payload?.message ?? response.statusText ?? '请求失败',
      details: payload?.details,
      requestId: payload?.requestId ?? requestId,
    };
    throw new ApiRequestError(response.status, error);
  }

  return payload as T;
}

function safeParse(text: string): any {
  try {
    return JSON.parse(text);
  } catch {
    return null;
  }
}

/**
 * 会话与身份（§5.6 / §6.1）。
 *
 * 这里只保存"服务端告诉我的事实"：`/auth/me` 返回的角色、展开后的权限 ID、
 * 范围摘要、必须改密标记与策略版本。**前端不推断权限**，也不把本地缓存当授权依据。
 */
import { defineStore } from 'pinia';
import { computed, ref } from 'vue';
import type { AuthMe, SystemInfo } from '../../sdk/types';
import { ApiRequestError, request, setCsrfToken } from '../../sdk/http';

type SessionStatus = 'booting' | 'anonymous' | 'authenticated' | 'error';

export const useSessionStore = defineStore('session', () => {
  const systemInfo = ref<SystemInfo | null>(null);
  const me = ref<AuthMe | null>(null);
  const status = ref<SessionStatus>('booting');
  const error = ref<string | null>(null);

  const isAuthenticated = computed(() => status.value === 'authenticated' && me.value !== null);
  const mustChangePassword = computed(() => me.value?.mustChangePassword === true);
  const permissions = computed(() => new Set(me.value?.permissions ?? []));
  const displayName = computed(() => me.value?.displayName ?? me.value?.username ?? '');
  const roles = computed(() => me.value?.roles?.map((r) => r.roleId) ?? []);

  function applyMe(next: AuthMe): void {
    me.value = next;
    setCsrfToken(next.csrfToken);
    status.value = 'authenticated';
    error.value = null;
  }

  async function loadSystemInfo(): Promise<void> {
    systemInfo.value = await request<SystemInfo>('/api/v1/system/info');
  }

  /** 启动流程：先拿最小系统信息（匿名），再试 `/auth/me`。401 不是错误，是"未登录"。 */
  async function bootstrap(): Promise<void> {
    status.value = 'booting';
    error.value = null;
    try {
      await loadSystemInfo();
    } catch (err) {
      error.value = describe(err);
      status.value = 'error';
      return;
    }
    try {
      applyMe(await request<AuthMe>('/api/v1/auth/me'));
    } catch (err) {
      if (err instanceof ApiRequestError && err.status === 401) {
        me.value = null;
        status.value = 'anonymous';
        return;
      }
      error.value = describe(err);
      status.value = 'error';
    }
  }

  async function login(username: string, password: string): Promise<void> {
    const result = await request<{ user: AuthMe }>('/api/v1/auth/login', {
      method: 'POST',
      body: { username, password },
    });
    applyMe(result.user);
  }

  async function logout(): Promise<void> {
    try {
      await request<void>('/api/v1/auth/logout', { method: 'POST' });
    } finally {
      me.value = null;
      setCsrfToken('');
      status.value = 'anonymous';
    }
  }

  async function changePassword(currentPassword: string, newPassword: string): Promise<void> {
    await request<void>('/api/v1/auth/password', {
      method: 'POST',
      body: { currentPassword, newPassword },
    });
    // 改密后服务端撤销其他会话并轮换会话；重新拉一次主体拿到新的 CSRF token。
    applyMe(await request<AuthMe>('/api/v1/auth/me'));
  }

  async function refresh(): Promise<void> {
    applyMe(await request<AuthMe>('/api/v1/auth/me'));
  }

  return {
    systemInfo,
    me,
    status,
    error,
    isAuthenticated,
    mustChangePassword,
    permissions,
    displayName,
    roles,
    bootstrap,
    login,
    logout,
    changePassword,
    refresh,
  };
});

function describe(err: unknown): string {
  if (err instanceof ApiRequestError) return `${err.code}: ${err.message}（requestId ${err.requestId}）`;
  if (err instanceof Error) return err.message;
  return String(err);
}

<script setup lang="ts">
/**
 * 登录页（§8.5）：只展示产品显示名、有限型号信息与语言，不暴露用户是否存在、
 * 内部组件版本、序列号或许可证细节。错误信息统一，不做账号枚举。
 */
import { computed, onBeforeUnmount, ref } from 'vue';
import { useRoute, useRouter } from 'vue-router';
import { ApiRequestError } from '../../sdk/http';
import { useSessionStore } from '../stores/session';
import UiButton from '../../ui/components/Button.vue';
import UiTextField from '../../ui/components/TextField.vue';
import UiBanner from '../../ui/components/Banner.vue';

const session = useSessionStore();
const router = useRouter();
const route = useRoute();

const username = ref('');
const password = ref('');
const submitting = ref(false);
const locked = ref(0);
let lockTimer: ReturnType<typeof setInterval> | undefined;

function startCountdown(): void {
  if (lockTimer) clearInterval(lockTimer);
  lockTimer = setInterval(() => {
    locked.value = Math.max(0, locked.value - 1);
    if (locked.value === 0) {
      clearInterval(lockTimer);
      lockTimer = undefined;
      error.value = '';
    }
  }, 1000);
}

onBeforeUnmount(() => {
  if (lockTimer) clearInterval(lockTimer);
});
const error = ref('');

const productName = computed(() => session.systemInfo?.displayName ?? 'EdgeOS');
const language = computed(() => session.systemInfo?.language ?? 'zh-CN');

async function submit(): Promise<void> {
  error.value = '';
  if (!username.value || !password.value) {
    error.value = '请输入用户名与口令';
    return;
  }
  submitting.value = true;
  try {
    await session.login(username.value, password.value);
    if (session.mustChangePassword) {
      await router.replace({ name: 'change-password' });
      return;
    }
    const redirect = typeof route.query.redirect === 'string' ? route.query.redirect : '/';
    await router.replace(redirect);
  } catch (err) {
    if (err instanceof ApiRequestError && err.status === 429) {
      // 锁定期间服务端连口令都不校验，页面就别再引导用户"再试一次"
      const retryAfter = Number((err.details as { retryAfterSeconds?: number } | undefined)?.retryAfterSeconds ?? 0);
      locked.value = retryAfter > 0 ? retryAfter : 30;
      error.value = `尝试次数过多，请 ${locked.value} 秒后再试`;
      startCountdown();
      return;
    }
    if (err instanceof ApiRequestError && err.status === 401) {
      // 401 不区分"用户不存在"和"口令错误"（避免账号枚举），只提示还剩几次
      const remaining = Number((err.details as { attemptsRemaining?: number } | undefined)?.attemptsRemaining ?? -1);
      error.value = remaining > 0 ? `用户名或口令错误（还可尝试 ${remaining} 次）` : '用户名或口令错误';
      return;
    }
    error.value = err instanceof Error ? err.message : '登录失败';
  } finally {
    submitting.value = false;
  }
}
</script>

<template>
  <main class="login">
    <section class="login__card">
      <header class="login__head">
        <p class="login__brand">{{ productName }}</p>
        <h1 class="login__title">登录设备管理界面</h1>
        <p class="login__lang">语言 / Language：{{ language }}</p>
      </header>

      <UiBanner v-if="error" tone="danger" :data-testid="locked > 0 ? 'login-locked' : 'login-error'">{{ error }}</UiBanner>

      <form class="login__form" @submit.prevent="submit">
        <UiTextField
          v-model="username"
          label="用户名"
          autocomplete="username"
          data-testid="login-username"
          required
        />
        <UiTextField
          v-model="password"
          label="口令"
          type="password"
          autocomplete="current-password"
          data-testid="login-password"
          required
        />
        <UiButton type="submit" variant="primary" :loading="submitting" data-testid="login-submit">
          登录
        </UiButton>
      </form>

      <p class="login__note">首次登录必须修改初始口令；本设备不存在通用默认口令。</p>
    </section>
  </main>
</template>

<style scoped>
.login {
  min-height: 100vh;
  display: grid;
  place-items: center;
  padding: var(--space-6);
  background: var(--bg-page);
}
.login__card {
  width: min(420px, 100%);
  padding: var(--space-8);
  background: var(--bg-surface);
  border: 1px solid var(--border-default);
  border-radius: var(--radius-lg);
  box-shadow: var(--shadow-2);
}
.login__head {
  margin-bottom: var(--space-6);
}
.login__brand {
  margin: 0 0 var(--space-2);
  color: var(--color-primary);
  font-weight: 600;
  letter-spacing: 0.04em;
}
.login__title {
  margin: 0 0 var(--space-1);
  font-size: var(--font-size-xl);
  font-weight: 600;
}
.login__lang {
  margin: 0;
  color: var(--text-secondary);
  font-size: var(--font-size-xs);
}
.login__form {
  margin-top: var(--space-4);
}
.login__note {
  margin: var(--space-4) 0 0;
  color: var(--text-secondary);
  font-size: var(--font-size-xs);
}
</style>

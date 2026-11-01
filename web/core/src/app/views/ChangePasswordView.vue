<script setup lang="ts">
/**
 * 强制改密页（§5.6 / WBUI-020）：首登必须改密；口令规则在前端只做**提示性**校验，
 * 服务端仍然独立校验（前端校验不是安全边界）。
 */
import { computed, ref } from 'vue';
import { useRouter } from 'vue-router';
import { ApiRequestError } from '../../sdk/http';
import { useSessionStore } from '../stores/session';
import UiButton from '../../ui/components/Button.vue';
import UiTextField from '../../ui/components/TextField.vue';
import UiBanner from '../../ui/components/Banner.vue';

const session = useSessionStore();
const router = useRouter();

const currentPassword = ref('');
const newPassword = ref('');
const confirmPassword = ref('');
const submitting = ref(false);
const error = ref('');

const ruleHint = computed(() => '至少 12 位，且不能与用户名相同');
const tooShort = computed(() => newPassword.value.length > 0 && newPassword.value.length < 12);
const sameAsUsername = computed(
  () => newPassword.value.length > 0 && newPassword.value === (session.me?.username ?? ''),
);
const mismatch = computed(() => confirmPassword.value.length > 0 && confirmPassword.value !== newPassword.value);

const canSubmit = computed(
  () =>
    currentPassword.value.length > 0 &&
    newPassword.value.length >= 12 &&
    !sameAsUsername.value &&
    confirmPassword.value === newPassword.value,
);

async function submit(): Promise<void> {
  error.value = '';
  if (!canSubmit.value) {
    error.value = '请按口令规则填写';
    return;
  }
  submitting.value = true;
  try {
    await session.changePassword(currentPassword.value, newPassword.value);
    await router.replace('/');
  } catch (err) {
    error.value = err instanceof ApiRequestError ? `${err.message}（${err.code}）` : '修改失败';
  } finally {
    submitting.value = false;
  }
}
</script>

<template>
  <main class="cp">
    <section class="cp__card">
      <h1 class="cp__title">必须修改初始口令</h1>
      <p class="cp__lead">
        当前账号（{{ session.me?.username }}）仍在使用初始口令，修改后才能进入系统。
      </p>

      <UiBanner v-if="error" tone="danger" data-testid="cp-error">{{ error }}</UiBanner>

      <form @submit.prevent="submit">
        <UiTextField
          v-model="currentPassword"
          label="当前口令"
          type="password"
          autocomplete="current-password"
          data-testid="cp-current"
          required
        />
        <UiTextField
          v-model="newPassword"
          label="新口令"
          type="password"
          autocomplete="new-password"
          :hint="ruleHint"
          :error="tooShort ? '长度不足 12 位' : sameAsUsername ? '不能与用户名相同' : undefined"
          data-testid="cp-new"
          required
        />
        <UiTextField
          v-model="confirmPassword"
          label="确认新口令"
          type="password"
          autocomplete="new-password"
          :error="mismatch ? '两次输入不一致' : undefined"
          data-testid="cp-confirm"
          required
        />
        <UiButton type="submit" variant="primary" :loading="submitting" :disabled="!canSubmit" data-testid="cp-submit">
          修改口令
        </UiButton>
      </form>
    </section>
  </main>
</template>

<style scoped>
.cp {
  min-height: 100vh;
  display: grid;
  place-items: center;
  padding: var(--space-6);
  background: var(--bg-page);
}
.cp__card {
  width: min(460px, 100%);
  padding: var(--space-8);
  background: var(--bg-surface);
  border: 1px solid var(--border-default);
  border-radius: var(--radius-lg);
}
.cp__title {
  margin: 0 0 var(--space-2);
  font-size: var(--font-size-xl);
}
.cp__lead {
  margin: 0 0 var(--space-4);
  color: var(--text-secondary);
  font-size: var(--font-size-sm);
}
</style>

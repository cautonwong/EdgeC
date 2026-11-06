<template>
  <section class="page">
    <header class="head">
      <h1>定时任务</h1>
      <p>时间源与冻结保护</p>
    </header>

    <UiBanner v-if="error" tone="danger" title="读取失败">{{ error }}</UiBanner>
    <p v-else-if="loading" class="muted">加载中…</p>

    <div v-else class="grid">
      <section class="card">
        <h2>时间源</h2>
        <dl>
          <div>
            <dt>单调时钟</dt>
            <dd :class="status?.monotonic_ok ? 'ok' : 'bad'">{{ status?.monotonic_ok ? '已锁定' : '异常' }}</dd>
          </div>
          <div>
            <dt>日历源</dt>
            <dd>{{ status?.calendar_source?.toUpperCase() ?? '—' }}</dd>
          </div>
          <div>
            <dt>最近 NTP 同步</dt>
            <dd>{{ lastSync }}</dd>
          </div>
        </dl>
      </section>

      <section class="card">
        <h2>冻结保护</h2>
        <p class="note">工业级 TimeGuard 已启用，系统可抵御时钟跳变。</p>
      </section>
    </div>
  </section>
</template>

<script setup lang="ts">
import { computed, onMounted, ref } from 'vue';
import { ApiRequestError, request } from '../../sdk/http';
import UiBanner from '../../ui/components/Banner.vue';

interface TimeStatus {
  monotonic_ok: boolean;
  calendar_source?: string;
  last_sync_ms: number;
}

const status = ref<TimeStatus | null>(null);
const loading = ref(true);
const error = ref('');

const lastSync = computed(() => {
  if (!status.value?.last_sync_ms) return '从未同步';
  return new Date(status.value.last_sync_ms).toLocaleString('zh-CN', { hour12: false });
});

onMounted(async () => {
  try {
    status.value = await request<TimeStatus>('/api/v1/time/status');
  } catch (err) {
    error.value = err instanceof ApiRequestError ? `${err.code}: ${err.message}` : String(err);
  } finally {
    loading.value = false;
  }
});
</script>

<style scoped>
.page {
  display: flex;
  flex-direction: column;
  gap: var(--space-4);
}
.head h1 {
  margin: 0;
  font-size: var(--font-size-xl);
}
.head p {
  margin: var(--space-1) 0 0;
  color: var(--text-secondary);
  font-size: var(--font-size-sm);
}
.grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(260px, 1fr));
  gap: var(--space-4);
}
.card {
  padding: var(--space-4);
  background: var(--bg-surface);
  border: 1px solid var(--border-default);
  border-radius: var(--radius-lg);
}
.card h2 {
  margin: 0 0 var(--space-3);
  font-size: var(--font-size-lg);
}
dl {
  margin: 0;
  display: flex;
  flex-direction: column;
  gap: var(--space-2);
}
dl > div {
  display: flex;
  justify-content: space-between;
  gap: var(--space-3);
}
dt {
  color: var(--text-secondary);
  font-size: var(--font-size-sm);
}
dd {
  margin: 0;
  font-variant-numeric: tabular-nums;
}
.ok {
  color: var(--color-success);
}
.bad {
  color: var(--color-danger);
}
.note {
  margin: 0;
  padding: var(--space-3);
  background: color-mix(in srgb, var(--color-info) 10%, transparent);
  border: 1px solid color-mix(in srgb, var(--color-info) 30%, transparent);
  border-radius: var(--radius-md);
  color: var(--text-primary);
  font-size: var(--font-size-sm);
}
.muted {
  color: var(--text-secondary);
}
</style>

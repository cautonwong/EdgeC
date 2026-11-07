<template>
  <section class="page">
    <header class="head">
      <h1>存储与数据</h1>
      <p>数据库与表统计</p>
    </header>

    <UiBanner v-if="error" tone="danger" title="读取失败">{{ error }}</UiBanner>
    <p v-else-if="loading" class="muted">加载中…</p>

    <section v-else class="card">
      <dl>
        <div>
          <dt>数据库引擎</dt>
          <dd>SQLite WAL</dd>
        </div>
        <div>
          <dt>数据库大小</dt>
          <dd>{{ stats?.db_size_kb ?? '—' }} KB</dd>
        </div>
        <div>
          <dt>活动表数量</dt>
          <dd>{{ stats?.table_count ?? '—' }}</dd>
        </div>
      </dl>
    </section>
  </section>
</template>

<script setup lang="ts">
import { onMounted, ref } from 'vue';
import { ApiRequestError, request } from '../../sdk/http';
import UiBanner from '../../ui/components/Banner.vue';

interface StorageStats {
  db_size_kb: number;
  table_count: number;
}

const stats = ref<StorageStats | null>(null);
const loading = ref(true);
const error = ref('');

onMounted(async () => {
  try {
    stats.value = await request<StorageStats>('/api/v1/storage/status');
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
.card {
  padding: var(--space-4);
  background: var(--bg-surface);
  border: 1px solid var(--border-default);
  border-radius: var(--radius-lg);
}
dl {
  margin: 0;
  display: flex;
  flex-direction: column;
  gap: var(--space-3);
}
dl > div {
  display: flex;
  justify-content: space-between;
  gap: var(--space-3);
  padding-bottom: var(--space-3);
  border-bottom: 1px solid var(--border-default);
}
dl > div:last-child {
  padding-bottom: 0;
  border-bottom: 0;
}
dt {
  color: var(--text-secondary);
  font-size: var(--font-size-sm);
}
dd {
  margin: 0;
  font-variant-numeric: tabular-nums;
}
.muted {
  color: var(--text-secondary);
}
</style>

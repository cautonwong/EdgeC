<script setup lang="ts">
/**
 * iEMS 策略页：页面级 `requires.entitlements: [iems.peak-shaving]`。
 * 许可缺失时菜单不出现、直接输入 URL 落到"功能未授权"，服务端同样拒绝
 * `x-required-entitlements` 的接口——三层判定各自独立。
 */
import { onMounted, ref } from 'vue';
import { request, ApiRequestError } from '@edgeos/web-core/sdk';
import { UiDataTable, UiBanner, type Column } from '@edgeos/web-core/ui';

interface StrategyRow {
  id: string;
  name: string;
  enabled: boolean;
  window: string;
}

const rows = ref<StrategyRow[]>([]);
const error = ref('');
const loading = ref(false);

const columns: Column[] = [
  { key: 'name', title: '策略' },
  { key: 'enabled', title: '状态' },
  { key: 'window', title: '时段' },
];

onMounted(async () => {
  loading.value = true;
  try {
    const result = await request<{ items: StrategyRow[] }>('/api/v1/iems/strategy');
    rows.value = result.items;
  } catch (err) {
    error.value = err instanceof ApiRequestError ? `${err.code}: ${err.message}` : String(err);
  } finally {
    loading.value = false;
  }
});
</script>

<template>
  <section>
    <header class="head">
      <h1>削峰填谷策略</h1>
      <p class="head__hint">写入属于 R4（业务控制/物理影响），需单独授权与维护窗口，不在本页直接下发。</p>
    </header>
    <UiBanner v-if="error" tone="danger" title="读取失败">{{ error }}</UiBanner>
    <UiDataTable :columns="columns" :rows="rows" row-key="id" :loading="loading" caption="策略列表">
      <template #cell-enabled="{ row }">{{ row.enabled ? '已启用' : '已停用' }}</template>
    </UiDataTable>
  </section>
</template>

<style scoped>
.head {
  margin-bottom: var(--space-4);
}
.head h1 {
  margin: 0 0 var(--space-1);
  font-size: var(--font-size-xl);
}
.head__hint {
  margin: 0;
  color: var(--text-secondary);
  font-size: var(--font-size-sm);
}
</style>

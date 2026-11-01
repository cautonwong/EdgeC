<script setup lang="ts">
/** DCU 电表档案：页面声明 `requires.capabilities: [software.collector.dlt698]`。 */
import { onMounted, ref } from 'vue';
import { request, ApiRequestError } from '@edgeos/web-core/sdk';
import { UiDataTable, UiBanner, UiStatusBadge, type Column } from '@edgeos/web-core/ui';

interface MeterRow {
  id: string;
  address: string;
  name: string;
  protocol: string;
  online: boolean;
  lastReadingAt: string;
}

const rows = ref<MeterRow[]>([]);
const loading = ref(false);
const error = ref('');

const columns: Column[] = [
  { key: 'address', title: '通信地址' },
  { key: 'name', title: '名称' },
  { key: 'protocol', title: '协议' },
  { key: 'online', title: '在线' },
  { key: 'lastReadingAt', title: '最近抄读' },
];

onMounted(async () => {
  loading.value = true;
  try {
    const result = await request<{ items: MeterRow[] }>('/api/v1/dcu/meters');
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
      <h1>电表档案</h1>
      <p class="head__hint">档案维护属高风险写操作（dcu.meter:write），需二次确认并审计。</p>
    </header>
    <UiBanner v-if="error" tone="danger" title="读取失败">{{ error }}</UiBanner>
    <UiDataTable :columns="columns" :rows="rows" row-key="id" :loading="loading" caption="电表档案" data-testid="dcu-meters">
      <template #cell-online="{ row }">
        <UiStatusBadge :status="row.online ? 'normal' : 'offline'" :label="row.online ? '在线' : '离线'" />
      </template>
      <template #cell-lastReadingAt="{ row }">{{ row.lastReadingAt }}</template>
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

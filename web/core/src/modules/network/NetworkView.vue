<script setup lang="ts">
/** 网络接口（§8.1 网络与通信）。写操作走 R2 事务，这里只做只读呈现 + 入口提示。 */
import { onMounted, ref } from 'vue';
import { request, ApiRequestError } from '../../sdk/http';
import { useAuthz } from '../../app/composables/useAuthz';
import UiDataTable, { type Column } from '../../ui/components/DataTable.vue';
import UiStatusBadge from '../../ui/components/StatusBadge.vue';
import UiBanner from '../../ui/components/Banner.vue';

interface InterfaceRow {
  id: string;
  name: string;
  mac: string;
  addresses: string[];
  state: 'up' | 'down';
  speedMbps: number | null;
}

const authz = useAuthz();
const rows = ref<InterfaceRow[]>([]);
const loading = ref(false);
const error = ref('');

const columns: Column[] = [
  { key: 'name', title: '接口' },
  { key: 'state', title: '状态' },
  { key: 'addresses', title: '地址' },
  { key: 'mac', title: 'MAC' },
  { key: 'speedMbps', title: '速率 (Mbps)', align: 'right' },
];

onMounted(async () => {
  loading.value = true;
  try {
    const result = await request<{ items: InterfaceRow[] }>('/api/v1/network/interfaces');
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
      <h1>网络接口</h1>
      <p class="head__hint">修改 IP / 网关属于 R2 风险，应用后需在确认窗口内确认，否则自动回滚。</p>
    </header>

    <UiBanner v-if="error" tone="danger" title="读取失败">{{ error }}</UiBanner>
    <UiBanner v-else-if="!authz.can('network.interface:write')" tone="info" data-testid="readonly-notice">
      当前账号只有只读权限（network.interface:read）。
    </UiBanner>

    <UiDataTable :columns="columns" :rows="rows" row-key="id" :loading="loading" caption="接口列表">
      <template #cell-state="{ row }">
        <UiStatusBadge :status="row.state === 'up' ? 'normal' : 'offline'" :label="row.state === 'up' ? '已连接' : '未连接'" />
      </template>
      <template #cell-addresses="{ row }">{{ (row.addresses as string[]).join(', ') || '—' }}</template>
      <template #cell-speedMbps="{ row }">{{ row.speedMbps ?? '—' }}</template>
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

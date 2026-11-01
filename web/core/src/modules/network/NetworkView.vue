<script setup lang="ts">
/**
 * 网络接口（§8.1 网络与通信 / §6.4 配置事务）。
 *
 * 这个页面是 R2 风险的样板：改 IP/网关可能把自己锁在门外，所以
 *   编辑 → 应用 → 60 秒内确认 → 否则设备自动回滚。
 * 表单字段来自 Registry 里的 `network.interfaces` Schema（与后端同一份），
 * 前端校验只做提前提示，服务端 422 的 fieldErrors 才是结论。
 */
import { computed, onBeforeUnmount, onMounted, ref, watch } from 'vue';
import { ApiRequestError, request } from '../../sdk/http';
import { useAuthz } from '../../app/composables/useAuthz';
import { useConfigSection } from '../../app/composables/useConfigSection';
import ConfigTransactionBar from '../../app/components/ConfigTransactionBar.vue';
import UiBanner from '../../ui/components/Banner.vue';
import UiButton from '../../ui/components/Button.vue';
import UiDataTable, { type Column } from '../../ui/components/DataTable.vue';
import UiSchemaForm from '../../ui/components/SchemaForm.vue';
import UiStatusBadge from '../../ui/components/StatusBadge.vue';

interface InterfaceRow {
  id: string;
  name: string;
  mac: string;
  addresses: string[];
  gateway?: string;
  mtu?: number;
  state: 'up' | 'down';
  speedMbps: number | null;
}

const authz = useAuthz();
const { tx, schema, open } = useConfigSection('network.interfaces');

const rows = ref<InterfaceRow[]>([]);
const loading = ref(false);
const error = ref('');
const editingId = ref<string | null>(null);

const canWrite = computed(() => authz.can('network.interface:write'));
const columns: Column[] = [
  { key: 'name', title: '接口' },
  { key: 'state', title: '状态' },
  { key: 'addresses', title: '地址' },
  { key: 'gateway', title: '网关' },
  { key: 'mtu', title: 'MTU', align: 'right' },
  { key: 'mac', title: 'MAC' },
];

async function load(): Promise<void> {
  loading.value = true;
  error.value = '';
  try {
    const result = await request<{ items: InterfaceRow[] }>('/api/v1/network/interfaces');
    rows.value = result.items;
  } catch (err) {
    error.value = err instanceof ApiRequestError ? `${err.code}: ${err.message}` : String(err);
  } finally {
    loading.value = false;
  }
}

function startEdit(row: InterfaceRow): void {
  editingId.value = row.id;
  open({
    interfaceId: row.id,
    addresses: [...row.addresses],
    gateway: row.gateway ?? '',
    mtu: row.mtu ?? 1500,
  });
  void tx.resume();
}

function cancelEdit(): void {
  editingId.value = null;
  tx.reset();
}

// 事务落地后刷新列表：否则"已确认"之后表格里还是旧 IP，用户会以为没生效。
watch(
  () => tx.transaction?.state,
  (state) => {
    if (state === 'CONFIRMED' || state === 'ROLLED_BACK') void load();
  },
);

onMounted(load);
onBeforeUnmount(() => tx.stopTicker());
</script>

<template>
  <section>
    <header class="head">
      <div>
        <h1>网络接口</h1>
        <p class="head__hint">
          修改 IP / 网关属于 <strong>R2（管理通道失联）</strong>：应用后必须在
          {{ schema?.confirmWindowSeconds ?? 60 }} 秒内确认，否则设备自动回滚。
        </p>
      </div>
      <UiStatusBadge v-if="!canWrite" status="unconfigured" label="只读（缺少 network.interface:write）" />
    </header>

    <UiBanner v-if="error" tone="danger" title="读取失败">{{ error }}</UiBanner>

    <ConfigTransactionBar v-if="editingId" />

    <UiDataTable :columns="columns" :rows="rows" row-key="id" :loading="loading" caption="接口列表">
      <template #cell-state="{ row }">
        <UiStatusBadge :status="row.state === 'up' ? 'normal' : 'offline'" :label="row.state === 'up' ? '已连接' : '未连接'" />
      </template>
      <template #cell-addresses="{ row }">{{ (row.addresses as string[]).join(', ') || '—' }}</template>
      <template #cell-gateway="{ row }">{{ row.gateway ?? '—' }}</template>
      <template #cell-mtu="{ row }">{{ row.mtu ?? '—' }}</template>
    </UiDataTable>

    <div class="actions">
      <UiButton
        v-for="row in rows"
        :key="row.id"
        variant="secondary"
        size="sm"
        :disabled="!canWrite"
        :data-testid="`edit-${row.id}`"
        @click="startEdit(row)"
      >
        编辑 {{ row.name }}
      </UiButton>
    </div>

    <div v-if="editingId && schema" class="editor" data-testid="network-form">
      <h2 class="editor__title">编辑 {{ editingId }}</h2>
      <UiSchemaForm
        :schema="schema"
        :model-value="tx.draft"
        :errors="tx.fieldErrors"
        :disabled="!canWrite"
        @change="tx.setValue"
      />
      <UiButton variant="ghost" size="sm" data-testid="network-cancel" @click="cancelEdit">关闭</UiButton>
    </div>
  </section>
</template>

<style scoped>
.head {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: var(--space-4);
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
.actions {
  display: flex;
  gap: var(--space-2);
  margin: var(--space-3) 0 var(--space-4);
}
.editor {
  max-width: 560px;
}
.editor__title {
  margin: 0 0 var(--space-3);
  font-size: var(--font-size-lg);
}
</style>

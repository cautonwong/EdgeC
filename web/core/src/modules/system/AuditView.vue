<script setup lang="ts">
/**
 * 审计日志（§5.7）。
 *
 * 高危动作（远程拉合闸、角色变更、会话撤销、告警静默、配置事务）一直都在写 audit，
 * 但之前**没有任何地方能读到**——事后追责只能翻进程日志。这一页把那条链补上。
 */
import { computed, onMounted, ref } from 'vue';
import { ApiRequestError, request } from '../../sdk/http';
import { useAuthz } from '../../app/composables/useAuthz';
import UiBanner from '../../ui/components/Banner.vue';
import UiButton from '../../ui/components/Button.vue';
import UiDataTable, { type Column } from '../../ui/components/DataTable.vue';

interface AuditEvent {
  ts: string;
  action: string;
  actor: string;
  actorId: string | null;
  actorName: string | null;
  ip: string | null;
  requestId: string;
  detail: Record<string, unknown>;
}

const authz = useAuthz();
const events = ref<AuditEvent[]>([]);
const actions = ref<string[]>([]);
const actionFilter = ref('');
const actorFilter = ref('');
const loading = ref(false);
const error = ref('');

const canExport = computed(() => authz.can('core.audit:export'));

const columns: Column[] = [
  { key: 'ts', title: '时间' },
  { key: 'action', title: '动作' },
  { key: 'actor', title: '操作人' },
  { key: 'ip', title: '来源' },
  { key: 'detail', title: '详情' },
  { key: 'requestId', title: '请求 ID' },
];

function describe(err: unknown): string {
  return err instanceof ApiRequestError ? `${err.code}: ${err.message}` : String(err);
}

function fmt(iso: string): string {
  const date = new Date(iso);
  return Number.isNaN(date.getTime()) ? iso : date.toLocaleString('zh-CN', { hour12: false });
}

function detailOf(detail: Record<string, unknown> | undefined): string {
  if (!detail) return '—';
  return Object.entries(detail)
    .map(([key, value]) => `${key}=${typeof value === 'object' ? JSON.stringify(value) : String(value)}`)
    .join(' · ');
}

async function load(): Promise<void> {
  loading.value = true;
  error.value = '';
  try {
    const query = new URLSearchParams({ limit: '200' });
    if (actionFilter.value) query.set('action', actionFilter.value);
    if (actorFilter.value) query.set('actor', actorFilter.value);
    const result = await request<{ items: AuditEvent[] }>(`/api/v1/audit/events?${query}`);
    events.value = result.items;
    if (actions.value.length === 0) {
      const all = await request<{ items: AuditEvent[] }>('/api/v1/audit/events?limit=500');
      actions.value = [...new Set(all.items.map((event) => event.action))].sort();
    }
  } catch (err) {
    error.value = describe(err);
  } finally {
    loading.value = false;
  }
}

/** 导出 CSV 在浏览器里生成：不额外开一个下载接口，少一条权限面。 */
function exportCsv(): void {
  const header = ['时间', '动作', '操作人', '来源', '请求 ID', '详情'];
  const rows = events.value.map((event) => [
    event.ts,
    event.action,
    event.actor,
    event.ip ?? '',
    event.requestId,
    detailOf(event.detail),
  ]);
  const escape = (value: string) => `"${String(value).replace(/"/g, '""')}"`;
  const csv = [header, ...rows].map((row) => row.map(escape).join(',')).join('\r\n');
  const url = URL.createObjectURL(new Blob([`\uFEFF${csv}`], { type: 'text/csv;charset=utf-8' }));
  const link = document.createElement('a');
  link.href = url;
  link.download = `audit-${new Date().toISOString().slice(0, 10)}.csv`;
  link.click();
  URL.revokeObjectURL(url);
}

onMounted(load);
</script>

<template>
  <section>
    <header class="head">
      <div>
        <h1>审计日志</h1>
        <p class="head__hint">
          高危动作（远程拉合闸、角色变更、会话撤销、告警静默、配置事务）全程留痕。
          服务端保留最近 500 条。
        </p>
      </div>
      <div class="head__actions">
        <UiButton v-if="canExport" variant="ghost" size="sm" data-testid="audit-export" @click="exportCsv">
          导出 CSV
        </UiButton>
        <UiButton variant="ghost" size="sm" data-testid="audit-refresh" @click="load">刷新</UiButton>
      </div>
    </header>

    <UiBanner v-if="error" tone="danger" title="操作失败" data-testid="audit-error">{{ error }}</UiBanner>

    <div class="filters">
      <label class="field">
        <span class="field__label">动作</span>
        <select v-model="actionFilter" class="field__input" data-testid="audit-action-filter" @change="load">
          <option value="">全部</option>
          <option v-for="action in actions" :key="action" :value="action">{{ action }}</option>
        </select>
      </label>
      <label class="field">
        <span class="field__label">操作人</span>
        <input v-model="actorFilter" class="field__input" data-testid="audit-actor-filter" placeholder="用户名" @change="load" />
      </label>
    </div>

    <UiDataTable
      :columns="columns"
      :rows="events"
      row-key="requestId"
      :loading="loading"
      caption="审计事件"
      data-testid="audit-table"
    >
      <template #cell-ts="{ row }">{{ fmt(row.ts as string) }}</template>
      <template #cell-actor="{ row }">
        {{ row.actor }}
        <span v-if="row.actorName" class="muted">（{{ row.actorName }}）</span>
      </template>
      <template #cell-ip="{ row }"><span class="mono">{{ row.ip ?? '—' }}</span></template>
      <template #cell-detail="{ row }"><span class="mono">{{ detailOf(row.detail as Record<string, unknown>) }}</span></template>
      <template #cell-requestId="{ row }"><span class="mono">{{ row.requestId }}</span></template>
    </UiDataTable>
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
  max-width: 760px;
  color: var(--text-secondary);
  font-size: var(--font-size-sm);
}
.head__actions,
.filters {
  display: flex;
  align-items: center;
  gap: var(--space-2);
}
.filters {
  margin-bottom: var(--space-3);
}
.field {
  display: flex;
  flex-direction: column;
  gap: var(--space-1);
}
.field__label {
  font-size: var(--font-size-xs);
  color: var(--text-secondary);
}
.field__input {
  min-height: var(--input-height);
  padding: 0 var(--space-3);
  border: 1px solid var(--border-default);
  border-radius: var(--radius-md);
  background: var(--bg-surface);
  color: var(--text-primary);
  font: inherit;
  font-size: var(--font-size-sm);
}
.mono {
  font-family: var(--font-mono);
  font-size: var(--font-size-xs);
}
.muted {
  color: var(--text-disabled);
}
</style>

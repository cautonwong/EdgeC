<script setup lang="ts">
/**
 * 告警与事件（§8.3）。
 *
 * 与旧 EventStreamHub 的区别：数据面从"契约点位轮询"换成 `/api/v1/alarms`，
 * 新增告警靠 WebSocket `alarm` 主题推过来——页面不用轮询、也不用用户点刷新。
 * 确认告警是写操作，按 core.alarm:write 判定（服务端也判）。
 */
import { computed, onMounted, ref } from 'vue';
import { ApiRequestError, request } from '../../sdk/http';
import { useAuthz } from '../../app/composables/useAuthz';
import { useLiveFeed } from '../../app/composables/useLiveFeed';
import UiBanner from '../../ui/components/Banner.vue';
import UiButton from '../../ui/components/Button.vue';
import UiDataTable, { type Column } from '../../ui/components/DataTable.vue';
import UiStatusBadge from '../../ui/components/StatusBadge.vue';

interface Alarm {
  id: string;
  ts: string;
  level: 'critical' | 'major' | 'minor' | 'warning' | 'info';
  source: string;
  code: string;
  message: string;
  state: 'active' | 'acked' | 'cleared';
  ackBy: string | null;
  ackAt: string | null;
}

const authz = useAuthz();

const items = ref<Alarm[]>([]);
const loading = ref(false);
const error = ref('');
const stateFilter = ref<'all' | 'active' | 'acked'>('all');
const levelFilter = ref<'all' | Alarm['level']>('all');
const flashing = ref<string[]>([]);

const canAck = computed(() => authz.can('core.alarm:write'));
const columns: Column[] = [
  { key: 'ts', title: '时间' },
  { key: 'level', title: '等级' },
  { key: 'source', title: '来源' },
  { key: 'message', title: '描述' },
  { key: 'state', title: '状态' },
  { key: 'ack', title: '操作', align: 'right' },
];

const visible = computed(() =>
  items.value.filter((alarm) => {
    if (levelFilter.value !== 'all' && alarm.level !== levelFilter.value) return false;
    if (stateFilter.value === 'active' && alarm.state !== 'active') return false;
    if (stateFilter.value === 'acked' && alarm.state !== 'acked') return false;
    return true;
  }),
);

const LEVEL_LABEL: Record<Alarm['level'], string> = {
  critical: '紧急',
  major: '重要',
  minor: '次要',
  warning: '警告',
  info: '提示',
};

const LEVEL_STATUS: Record<Alarm['level'], 'fault' | 'warning' | 'standby' | 'offline'> = {
  critical: 'fault',
  major: 'fault',
  minor: 'warning',
  warning: 'warning',
  info: 'standby',
};

async function load(): Promise<void> {
  loading.value = true;
  error.value = '';
  try {
    const result = await request<{ items: Alarm[] }>('/api/v1/alarms?limit=50');
    items.value = result.items;
  } catch (err) {
    error.value = err instanceof ApiRequestError ? `${err.code}: ${err.message}` : String(err);
  } finally {
    loading.value = false;
  }
}

async function ack(alarm: Alarm): Promise<void> {
  error.value = '';
  try {
    const updated = await request<Alarm>(`/api/v1/alarms/${alarm.id}/ack`, { method: 'POST', body: {} });
    upsert(updated);
  } catch (err) {
    error.value = err instanceof ApiRequestError ? `${err.code}: ${err.message}` : String(err);
  }
}

/** 同 id 覆盖、新的插到最前；顺带做一个短暂的"新到"高亮。 */
function upsert(alarm: Alarm): void {
  const index = items.value.findIndex((item) => item.id === alarm.id);
  if (index >= 0) {
    items.value = items.value.map((item) => (item.id === alarm.id ? alarm : item));
    return;
  }
  items.value = [alarm, ...items.value];
  flashing.value = [alarm.id, ...flashing.value].slice(0, 5);
  setTimeout(() => {
    flashing.value = flashing.value.filter((id) => id !== alarm.id);
  }, 4000);
}

function timeOf(ts: string): string {
  const date = new Date(ts);
  return Number.isNaN(date.getTime()) ? ts : date.toLocaleString('zh-CN', { hour12: false });
}

// 新告警靠推送到达；resync（重连后补状态）时整体重取一次。
useLiveFeed((message) => {
  if (message.type === 'resync') {
    void load();
    return;
  }
  if (message.topic !== 'alarm') return;
  upsert(message.payload as unknown as Alarm);
});

onMounted(load);
</script>

<template>
  <section>
    <header class="head">
      <div>
        <h1>告警与事件</h1>
        <p class="head__hint">
          新告警通过实时通道推送（无需刷新）；确认是写操作，受 core.alarm:write 约束。
        </p>
      </div>
      <div class="filters">
        <select v-model="levelFilter" class="filters__select" data-testid="alarm-level-filter" aria-label="按等级过滤">
          <option value="all">全部等级</option>
          <option value="critical">紧急</option>
          <option value="major">重要</option>
          <option value="minor">次要</option>
          <option value="info">提示</option>
        </select>
        <select v-model="stateFilter" class="filters__select" data-testid="alarm-state-filter" aria-label="按状态过滤">
          <option value="all">全部状态</option>
          <option value="active">未确认</option>
          <option value="acked">已确认</option>
        </select>
        <UiButton variant="ghost" size="sm" data-testid="alarm-refresh" @click="load">刷新</UiButton>
      </div>
    </header>

    <UiBanner v-if="error" tone="danger" title="操作失败" data-testid="alarm-error">{{ error }}</UiBanner>

    <UiDataTable
      :columns="columns"
      :rows="visible as unknown as Record<string, unknown>[]"
      row-key="id"
      :loading="loading"
      caption="告警列表"
      data-testid="alarms-table"
    >
      <template #cell-ts="{ row }">{{ timeOf(row.ts as string) }}</template>
      <template #cell-level="{ row }">
        <UiStatusBadge :status="LEVEL_STATUS[row.level as Alarm['level']]" :label="LEVEL_LABEL[row.level as Alarm['level']]" />
      </template>
      <template #cell-source="{ row }">
        <span class="mono">{{ row.source }}</span>
      </template>
      <template #cell-message="{ row }">
        <span :class="{ flash: flashing.includes(row.id as string) }" :data-testid="`alarm-message-${row.id}`">{{ row.message }}</span>
      </template>
      <template #cell-state="{ row }">
        <UiStatusBadge
          :status="row.state === 'active' ? 'warning' : row.state === 'acked' ? 'normal' : 'offline'"
          :label="row.state === 'active' ? '未确认' : row.state === 'acked' ? '已确认' : '已清除'"
        />
      </template>
      <template #cell-ack="{ row }">
        <UiButton
          v-if="canAck && row.state === 'active'"
          variant="secondary"
          size="sm"
          :data-testid="`alarm-ack-${row.id}`"
          @click="ack(row as unknown as Alarm)"
        >
          确认
        </UiButton>
        <span v-else-if="row.state !== 'active'" class="muted">—</span>
      </template>
    </UiDataTable>

    <p v-if="!loading && visible.length === 0" class="empty" data-testid="alarms-empty">当前过滤条件下没有告警</p>
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
.filters {
  display: flex;
  gap: var(--space-2);
  align-items: center;
}
.filters__select {
  min-height: var(--input-height);
  padding: 0 var(--space-2);
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
.flash {
  animation: flash 1.2s ease-in-out 2;
}
@keyframes flash {
  from {
    background: color-mix(in srgb, var(--color-warning) 24%, transparent);
  }
  to {
    background: transparent;
  }
}
.empty {
  margin-top: var(--space-3);
  color: var(--text-secondary);
  font-size: var(--font-size-sm);
}
</style>

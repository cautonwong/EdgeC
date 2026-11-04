<script setup lang="ts">
/**
 * 通知渠道（§8.3）。
 *
 * 只做 Webhook：邮件需要 SMTP 才能验证"真的发出去了"，台架上假发邮件等于自欺欺人。
 * 规则写清楚：只有**新产生的**告警才投递（归并进来的后续事件不重复推送），
 * 并且按最低等级过滤、跳过已静默的告警。
 */
import { computed, onMounted, ref } from 'vue';
import { ApiRequestError, request } from '../../sdk/http';
import { useAuthz } from '../../app/composables/useAuthz';
import UiBanner from '../../ui/components/Banner.vue';
import UiButton from '../../ui/components/Button.vue';
import UiDataTable, { type Column } from '../../ui/components/DataTable.vue';
import UiStatusBadge from '../../ui/components/StatusBadge.vue';

interface Channel {
  id: string;
  name: string;
  type: string;
  url: string;
  enabled: boolean;
  minLevel: 'info' | 'warning' | 'major' | 'critical';
  lastDeliveryAt: string | null;
  lastError: string | null;
}

interface Delivery {
  id: string;
  channelName: string;
  alarmId: string | null;
  code: string;
  level: string;
  ts: string;
  status: string;
  attempts?: number;
  httpStatus?: number | null;
  error?: string | null;
  note: string;
}

const authz = useAuthz();
const channels = ref<Channel[]>([]);
const deliveries = ref<Delivery[]>([]);
const loading = ref(false);
const error = ref('');
const notice = ref('');

const canEdit = computed(() => authz.can('core.alarm:write'));

const channelColumns: Column[] = [
  { key: 'name', title: '渠道' },
  { key: 'url', title: '地址' },
  { key: 'minLevel', title: '最低等级' },
  { key: 'enabled', title: '状态' },
  { key: 'lastDeliveryAt', title: '最近投递' },
  { key: 'actions', title: '操作', align: 'right' },
];

const deliveryColumns: Column[] = [
  { key: 'ts', title: '时间' },
  { key: 'channelName', title: '渠道' },
  { key: 'code', title: '告警码' },
  { key: 'level', title: '等级' },
  { key: 'status', title: '结果' },
  { key: 'attempts', title: '尝试' },
  { key: 'error', title: '原因' },
];

function describe(err: unknown): string {
  return err instanceof ApiRequestError ? `${err.code}: ${err.message}` : String(err);
}

function fmt(iso: string | null): string {
  if (!iso) return '—';
  const date = new Date(iso);
  return Number.isNaN(date.getTime()) ? iso : date.toLocaleString('zh-CN', { hour12: false });
}

async function load(): Promise<void> {
  loading.value = true;
  error.value = '';
  try {
    const [channelResult, deliveryResult] = await Promise.all([
      request<{ items: Channel[] }>('/api/v1/notifications/channels'),
      request<{ items: Delivery[] }>('/api/v1/notifications/deliveries'),
    ]);
    channels.value = channelResult.items;
    deliveries.value = deliveryResult.items;
  } catch (err) {
    error.value = describe(err);
  } finally {
    loading.value = false;
  }
}

async function toggle(channel: Channel): Promise<void> {
  error.value = '';
  notice.value = '';
  try {
    const updated = await request<Channel>(`/api/v1/notifications/channels/${channel.id}`, {
      method: 'PATCH',
      body: { enabled: !channel.enabled },
    });
    channels.value = channels.value.map((item) => (item.id === updated.id ? updated : item));
    notice.value = `${updated.name} 已${updated.enabled ? '启用' : '停用'}。`;
  } catch (err) {
    error.value = describe(err);
  }
}

async function changeLevel(channel: Channel, level: string): Promise<void> {
  error.value = '';
  notice.value = '';
  try {
    const updated = await request<Channel>(`/api/v1/notifications/channels/${channel.id}`, {
      method: 'PATCH',
      body: { minLevel: level },
    });
    channels.value = channels.value.map((item) => (item.id === updated.id ? updated : item));
    notice.value = `${updated.name} 的最低推送等级已改为 ${updated.minLevel}。`;
  } catch (err) {
    error.value = describe(err);
  }
}

async function test(channel: Channel): Promise<void> {
  error.value = '';
  notice.value = '';
  try {
    const result = await request<{ accepted?: boolean; status?: string; attempts?: number }>(
      `/api/v1/notifications/channels/${channel.id}/test`,
      { method: 'POST', body: {} },
    );
    if (result.accepted) {
      notice.value = `测试已提交（Webhook 异步投递，最多 ${result.attempts ?? 3} 次尝试），稍后看投递记录。`;
      // 真发 HTTP 可能要几百毫秒到几秒（含重试），延迟刷一次结果
      window.setTimeout(() => void load(), 1500);
    } else if (result.status === 'skipped') {
      await load();
      notice.value = `${channel.name} 未启用，测试没有发送。`;
    }
  } catch (err) {
    error.value = describe(err);
  }
}

onMounted(load);
</script>

<template>
  <section>
    <header class="head">
      <div>
        <h1>通知渠道</h1>
        <p class="head__hint">
          只有**新产生的**告警才投递（归并进来的后续事件不重复推送），按最低等级过滤，跳过已静默的告警。
        </p>
      </div>
      <UiButton variant="ghost" size="sm" data-testid="notify-refresh" @click="load">刷新</UiButton>
    </header>

    <UiBanner v-if="error" tone="danger" title="操作失败" data-testid="notify-error">{{ error }}</UiBanner>
    <UiBanner v-if="notice" tone="success" data-testid="notify-notice">{{ notice }}</UiBanner>

    <UiDataTable
      :columns="channelColumns"
      :rows="channels"
      row-key="id"
      :loading="loading"
      caption="通知渠道"
      data-testid="notify-channels"
    >
      <template #cell-url="{ row }"><span class="mono">{{ row.url }}</span></template>
      <template #cell-minLevel="{ row }">
        <select
          class="level"
          :data-testid="`notify-level-${row.id}`"
          :value="row.minLevel"
          :disabled="!canEdit"
          @change="changeLevel(row as unknown as Channel, ($event.target as HTMLSelectElement).value)"
        >
          <option value="info">info</option>
          <option value="warning">warning</option>
          <option value="major">major</option>
          <option value="critical">critical</option>
        </select>
      </template>
      <template #cell-enabled="{ row }">
        <UiStatusBadge :status="row.enabled ? 'normal' : 'offline'" :label="row.enabled ? '已启用' : '已停用'" />
      </template>
      <template #cell-lastDeliveryAt="{ row }">{{ fmt(row.lastDeliveryAt as string | null) }}</template>
      <template #cell-actions="{ row }">
        <div class="row-actions">
          <UiButton
            v-if="canEdit"
            variant="ghost"
            size="sm"
            :data-testid="`notify-toggle-${row.id}`"
            @click="toggle(row as unknown as Channel)"
          >
            {{ row.enabled ? '停用' : '启用' }}
          </UiButton>
          <UiButton
            v-if="canEdit"
            variant="ghost"
            size="sm"
            :data-testid="`notify-test-${row.id}`"
            @click="test(row as unknown as Channel)"
          >
            发测试
          </UiButton>
          <span v-if="!canEdit" class="muted">—</span>
        </div>
      </template>
    </UiDataTable>

    <h2 class="sub">最近投递</h2>
    <UiDataTable
      :columns="deliveryColumns"
      :rows="deliveries"
      row-key="id"
      :loading="loading"
      caption="投递记录"
      data-testid="notify-deliveries"
    >
      <template #cell-ts="{ row }">{{ fmt(row.ts as string) }}</template>
      <template #cell-status="{ row }">
        <UiStatusBadge
          :status="row.status === 'delivered' ? 'normal' : row.status === 'failed' ? 'warning' : 'standby'"
          :label="row.status as string"
        />
      </template>
      <template #cell-attempts="{ row }"><span class="mono">{{ row.attempts ?? '—' }}</span></template>
      <template #cell-error="{ row }"><span class="mono">{{ row.error ?? '—' }}</span></template>
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
.sub {
  margin: var(--space-6) 0 var(--space-3);
  font-size: var(--font-size-lg);
}
.level {
  min-height: var(--input-height);
  padding: 0 var(--space-2);
  border: 1px solid var(--border-default);
  border-radius: var(--radius-md);
  background: var(--bg-surface);
  color: var(--text-primary);
  font: inherit;
  font-size: var(--font-size-sm);
}
.row-actions {
  display: inline-flex;
  gap: var(--space-1);
  justify-content: flex-end;
}
.mono {
  font-family: var(--font-mono);
  font-size: var(--font-size-xs);
}
.muted {
  color: var(--text-disabled);
}
</style>

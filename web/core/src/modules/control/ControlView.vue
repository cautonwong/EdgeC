<script setup lang="ts">
/**
 * 远程控制（§8.4）。
 *
 * 拉合闸是不可撤销的物理动作，所以这里只有一条路：**发起 → 另一人复核 → 执行**。
 * 页面把"发起人不能自批"写在按钮上，但结论在服务端（self_approval_denied）；
 * 页面隐藏按钮只是省得人白点。
 */
import { computed, onMounted, ref } from 'vue';
import { ApiRequestError, request } from '../../sdk/http';
import { useAuthz } from '../../app/composables/useAuthz';
import UiBanner from '../../ui/components/Banner.vue';
import UiButton from '../../ui/components/Button.vue';
import UiDataTable, { type Column } from '../../ui/components/DataTable.vue';
import UiStatusBadge from '../../ui/components/StatusBadge.vue';

interface Device {
  id: string;
  name: string;
  location: string;
  state: 'open' | 'closed';
  ratedA: number;
}

interface Command {
  id: string;
  deviceId: string;
  deviceName: string;
  action: 'trip' | 'close';
  reason: string;
  state: 'PENDING_APPROVAL' | 'EXECUTED' | 'REJECTED' | 'EXPIRED';
  requestedBy: string;
  requestedAt: string;
  expiresAt: string;
  decidedBy: string | null;
  decidedAt: string | null;
  note: string | null;
}

const authz = useAuthz();
const devices = ref<Device[]>([]);
const commands = ref<Command[]>([]);
const loading = ref(false);
const error = ref('');
const notice = ref('');
const pendingAction = ref<{ device: Device; action: 'trip' | 'close' } | null>(null);
const reason = ref('');

const canRead = computed(() => authz.can('core.control:read'));
const canExecute = computed(() => authz.can('core.control:execute'));
const me = computed(() => authz.username.value);

const columns: Column[] = [
  { key: 'requestedAt', title: '时间' },
  { key: 'deviceName', title: '设备' },
  { key: 'action', title: '动作' },
  { key: 'reason', title: '理由' },
  { key: 'requestedBy', title: '发起人' },
  { key: 'state', title: '状态' },
  { key: 'actions', title: '复核', align: 'right' },
];

const STATE_LABEL: Record<Command['state'], string> = {
  PENDING_APPROVAL: '待复核',
  EXECUTED: '已执行',
  REJECTED: '已驳回',
  EXPIRED: '已超时',
};

const STATE_STATUS: Record<Command['state'], 'warning' | 'normal' | 'offline' | 'fault'> = {
  PENDING_APPROVAL: 'warning',
  EXECUTED: 'normal',
  REJECTED: 'offline',
  EXPIRED: 'fault',
};

function describe(err: unknown): string {
  return err instanceof ApiRequestError ? `${err.code}: ${err.message}` : String(err);
}

function fmt(iso: string | null): string {
  if (!iso) return '—';
  const date = new Date(iso);
  return Number.isNaN(date.getTime()) ? iso : date.toLocaleString('zh-CN', { hour12: false });
}

/** 用户名与 requestedBy(userId) 对不上，所以按"是不是我发起的"用角色+时间兜底判断不靠谱，
 *  这里直接比对服务端返回的 requestedBy 与当前会话的 userId（authz 只给 username）。 */
function isMine(command: Command): boolean {
  return command.requestedBy === authz.userId.value;
}

async function load(): Promise<void> {
  loading.value = true;
  error.value = '';
  try {
    const [deviceList, commandList] = await Promise.all([
      request<{ items: Device[] }>('/api/v1/control/devices'),
      request<{ items: Command[] }>('/api/v1/control/commands'),
    ]);
    devices.value = deviceList.items;
    commands.value = commandList.items;
  } catch (err) {
    error.value = describe(err);
  } finally {
    loading.value = false;
  }
}

function start(device: Device, action: 'trip' | 'close'): void {
  pendingAction.value = { device, action };
  reason.value = '';
  error.value = '';
  notice.value = '';
}

async function submit(): Promise<void> {
  if (!pendingAction.value) return;
  error.value = '';
  try {
    await request('/api/v1/control/commands', {
      method: 'POST',
      body: {
        deviceId: pendingAction.value.device.id,
        action: pendingAction.value.action,
        reason: reason.value.trim(),
      },
    });
    notice.value = '已提交，等待另一名有控制权限的账号复核后才会执行。';
    pendingAction.value = null;
    await load();
  } catch (err) {
    error.value = describe(err);
  }
}

async function decide(command: Command, approve: boolean): Promise<void> {
  error.value = '';
  notice.value = '';
  try {
    await request(`/api/v1/control/commands/${command.id}/${approve ? 'approve' : 'reject'}`, {
      method: 'POST',
      body: approve ? {} : { note: '复核人驳回' },
    });
    notice.value = approve ? `已执行 ${command.deviceName}。` : `已驳回 ${command.deviceName} 的命令。`;
    await load();
  } catch (err) {
    error.value = describe(err);
  }
}

onMounted(() => {
  if (!canRead.value) return;
  void load();
});
</script>

<template>
  <section>
    <header class="head">
      <div>
        <h1>远程控制</h1>
        <p class="head__hint">
          拉合闸不可撤销：发起后必须由**另一名**有控制权限的账号复核才会执行；发起人不能自批。
        </p>
      </div>
      <UiButton variant="ghost" size="sm" data-testid="control-refresh" @click="load">刷新</UiButton>
    </header>

    <UiBanner v-if="error" tone="danger" title="操作失败" data-testid="control-error">{{ error }}</UiBanner>
    <UiBanner v-if="notice" tone="success" data-testid="control-notice">{{ notice }}</UiBanner>

    <div class="grid" data-testid="control-devices">
      <article v-for="device in devices" :key="device.id" class="card" :data-testid="`device-${device.id}`">
        <header class="card__head">
          <div>
            <h2 class="card__title">{{ device.name }}</h2>
            <p class="card__sub">{{ device.location }} · 额定 {{ device.ratedA }}A</p>
          </div>
          <UiStatusBadge
            :status="device.state === 'closed' ? 'normal' : 'offline'"
            :label="device.state === 'closed' ? '合闸' : '分闸'"
          />
        </header>
        <div class="card__actions">
          <UiButton
            v-if="canExecute && device.state === 'closed'"
            variant="danger"
            size="sm"
            :data-testid="`trip-${device.id}`"
            @click="start(device, 'trip')"
          >
            远程拉闸
          </UiButton>
          <UiButton
            v-if="canExecute && device.state === 'open'"
            variant="secondary"
            size="sm"
            :data-testid="`close-${device.id}`"
            @click="start(device, 'close')"
          >
            远程合闸
          </UiButton>
          <span v-if="!canExecute" class="muted">只读（缺少 core.control:execute）</span>
        </div>
      </article>
    </div>

    <form v-if="pendingAction" class="dialog" data-testid="control-dialog" @submit.prevent="submit">
      <p class="dialog__title">
        确认对 <strong>{{ pendingAction.device.name }}</strong>
        执行<strong>{{ pendingAction.action === 'trip' ? '远程拉闸' : '远程合闸' }}</strong>？
      </p>
      <label class="field">
        <span class="field__label">操作理由（会进审计与告警）</span>
        <input v-model="reason" class="field__input" data-testid="control-reason" placeholder="例如：检修需要，工单 WO-2026-031" />
      </label>
      <div class="dialog__actions">
        <UiButton variant="ghost" size="sm" data-testid="control-cancel" @click="pendingAction = null">取消</UiButton>
        <UiButton variant="danger" size="sm" data-testid="control-submit" :disabled="reason.trim().length < 4" @click="submit">
          提交复核
        </UiButton>
      </div>
    </form>

    <UiDataTable :columns="columns" :rows="commands" row-key="id" :loading="loading" caption="控制命令" data-testid="control-table">
      <template #cell-requestedAt="{ row }">{{ fmt(row.requestedAt as string) }}</template>
      <template #cell-action="{ row }">
        <UiStatusBadge
          :status="row.action === 'trip' ? 'fault' : 'normal'"
          :label="row.action === 'trip' ? '拉闸' : '合闸'"
        />
      </template>
      <template #cell-requestedBy="{ row }">
        <span class="mono">{{ row.requestedBy }}</span>
      </template>
      <template #cell-state="{ row }">
        <UiStatusBadge :status="STATE_STATUS[row.state as Command['state']]" :label="STATE_LABEL[row.state as Command['state']]" />
      </template>
      <template #cell-actions="{ row }">
        <div v-if="canExecute && row.state === 'PENDING_APPROVAL' && !isMine(row as unknown as Command)" class="row-actions">
          <UiButton variant="danger" size="sm" :data-testid="`approve-${row.id}`" @click="decide(row as unknown as Command, true)">
            批准
          </UiButton>
          <UiButton variant="ghost" size="sm" :data-testid="`reject-${row.id}`" @click="decide(row as unknown as Command, false)">
            驳回
          </UiButton>
        </div>
        <span v-else-if="row.state === 'PENDING_APPROVAL'" class="muted">等待他人复核</span>
        <span v-else class="muted">—</span>
      </template>
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
.grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(260px, 1fr));
  gap: var(--space-3);
  margin-bottom: var(--space-4);
}
.card {
  padding: var(--space-4);
  background: var(--bg-surface);
  border: 1px solid var(--border-default);
  border-radius: var(--radius-md);
}
.card__head {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: var(--space-2);
}
.card__title {
  margin: 0;
  font-size: var(--font-size-lg);
}
.card__sub {
  margin: var(--space-1) 0 0;
  color: var(--text-secondary);
  font-size: var(--font-size-xs);
}
.card__actions {
  display: flex;
  align-items: center;
  gap: var(--space-2);
  margin-top: var(--space-3);
}
.dialog {
  padding: var(--space-4);
  margin-bottom: var(--space-4);
  background: var(--bg-surface);
  border: 1px solid var(--color-danger);
  border-radius: var(--radius-md);
}
.dialog__title {
  margin: 0 0 var(--space-3);
}
.dialog__actions {
  display: flex;
  justify-content: flex-end;
  gap: var(--space-2);
  margin-top: var(--space-3);
}
.field {
  display: flex;
  flex-direction: column;
  gap: var(--space-1);
}
.field__label {
  font-size: var(--font-size-sm);
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

<script setup lang="ts">
/**
 * 在线会话（§5.6）。
 *
 * 用途很具体：发现"有人在别处用同一个账号登录"时，能当场把那一条踢掉，
 * 而不是把整个账号禁用（那会连自己一起关在门外）。
 * 撤销后**下一次请求即失效**——服务端每请求校验会话，不做本地乐观假设。
 */
import { computed, onMounted, ref } from 'vue';
import { ApiRequestError, request } from '../../sdk/http';
import { useAuthz } from '../../app/composables/useAuthz';
import UiBanner from '../../ui/components/Banner.vue';
import UiButton from '../../ui/components/Button.vue';
import UiDataTable, { type Column } from '../../ui/components/DataTable.vue';
import UiStatusBadge from '../../ui/components/StatusBadge.vue';

interface SessionRow {
  sid: string;
  username: string;
  displayName: string;
  ip: string | null;
  userAgent: string;
  createdAt: string;
  lastSeenAt: string;
  current: boolean;
}

const authz = useAuthz();
const rows = ref<SessionRow[]>([]);
const loading = ref(false);
const error = ref('');
const notice = ref('');

const canAdmin = computed(() => authz.can('user.session:admin'));
const columns: Column[] = [
  { key: 'username', title: '账号' },
  { key: 'ip', title: '来源' },
  { key: 'client', title: '客户端' },
  { key: 'lastSeenAt', title: '最近活跃' },
  { key: 'createdAt', title: '登录时间' },
  { key: 'current', title: '当前' },
  { key: 'actions', title: '操作', align: 'right' },
];

function describe(err: unknown): string {
  return err instanceof ApiRequestError ? `${err.code}: ${err.message}` : String(err);
}

function fmt(iso: string): string {
  const date = new Date(iso);
  return Number.isNaN(date.getTime()) ? iso : date.toLocaleString('zh-CN', { hour12: false });
}

/** 从 UA 里挑出可读的客户端名，不整段甩给用户看。 */
function clientOf(ua: string): string {
  if (!ua) return '—';
  const browser = /Firefox\//.test(ua) ? 'Firefox' : /Edg\//.test(ua) ? 'Edge' : /Chrome\//.test(ua) ? 'Chrome' : '其它';
  const os = /Windows/.test(ua) ? 'Windows' : /Mac OS X/.test(ua) ? 'macOS' : /Linux/.test(ua) ? 'Linux' : '';
  return os ? `${browser} / ${os}` : browser;
}

async function load(): Promise<void> {
  loading.value = true;
  error.value = '';
  try {
    const result = await request<{ items: SessionRow[] }>('/api/v1/sessions');
    rows.value = result.items;
  } catch (err) {
    error.value = describe(err);
  } finally {
    loading.value = false;
  }
}

async function revoke(row: SessionRow): Promise<void> {
  error.value = '';
  notice.value = '';
  try {
    await request(`/api/v1/sessions/${row.sid}`, { method: 'DELETE' });
    notice.value = `已撤销 ${row.username} 的会话 ${row.sid}，下一次请求即失效。`;
    await load();
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
        <h1>在线会话</h1>
        <p class="head__hint">
          发现异常登录时，撤销那一条会话即可（比禁用账号安全：不会把自己也关在门外）。
        </p>
      </div>
      <div class="head__actions">
        <UiButton variant="ghost" size="sm" data-testid="sessions-refresh" @click="load">刷新</UiButton>
        <UiStatusBadge v-if="!canAdmin" status="unconfigured" label="只读（缺少 user.session:admin）" />
      </div>
    </header>

    <UiBanner v-if="error" tone="danger" title="操作失败" data-testid="sessions-error">{{ error }}</UiBanner>
    <UiBanner v-if="notice" tone="success" data-testid="sessions-notice">{{ notice }}</UiBanner>

    <UiDataTable :columns="columns" :rows="rows" row-key="sid" :loading="loading" caption="会话列表" data-testid="sessions-table">
      <template #cell-username="{ row }">
        {{ row.username }}
        <span v-if="row.displayName" class="muted">（{{ row.displayName }}）</span>
      </template>
      <template #cell-ip="{ row }">
        <span class="mono">{{ row.ip ?? '—' }}</span>
      </template>
      <template #cell-client="{ row }">{{ clientOf(row.userAgent as string) }}</template>
      <template #cell-lastSeenAt="{ row }">{{ fmt(row.lastSeenAt as string) }}</template>
      <template #cell-createdAt="{ row }">{{ fmt(row.createdAt as string) }}</template>
      <template #cell-current="{ row }">
        <UiStatusBadge v-if="row.current" status="normal" label="当前会话" />
        <span v-else class="muted">—</span>
      </template>
      <template #cell-actions="{ row }">
        <UiButton
          v-if="canAdmin && !row.current"
          variant="ghost"
          size="sm"
          :data-testid="`revoke-${row.sid}`"
          @click="revoke(row as unknown as SessionRow)"
        >
          撤销
        </UiButton>
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
  max-width: 720px;
  color: var(--text-secondary);
  font-size: var(--font-size-sm);
}
.head__actions {
  display: flex;
  align-items: center;
  gap: var(--space-2);
}
.mono {
  font-family: var(--font-mono);
  font-size: var(--font-size-xs);
}
.muted {
  color: var(--text-disabled);
}
</style>

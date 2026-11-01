<script setup lang="ts">
/**
 * 用户与角色（§5.3 / §5.5）。
 * "新建用户"按钮受 user.account:admin 控制——但这只是体验，POST 仍由服务端判定；
 * 页面故意把"我有哪些角色"也显示出来，方便现场核对授权合成结果。
 */
import { onMounted, ref } from 'vue';
import { request, ApiRequestError } from '../../sdk/http';
import { useAuthz } from '../../app/composables/useAuthz';
import UiDataTable, { type Column } from '../../ui/components/DataTable.vue';
import UiStatusBadge from '../../ui/components/StatusBadge.vue';
import UiBanner from '../../ui/components/Banner.vue';
import UiButton from '../../ui/components/Button.vue';

interface UserRow {
  id: string;
  username: string;
  displayName: string;
  roles: string[];
  status: 'active' | 'disabled';
  mustChangePassword: boolean;
  lastLogin: string | null;
}

const authz = useAuthz();
const rows = ref<UserRow[]>([]);
const loading = ref(false);
const error = ref('');
const notice = ref('');

const columns: Column[] = [
  { key: 'username', title: '用户名' },
  { key: 'displayName', title: '显示名' },
  { key: 'roles', title: '角色' },
  { key: 'status', title: '状态' },
  { key: 'mustChangePassword', title: '待改密' },
  { key: 'lastLogin', title: '最近登录' },
];

async function load(): Promise<void> {
  loading.value = true;
  error.value = '';
  try {
    const result = await request<{ items: UserRow[] }>('/api/v1/users');
    rows.value = result.items;
  } catch (err) {
    error.value = err instanceof ApiRequestError ? `${err.code}: ${err.message}` : String(err);
  } finally {
    loading.value = false;
  }
}

async function createUser(): Promise<void> {
  notice.value = '';
  error.value = '';
  try {
    await request('/api/v1/users', {
      method: 'POST',
      body: { username: `operator-${rows.value.length + 1}`, roleIds: ['viewer'] },
    });
    notice.value = '已创建（初始口令需由设备唯一口令流程交付）';
    await load();
  } catch (err) {
    error.value = err instanceof ApiRequestError ? `${err.code}: ${err.message}` : String(err);
  }
}

onMounted(load);
</script>

<template>
  <section>
    <header class="head">
      <div>
        <h1>用户与角色</h1>
        <p class="head__hint">
          当前身份：{{ authz.username.value }}（{{ authz.roles.value.join(' / ') || '无角色' }}）
        </p>
      </div>
      <UiButton
        v-if="authz.can('user.account:admin')"
        variant="primary"
        data-testid="create-user"
        @click="createUser"
      >
        新建用户
      </UiButton>
      <span v-else class="head__readonly" data-testid="no-admin">只读（缺少 user.account:admin）</span>
    </header>

    <UiBanner v-if="error" tone="danger" title="操作失败" data-testid="users-error">{{ error }}</UiBanner>
    <UiBanner v-if="notice" tone="success">{{ notice }}</UiBanner>

    <UiDataTable :columns="columns" :rows="rows" row-key="id" :loading="loading" caption="账号列表">
      <template #cell-roles="{ row }">{{ (row.roles as string[]).join(', ') }}</template>
      <template #cell-status="{ row }">
        <UiStatusBadge :status="row.status === 'active' ? 'normal' : 'offline'" :label="row.status === 'active' ? '启用' : '禁用'" />
      </template>
      <template #cell-mustChangePassword="{ row }">{{ row.mustChangePassword ? '是' : '否' }}</template>
      <template #cell-lastLogin="{ row }">{{ row.lastLogin ?? '—' }}</template>
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
  color: var(--text-secondary);
  font-size: var(--font-size-sm);
}
.head__readonly {
  color: var(--color-offline);
  font-size: var(--font-size-xs);
}
</style>

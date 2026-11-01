<script setup lang="ts">
/**
 * 用户管理（§5.5）。
 *
 * 这一页的每条动作都有"把自己锁在门外"的可能，所以三条不变量写在这里也写在服务端：
 *   1. 不能禁用/删除自己（self_protected）；
 *   2. 不能禁用/降级最后一个可用管理员（last_admin）；
 *   3. 重置口令会踢掉该用户全部会话，并强制下次登录改密。
 * 前端只做提前提示，服务端才是结论。
 */
import { computed, onMounted, ref } from 'vue';
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

interface RoleRow {
  id: string;
  name: string;
  description: string;
  builtin: boolean;
  permissions: string[];
}

const authz = useAuthz();
const rows = ref<UserRow[]>([]);
const roles = ref<RoleRow[]>([]);
const loading = ref(false);
const error = ref('');
const notice = ref('');
const oneTimePassword = ref<{ username: string; password: string } | null>(null);
const creating = ref(false);
const form = ref({ username: '', displayName: '', roleIds: ['viewer'] as string[] });

const canAdmin = computed(() => authz.can('user.account:admin'));
const columns: Column[] = [
  { key: 'username', title: '账号' },
  { key: 'displayName', title: '显示名' },
  { key: 'roles', title: '角色' },
  { key: 'status', title: '状态' },
  { key: 'mustChangePassword', title: '待改密' },
  { key: 'lastLogin', title: '最近登录' },
  { key: 'actions', title: '操作', align: 'right' },
];

function describe(err: unknown): string {
  return err instanceof ApiRequestError ? `${err.code}: ${err.message}` : String(err);
}

async function load(): Promise<void> {
  loading.value = true;
  error.value = '';
  try {
    const [users, roleList] = await Promise.all([
      request<{ items: UserRow[] }>('/api/v1/users'),
      request<{ items: RoleRow[] }>('/api/v1/roles'),
    ]);
    rows.value = users.items;
    roles.value = roleList.items;
  } catch (err) {
    error.value = describe(err);
  } finally {
    loading.value = false;
  }
}

async function createUser(): Promise<void> {
  notice.value = '';
  oneTimePassword.value = null;
  error.value = '';
  try {
    const created = await request<{ username: string; initialPassword?: string }>('/api/v1/users', {
      method: 'POST',
      body: {
        username: form.value.username.trim(),
        displayName: form.value.displayName.trim() || undefined,
        roleIds: form.value.roleIds,
      },
    });
    if (created.initialPassword) {
      oneTimePassword.value = { username: created.username, password: created.initialPassword };
    }
    notice.value = `已创建 ${created.username}，首次登录必须改密。`;
    creating.value = false;
    form.value = { username: '', displayName: '', roleIds: ['viewer'] };
    await load();
  } catch (err) {
    error.value = describe(err);
  }
}

async function patch(user: UserRow, body: Record<string, unknown>): Promise<void> {
  error.value = '';
  notice.value = '';
  try {
    const result = await request<{ resetPassword?: string }>(`/api/v1/users/${user.id}`, { method: 'PATCH', body });
    if (result.resetPassword) {
      oneTimePassword.value = { username: user.username, password: result.resetPassword };
      notice.value = `已重置 ${user.username} 的口令，该用户已有会话全部失效。`;
    } else {
      notice.value = `已更新 ${user.username}。`;
    }
    await load();
  } catch (err) {
    error.value = describe(err);
  }
}

async function remove(user: UserRow): Promise<void> {
  error.value = '';
  notice.value = '';
  try {
    await request(`/api/v1/users/${user.id}`, { method: 'DELETE' });
    notice.value = `已删除 ${user.username}。`;
    await load();
  } catch (err) {
    error.value = describe(err);
  }
}

function toggleRole(roleId: string, checked: boolean): void {
  const set = new Set(form.value.roleIds);
  if (checked) set.add(roleId);
  else set.delete(roleId);
  form.value.roleIds = [...set];
}

function roleName(roleId: string): string {
  return roles.value.find((role) => role.id === roleId)?.name ?? roleId;
}

onMounted(load);
</script>

<template>
  <section>
    <header class="head">
      <div>
        <h1>用户管理</h1>
        <p class="head__hint">
          当前身份：{{ authz.username.value }}（{{ authz.roles.value.map(roleName).join(' / ') || '无角色' }}）
        </p>
      </div>
      <UiButton
        v-if="canAdmin"
        variant="primary"
        data-testid="create-user"
        @click="creating = !creating"
      >
        {{ creating ? '取消' : '新建用户' }}
      </UiButton>
      <span v-else class="head__readonly" data-testid="no-admin">只读（缺少 user.account:admin）</span>
    </header>

    <UiBanner v-if="error" tone="danger" title="操作失败" data-testid="users-error">{{ error }}</UiBanner>
    <UiBanner v-if="notice" tone="success" data-testid="users-notice">{{ notice }}</UiBanner>

    <UiBanner v-if="oneTimePassword" tone="warning" title="一次性口令（只显示这一次）" data-testid="one-time-password">
      <p>
        账号 <strong>{{ oneTimePassword.username }}</strong> 的口令：
        <code class="secret">{{ oneTimePassword.password }}</code>
      </p>
      <p class="hint">请当面/带外交付。该用户首次登录必须改密；重置口令已使其旧会话全部失效。</p>
    </UiBanner>

    <form v-if="creating" class="create" data-testid="create-user-form" @submit.prevent="createUser">
      <label class="field">
        <span class="field__label">账号</span>
        <input v-model="form.username" class="field__input" data-testid="new-username" placeholder="小写字母/数字/._-" />
      </label>
      <label class="field">
        <span class="field__label">显示名</span>
        <input v-model="form.displayName" class="field__input" data-testid="new-display-name" />
      </label>
      <fieldset class="field roles">
        <legend class="field__label">角色</legend>
        <label v-for="role in roles" :key="role.id" class="roles__item">
          <input
            type="checkbox"
            :checked="form.roleIds.includes(role.id)"
            :data-testid="`new-role-${role.id}`"
            @change="toggleRole(role.id, ($event.target as HTMLInputElement).checked)"
          />
          <span>{{ role.name }}</span>
        </label>
      </fieldset>
      <UiButton variant="primary" size="sm" data-testid="create-user-submit" :disabled="!form.username" @click="createUser">创建</UiButton>
    </form>

    <UiDataTable :columns="columns" :rows="rows" row-key="id" :loading="loading" caption="账号列表" data-testid="users-table">
      <template #cell-roles="{ row }">{{ (row.roles as string[]).map(roleName).join(', ') }}</template>
      <template #cell-status="{ row }">
        <UiStatusBadge :status="row.status === 'active' ? 'normal' : 'offline'" :label="row.status === 'active' ? '启用' : '禁用'" />
      </template>
      <template #cell-mustChangePassword="{ row }">{{ row.mustChangePassword ? '是' : '否' }}</template>
      <template #cell-lastLogin="{ row }">{{ row.lastLogin ?? '从未登录' }}</template>
      <template #cell-actions="{ row }">
        <div v-if="canAdmin" class="row-actions">
          <UiButton
            variant="ghost"
            size="sm"
            :data-testid="`reset-${row.username}`"
            @click="patch(row as unknown as UserRow, { resetPassword: true })"
          >
            重置口令
          </UiButton>
          <UiButton
            v-if="row.status === 'active'"
            variant="ghost"
            size="sm"
            :data-testid="`disable-${row.username}`"
            @click="patch(row as unknown as UserRow, { status: 'disabled' })"
          >
            禁用
          </UiButton>
          <UiButton
            v-else
            variant="ghost"
            size="sm"
            :data-testid="`enable-${row.username}`"
            @click="patch(row as unknown as UserRow, { status: 'active' })"
          >
            启用
          </UiButton>
          <UiButton
            variant="ghost"
            size="sm"
            :data-testid="`delete-${row.username}`"
            @click="remove(row as unknown as UserRow)"
          >
            删除
          </UiButton>
        </div>
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
  color: var(--text-secondary);
  font-size: var(--font-size-sm);
}
.head__readonly {
  color: var(--text-secondary);
  font-size: var(--font-size-sm);
}
.create {
  display: flex;
  flex-wrap: wrap;
  gap: var(--space-3);
  align-items: flex-end;
  padding: var(--space-4);
  margin-bottom: var(--space-4);
  background: var(--bg-surface);
  border: 1px solid var(--border-default);
  border-radius: var(--radius-md);
}
.field {
  display: flex;
  flex-direction: column;
  gap: var(--space-1);
  border: none;
  padding: 0;
  margin: 0;
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
.roles {
  flex-direction: row;
  flex-wrap: wrap;
  gap: var(--space-3);
  align-items: center;
}
.roles__item {
  display: inline-flex;
  align-items: center;
  gap: 4px;
  font-size: var(--font-size-sm);
}
.row-actions {
  display: inline-flex;
  gap: var(--space-1);
  justify-content: flex-end;
}
.secret {
  font-family: var(--font-mono);
  padding: 1px 6px;
  border-radius: var(--radius-sm);
  background: var(--bg-elevated);
}
.hint {
  margin: var(--space-1) 0 0;
  font-size: var(--font-size-xs);
  color: var(--text-secondary);
}
.muted {
  color: var(--text-disabled);
}
</style>

<script setup lang="ts">
/**
 * 角色管理（§5.5）。
 *
 * 内置角色只读（它们的权限来自构建期的 role-presets，改了要发版）；
 * 自定义角色在设备上创建，权限必须是**具体 ID** —— 通配符只活在构建输入，
 * 服务端会逐条校验"目录里存在 + 不超过自己的权限"（禁止提权）。
 */
import { computed, onMounted, ref } from 'vue';
import { request, ApiRequestError } from '../../sdk/http';
import { useAuthz } from '../../app/composables/useAuthz';
import UiBanner from '../../ui/components/Banner.vue';
import UiButton from '../../ui/components/Button.vue';
import UiDataTable, { type Column } from '../../ui/components/DataTable.vue';
import UiStatusBadge from '../../ui/components/StatusBadge.vue';

interface RoleRow {
  id: string;
  name: string;
  description: string;
  builtin: boolean;
  permissions: string[];
  permissionCount?: number;
}

const authz = useAuthz();

const roles = ref<RoleRow[]>([]);
const loading = ref(false);
const error = ref('');
const notice = ref('');
const creating = ref(false);
const draft = ref({ id: '', name: '', description: '', permissions: [] as string[] });

const canAdmin = computed(() => authz.can('user.role:admin'));
const catalog = computed(() => authz.registry.value?.permissions ?? []);
const granted = computed(() => authz.granted.value);

/** 按域分组：core / network / system / user / 产品域 */
const groups = computed(() => {
  const map = new Map<string, { id: string; description: string; dependencies: string[] }[]>();
  for (const item of catalog.value) {
    const domain = item.id.split('.')[0].split(':')[0];
    if (!map.has(domain)) map.set(domain, []);
    map.get(domain)!.push(item);
  }
  return [...map.entries()].sort((a, b) => a[0].localeCompare(b[0]));
});

const columns: Column[] = [
  { key: 'name', title: '角色' },
  { key: 'id', title: 'ID' },
  { key: 'description', title: '职责' },
  { key: 'builtin', title: '类型' },
  { key: 'permissionCount', title: '权限数', align: 'right' },
];

function describe(err: unknown): string {
  return err instanceof ApiRequestError ? `${err.code}: ${err.message}` : String(err);
}

async function load(): Promise<void> {
  loading.value = true;
  error.value = '';
  try {
    const result = await request<{ items: RoleRow[] }>('/api/v1/roles');
    roles.value = result.items;
  } catch (err) {
    error.value = describe(err);
  } finally {
    loading.value = false;
  }
}

function toggle(permissionId: string, checked: boolean): void {
  const set = new Set(draft.value.permissions);
  if (checked) set.add(permissionId);
  else set.delete(permissionId);
  draft.value.permissions = [...set].sort();
}

async function createRole(): Promise<void> {
  error.value = '';
  notice.value = '';
  try {
    await request('/api/v1/roles', {
      method: 'POST',
      body: {
        id: draft.value.id.trim(),
        name: draft.value.name.trim(),
        description: draft.value.description.trim(),
        permissions: draft.value.permissions,
      },
    });
    notice.value = `已创建角色 ${draft.value.name}。`;
    creating.value = false;
    draft.value = { id: '', name: '', description: '', permissions: [] };
    await load();
  } catch (err) {
    error.value = describe(err);
  }
}

/** 提权提示：勾了自己没有的权限，服务端会拒（grant_escalation），这里先标红。 */
function isEscalation(permissionId: string): boolean {
  return !granted.value.has(permissionId);
}

onMounted(load);
</script>

<template>
  <section>
    <header class="head">
      <div>
        <h1>角色管理</h1>
        <p class="head__hint">
          内置角色的权限来自构建期 role-presets（改要发版）；自定义角色只能在目录里挑具体权限，
          且不能超过你自己的权限。
        </p>
      </div>
      <UiButton v-if="canAdmin" variant="primary" data-testid="create-role" @click="creating = !creating">
        {{ creating ? '取消' : '新建角色' }}
      </UiButton>
      <span v-else class="head__readonly" data-testid="no-role-admin">只读（缺少 user.role:admin）</span>
    </header>

    <UiBanner v-if="error" tone="danger" title="操作失败" data-testid="roles-error">{{ error }}</UiBanner>
    <UiBanner v-if="notice" tone="success" data-testid="roles-notice">{{ notice }}</UiBanner>

    <form v-if="creating" class="create" data-testid="create-role-form" @submit.prevent="createRole">
      <div class="create__row">
        <label class="field">
          <span class="field__label">角色 ID</span>
          <input v-model="draft.id" class="field__input" data-testid="role-id" placeholder="小写字母开头" />
        </label>
        <label class="field">
          <span class="field__label">名称</span>
          <input v-model="draft.name" class="field__input" data-testid="role-name" />
        </label>
        <label class="field field--wide">
          <span class="field__label">职责说明</span>
          <input v-model="draft.description" class="field__input" data-testid="role-description" />
        </label>
      </div>

      <div class="picker" data-testid="role-permission-picker">
        <section v-for="[domain, items] in groups" :key="domain" class="picker__group">
          <h3 class="picker__title">{{ domain }}</h3>
          <label v-for="item in items" :key="item.id" class="picker__item" :class="{ 'picker__item--warn': isEscalation(item.id) }">
            <input
              type="checkbox"
              :checked="draft.permissions.includes(item.id)"
              :data-testid="`perm-${item.id}`"
              @change="toggle(item.id, ($event.target as HTMLInputElement).checked)"
            />
            <span class="picker__id">{{ item.id }}</span>
            <span class="picker__desc">{{ item.description }}</span>
          </label>
        </section>
      </div>

      <div class="create__actions">
        <span class="hint">已选 {{ draft.permissions.length }} 条；标黄的是你没有的权限（服务端会拒）。</span>
        <UiButton
          variant="primary"
          size="sm"
          data-testid="create-role-submit"
          :disabled="!draft.id || !draft.name || draft.permissions.length === 0"
          @click="createRole"
        >
          创建
        </UiButton>
      </div>
    </form>

    <UiDataTable :columns="columns" :rows="roles" row-key="id" :loading="loading" caption="角色列表" data-testid="roles-table">
      <template #cell-builtin="{ row }">
        <UiStatusBadge :status="row.builtin ? 'standby' : 'normal'" :label="row.builtin ? '内置' : '自定义'" />
      </template>
      <template #cell-description="{ row }">{{ row.description || '—' }}</template>
      <template #cell-permissionCount="{ row }">{{ row.permissions.length }}</template>
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
.head__readonly {
  color: var(--text-secondary);
  font-size: var(--font-size-sm);
}
.create {
  padding: var(--space-4);
  margin-bottom: var(--space-4);
  background: var(--bg-surface);
  border: 1px solid var(--border-default);
  border-radius: var(--radius-md);
}
.create__row {
  display: flex;
  flex-wrap: wrap;
  gap: var(--space-3);
}
.create__actions {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-top: var(--space-3);
}
.field {
  display: flex;
  flex-direction: column;
  gap: var(--space-1);
}
.field--wide {
  flex: 1;
  min-width: 240px;
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
.picker {
  max-height: 320px;
  overflow: auto;
  margin-top: var(--space-3);
  padding: var(--space-2) var(--space-3);
  border: 1px solid var(--border-default);
  border-radius: var(--radius-md);
}
.picker__group + .picker__group {
  margin-top: var(--space-2);
}
.picker__title {
  margin: var(--space-2) 0 var(--space-1);
  font-size: var(--font-size-sm);
  color: var(--text-secondary);
  text-transform: uppercase;
  letter-spacing: 0.04em;
}
.picker__item {
  display: grid;
  grid-template-columns: 18px 260px 1fr;
  gap: var(--space-2);
  align-items: center;
  padding: 2px 0;
  font-size: var(--font-size-sm);
}
.picker__item--warn {
  color: var(--color-warning);
}
.picker__id {
  font-family: var(--font-mono);
  font-size: var(--font-size-xs);
}
.picker__desc {
  color: var(--text-secondary);
}
.hint {
  font-size: var(--font-size-xs);
  color: var(--text-secondary);
}
</style>

<script setup lang="ts">
/**
 * 防火墙策略（R3：安全策略，应用后必须确认）。
 *
 * 这一页最容易把自己关在门外：把 defaultPolicy 改成 deny 又清空白名单，管理面立刻不可达。
 * 所以除了确认窗口，还把"白名单为空 + 默认拒绝"这条组合单独提示出来。
 */
import { computed, onBeforeUnmount, onMounted, ref } from 'vue';
import { ApiRequestError } from '../../sdk/http';
import { useAuthz } from '../../app/composables/useAuthz';
import { useConfigSection } from '../../app/composables/useConfigSection';
import ConfigTransactionBar from '../../app/components/ConfigTransactionBar.vue';
import UiBanner from '../../ui/components/Banner.vue';
import UiSchemaForm from '../../ui/components/SchemaForm.vue';
import UiStatusBadge from '../../ui/components/StatusBadge.vue';

const authz = useAuthz();
const { tx, schema, load } = useConfigSection('network.firewall');
const error = ref('');
const loading = ref(true);

const canWrite = authz.can('network.firewall:write');

/** 会把自己锁在门外的组合：默认拒绝 + 白名单为空。 */
const lockoutRisk = computed(() => {
  const draft = tx.draft as { defaultPolicy?: string; allowSources?: unknown[] };
  return draft?.defaultPolicy === 'deny' && (draft.allowSources?.length ?? 0) === 0;
});

onMounted(async () => {
  try {
    await load();
  } catch (err) {
    error.value = err instanceof ApiRequestError ? `${err.code}: ${err.message}` : String(err);
  } finally {
    loading.value = false;
  }
});

onBeforeUnmount(() => tx.stopTicker());
</script>

<template>
  <section>
    <header class="head">
      <div>
        <h1>防火墙策略</h1>
        <p class="head__hint">
          风险等级 <strong>{{ schema?.riskClass ?? 'R3' }}</strong>：应用后必须在
          {{ schema?.confirmWindowSeconds ?? 60 }} 秒内确认，否则设备回滚。
        </p>
      </div>
      <UiStatusBadge v-if="!canWrite" status="unconfigured" label="只读（缺少 network.firewall:write）" />
    </header>

    <UiBanner v-if="error" tone="danger" title="读取失败" data-testid="firewall-error">{{ error }}</UiBanner>
    <p v-else-if="loading" class="loading">加载中…</p>

    <template v-else-if="schema">
      <UiBanner v-if="lockoutRisk" tone="danger" title="高风险组合" data-testid="lockout-warning">
        默认策略为拒绝且白名单为空——应用后管理面可能立刻不可达。请先加入管理网段。
      </UiBanner>
      <ConfigTransactionBar />
      <UiSchemaForm
        :schema="schema"
        :model-value="tx.draft"
        :errors="tx.fieldErrors"
        :disabled="!canWrite"
        @change="tx.setValue"
      />
    </template>
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
.loading {
  color: var(--text-secondary);
}
</style>

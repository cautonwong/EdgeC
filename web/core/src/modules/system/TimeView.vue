<script setup lang="ts">
/**
 * 系统时间（R1：服务中断，应用即生效，不弹确认窗口）。
 *
 * 和网络接口页共用同一套机制，差别只来自 Schema 的 riskClass —— 页面里没有任何
 * "if 这是 R1" 的分支，风险等级由 Schema 与事务条决定。
 */
import { onBeforeUnmount, onMounted, ref } from 'vue';
import { ApiRequestError } from '../../sdk/http';
import { useAuthz } from '../../app/composables/useAuthz';
import { useConfigSection } from '../../app/composables/useConfigSection';
import ConfigTransactionBar from '../../app/components/ConfigTransactionBar.vue';
import UiBanner from '../../ui/components/Banner.vue';
import UiSchemaForm from '../../ui/components/SchemaForm.vue';
import UiStatusBadge from '../../ui/components/StatusBadge.vue';

const authz = useAuthz();
const { tx, schema, load } = useConfigSection('system.time');
const error = ref('');
const loading = ref(true);

const canWrite = authz.can('system.time:write');

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
        <h1>系统时间</h1>
        <p class="head__hint">
          风险等级 <strong>{{ schema?.riskClass ?? 'R1' }}</strong>：应用后立即生效，没有确认窗口。
          NTP 不可达时会影响采集时间戳，请优先保证内网时间源可用。
        </p>
      </div>
      <UiStatusBadge v-if="!canWrite" status="unconfigured" label="只读（缺少 system.time:write）" />
    </header>

    <UiBanner v-if="error" tone="danger" title="读取失败" data-testid="time-error">{{ error }}</UiBanner>
    <p v-else-if="loading" class="loading">加载中…</p>

    <template v-else-if="schema">
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

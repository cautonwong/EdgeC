<script setup lang="ts">
/**
 * 蜂窝链路（§4.5 能力驱动的典型页面）。
 * 页面声明了 `requires.capabilities: [hardware.network.cellular]`，设备没有该模块时
 * 菜单不出现、直接输入 URL 会落到"当前设备不支持"页；服务端同样会拒绝该 API。
 */
import { onMounted, ref } from 'vue';
import { request, ApiRequestError } from '../../sdk/http';
import UiMetricCard from '../../ui/components/MetricCard.vue';
import UiStatusBadge from '../../ui/components/StatusBadge.vue';
import UiBanner from '../../ui/components/Banner.vue';

interface CellularState {
  state: 'connected' | 'disconnected' | 'searching';
  operator: string;
  technology: string;
  signalDbm: number;
  rxBytes: number;
  txBytes: number;
  updatedAt: string;
}

const state = ref<CellularState | null>(null);
const loading = ref(false);
const error = ref('');

onMounted(async () => {
  loading.value = true;
  try {
    state.value = await request<CellularState>('/api/v1/network/cellular');
  } catch (err) {
    error.value = err instanceof ApiRequestError ? `${err.code}: ${err.message}` : String(err);
  } finally {
    loading.value = false;
  }
});
</script>

<template>
  <section>
    <header class="head">
      <h1>蜂窝链路</h1>
      <p class="head__hint">能力缺失时本页不会出现在菜单里；服务端也会拒绝该接口。</p>
    </header>

    <UiBanner v-if="error" tone="danger" title="读取失败">{{ error }}</UiBanner>

    <div v-if="state" class="grid">
      <UiMetricCard title="链路状态" :value="state.state" :updated-at="state.updatedAt">
        <template #status>
          <UiStatusBadge :status="state.state === 'connected' ? 'normal' : 'offline'" />
        </template>
      </UiMetricCard>
      <UiMetricCard title="运营商" :value="state.operator" :hint="state.technology" />
      <UiMetricCard title="信号强度" :value="state.signalDbm" unit="dBm" />
      <UiMetricCard title="流量" :value="`${(state.rxBytes / 1048576).toFixed(1)} / ${(state.txBytes / 1048576).toFixed(1)}`" unit="MB 收/发" />
    </div>
    <p v-else-if="loading" class="loading">加载中…</p>
  </section>
</template>

<style scoped>
.head {
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
.grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(220px, 1fr));
  gap: var(--space-4);
}
.loading {
  color: var(--text-secondary);
}
</style>

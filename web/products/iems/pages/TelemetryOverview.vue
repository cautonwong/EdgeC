<script setup lang="ts">
/** iEMS 能流概览：只组合 core-ui（Pack 不得引入未经批准的第三方 UI 库）。 */
import { onMounted, ref } from 'vue';
import { request, ApiRequestError } from '@edgeos/web-core/sdk';
import { UiMetricCard, UiBanner, UiStatusBadge } from '@edgeos/web-core/ui';

interface Telemetry {
  soc: number;
  pvPowerKw: number;
  gridPowerKw: number;
  loadPowerKw: number;
  updatedAt: string;
}

const data = ref<Telemetry | null>(null);
const error = ref('');

onMounted(async () => {
  try {
    data.value = await request<Telemetry>('/api/v1/iems/telemetry');
  } catch (err) {
    error.value = err instanceof ApiRequestError ? `${err.code}: ${err.message}` : String(err);
  }
});
</script>

<template>
  <section>
    <header class="head">
      <h1>能流概览</h1>
      <p class="head__hint">数据来自 iEMS Pack 扩展接口（/api/v1/iems/telemetry），受 iems.telemetry:read 与设备能力双重约束。</p>
    </header>

    <UiBanner v-if="error" tone="danger" title="读取失败">{{ error }}</UiBanner>

    <div v-if="data" class="grid" data-testid="iems-telemetry">
      <UiMetricCard title="电池 SOC" :value="data.soc" unit="%" :updated-at="data.updatedAt" />
      <UiMetricCard title="光伏出力" :value="data.pvPowerKw" unit="kW" />
      <UiMetricCard title="电网交换" :value="data.gridPowerKw" unit="kW" hint="负值表示上网" />
      <UiMetricCard title="负荷" :value="data.loadPowerKw" unit="kW">
        <template #status><UiStatusBadge status="normal" label="运行中" /></template>
      </UiMetricCard>
    </div>
    <p v-else class="loading">加载中…</p>
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

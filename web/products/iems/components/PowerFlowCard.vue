<script setup lang="ts">
/** iEMS 能流卡片（Dashboard Widget 示例）。Widget 同样受权限与能力约束。 */
import { UiMetricCard } from '@edgeos/web-core/ui';

defineProps<{ soc: number; pvPowerKw: number; loadPowerKw: number }>();
</script>

<template>
  <div class="power-flow">
    <UiMetricCard title="光伏 → 负荷" :value="pvPowerKw" unit="kW" hint="能流卡片（Pack Widget）" />
    <UiMetricCard title="电池 SOC" :value="soc" unit="%" />
    <UiMetricCard title="负荷" :value="loadPowerKw" unit="kW" />
  </div>
</template>

<style scoped>
.power-flow {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(160px, 1fr));
  gap: var(--space-3);
}
</style>

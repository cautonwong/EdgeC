<script setup lang="ts">
/** L2 指标卡：数据过期时置灰并标注更新时间，避免"停更的数值看着像实时的"（§9.3）。 */
withDefaults(
  defineProps<{
    title: string;
    value: string | number;
    unit?: string;
    hint?: string;
    updatedAt?: string;
    stale?: boolean;
  }>(),
  { stale: false },
);
</script>

<template>
  <section class="metric" :class="{ 'metric--stale': stale }" :aria-busy="stale || undefined">
    <header class="metric__head">
      <span class="metric__title">{{ title }}</span>
      <slot name="status" />
    </header>
    <p class="metric__value">
      <span class="metric__number">{{ value }}</span>
      <span v-if="unit" class="metric__unit">{{ unit }}</span>
    </p>
    <footer class="metric__foot">
      <span v-if="hint">{{ hint }}</span>
      <span v-if="updatedAt" class="metric__time">{{ stale ? '已过期 · ' : '' }}{{ updatedAt }}</span>
    </footer>
  </section>
</template>

<style scoped>
.metric {
  display: flex;
  flex-direction: column;
  gap: var(--space-2);
  padding: var(--space-4);
  background: var(--bg-surface);
  border: 1px solid var(--border-default);
  border-radius: var(--radius-lg);
}
.metric--stale {
  opacity: 0.72;
}
.metric__head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: var(--space-2);
}
.metric__title {
  color: var(--text-secondary);
  font-size: var(--font-size-sm);
}
.metric__value {
  margin: 0;
  display: flex;
  align-items: baseline;
  gap: var(--space-1);
}
.metric__number {
  font-size: var(--font-size-2xl);
  font-weight: 600;
  font-variant-numeric: tabular-nums;
}
.metric__unit {
  color: var(--text-secondary);
  font-size: var(--font-size-sm);
}
.metric__foot {
  display: flex;
  justify-content: space-between;
  gap: var(--space-2);
  color: var(--text-secondary);
  font-size: var(--font-size-xs);
}
.metric__time {
  font-variant-numeric: tabular-nums;
}
</style>

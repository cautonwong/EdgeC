<script setup lang="ts">
/**
 * L2 常驻横幅：需要用户处理的状态（待应用、断连、能力不可用）用横幅，
 * 不用 Toast —— Toast 只用于瞬时反馈（§9.3）。
 */
withDefaults(
  defineProps<{
    tone?: 'info' | 'warning' | 'danger' | 'success';
    title?: string;
  }>(),
  { tone: 'info' },
);
</script>

<template>
  <div class="banner" :class="`banner--${tone}`" role="status">
    <div class="banner__body">
      <strong v-if="title" class="banner__title">{{ title }}</strong>
      <slot />
    </div>
    <div class="banner__actions"><slot name="actions" /></div>
  </div>
</template>

<style scoped>
.banner {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: var(--space-4);
  padding: var(--space-3) var(--space-4);
  border-radius: var(--radius-md);
  border: 1px solid var(--border-default);
  background: var(--bg-surface);
  font-size: var(--font-size-sm);
}
.banner--warning {
  background: var(--color-warning-bg);
  border-color: color-mix(in srgb, var(--color-warning) 35%, transparent);
  color: var(--color-warning);
}
.banner--danger {
  background: var(--color-danger-bg);
  border-color: color-mix(in srgb, var(--color-danger) 35%, transparent);
  color: var(--color-danger);
}
.banner--info {
  color: var(--color-info);
}
.banner--success {
  color: var(--color-success);
}
.banner__title {
  margin-right: var(--space-2);
}
.banner__actions {
  display: flex;
  gap: var(--space-2);
  flex-shrink: 0;
}
</style>

<script setup lang="ts">
/** L2 状态徽标：圆点 + 文字，不单靠颜色表达状态（§9.3 / §9.5）。 */
withDefaults(
  defineProps<{
    status: 'normal' | 'warning' | 'fault' | 'offline' | 'unconfigured';
    label?: string;
    count?: number;
  }>(),
  {},
);

const ICONS: Record<string, string> = {
  normal: '●',
  warning: '▲',
  fault: '✕',
  offline: '○',
  unconfigured: '◇',
};

const TEXT: Record<string, string> = {
  normal: '正常',
  warning: '告警',
  fault: '故障',
  offline: '离线',
  unconfigured: '未配置',
};
</script>

<template>
  <span class="badge" :class="`badge--${status}`">
    <span class="badge__icon" aria-hidden="true">{{ ICONS[status] }}</span>
    <span class="badge__text">{{ label ?? TEXT[status] }}</span>
    <span v-if="count !== undefined" class="badge__count">{{ count }}</span>
  </span>
</template>

<style scoped>
.badge {
  display: inline-flex;
  align-items: center;
  gap: var(--space-1);
  padding: 2px var(--space-2);
  border-radius: 999px;
  font-size: var(--font-size-xs);
  border: 1px solid transparent;
  white-space: nowrap;
}
.badge--normal {
  color: var(--color-success);
  border-color: color-mix(in srgb, var(--color-success) 40%, transparent);
}
.badge--warning {
  color: var(--color-warning);
  background: var(--color-warning-bg);
  border-color: color-mix(in srgb, var(--color-warning) 35%, transparent);
}
.badge--fault {
  color: var(--color-danger);
  background: var(--color-danger-bg);
  border-color: color-mix(in srgb, var(--color-danger) 35%, transparent);
}
.badge--offline,
.badge--unconfigured {
  color: var(--color-offline);
  border-color: var(--border-default);
}
.badge__count {
  font-variant-numeric: tabular-nums;
  font-weight: 600;
}
</style>

<script setup lang="ts">
/** L1 基础按钮。每个区域只出现一个 primary（§9.3）。 */
withDefaults(
  defineProps<{
    variant?: 'primary' | 'secondary' | 'ghost' | 'danger';
    size?: 'sm' | 'md';
    type?: 'button' | 'submit';
    disabled?: boolean;
    loading?: boolean;
  }>(),
  { variant: 'secondary', size: 'md', type: 'button', disabled: false, loading: false },
);
</script>

<template>
  <button
    :type="type"
    :disabled="disabled || loading"
    :aria-busy="loading || undefined"
    :class="['ui-btn', `ui-btn--${variant}`, `ui-btn--${size}`]"
  >
    <span v-if="loading" class="ui-btn__spinner" aria-hidden="true" />
    <slot />
  </button>
</template>

<style scoped>
.ui-btn {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: var(--space-2);
  min-height: var(--btn-height);
  padding: 0 var(--space-4);
  border-radius: var(--btn-radius);
  border: 1px solid transparent;
  font: inherit;
  font-size: var(--font-size-base);
  cursor: pointer;
  transition: background var(--duration-fast) ease, border-color var(--duration-fast) ease;
}
.ui-btn--sm {
  min-height: 28px;
  padding: 0 var(--space-3);
  font-size: var(--font-size-sm);
}
.ui-btn--primary {
  background: var(--color-primary);
  color: var(--color-on-primary);
}
.ui-btn--primary:hover:not(:disabled) {
  background: var(--color-primary-hover);
}
.ui-btn--secondary {
  background: var(--bg-surface);
  border-color: var(--border-default);
  color: var(--text-primary);
}
.ui-btn--secondary:hover:not(:disabled) {
  border-color: var(--border-strong);
}
.ui-btn--ghost {
  background: transparent;
  color: var(--color-primary);
}
.ui-btn--ghost:hover:not(:disabled) {
  background: var(--bg-page);
}
.ui-btn--danger {
  background: var(--color-danger);
  color: var(--palette-white);
}
.ui-btn:disabled {
  opacity: 0.55;
  cursor: not-allowed;
}
.ui-btn__spinner {
  width: 12px;
  height: 12px;
  border: 2px solid currentColor;
  border-right-color: transparent;
  border-radius: 50%;
  animation: ui-spin 0.7s linear infinite;
}
@keyframes ui-spin {
  to {
    transform: rotate(360deg);
  }
}
</style>

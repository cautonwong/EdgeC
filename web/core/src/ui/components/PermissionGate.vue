<script setup lang="ts">
/**
 * L2 权限门：只控制**呈现**，不构成安全边界（§5.1）。
 * 真正的判定在服务端逐请求执行；这里隐藏按钮只是减少误操作。
 */
import { computed } from 'vue';
import { isPermitted } from '../../sdk/permission';

const props = defineProps<{
  granted: string[] | Set<string>;
  allOf?: string[];
  anyOf?: string[];
  /** 无权限时的降级：`hide` 直接不渲染，`disable` 渲染但禁用并说明原因 */
  fallback?: 'hide' | 'disable';
}>();

const grantedSet = computed(() => (props.granted instanceof Set ? props.granted : new Set(props.granted)));
const allowed = computed(() => isPermitted({ allOf: props.allOf ?? [], anyOf: props.anyOf }, grantedSet.value));
const mode = computed(() => props.fallback ?? 'hide');
</script>

<template>
  <slot v-if="allowed" />
  <slot v-else-if="mode === 'disable'" name="disabled">
    <span class="gate-disabled" :title="`需要权限：${[...(allOf ?? []), ...(anyOf ?? [])].join(' / ')}`">
      无权限
    </span>
  </slot>
</template>

<style scoped>
.gate-disabled {
  color: var(--color-offline);
  font-size: var(--font-size-xs);
  cursor: not-allowed;
}
</style>

<script setup lang="ts">
/**
 * 拒绝页（§5.4）：直接输入 URL 时，必须区分"没权限"和"设备不支持"。
 * 只读降级与能力缺失都不是安全边界，服务端仍会对每个 API 独立判定。
 */
import { computed } from 'vue';
import { useRoute } from 'vue-router';
import { useRegistryStore } from '../stores/registry';
import UiButton from '../../ui/components/Button.vue';

const route = useRoute();
const registryStore = useRegistryStore();

const pageId = computed(() => String(route.params.pageId ?? ''));
const page = computed(() => registryStore.registry?.pages.find((p) => p.id === pageId.value) ?? null);
const denial = computed(() => (page.value ? registryStore.denialOf(page.value) : null));

const title = computed(() => {
  switch (denial.value?.state) {
    case 'forbidden':
      return '403 · 当前账号无权访问该页面';
    case 'unsupported':
      return '当前设备不支持该功能';
    case 'unlicensed':
      return '该功能未授权（许可未生效）';
    default:
      return '该页面未包含在此构建中';
  }
});

const detail = computed(() => {
  if (!denial.value) return '';
  if (denial.value.state === 'forbidden' && denial.value.missing.length) {
    return `缺少权限：${denial.value.missing.join('、')}`;
  }
  if (denial.value.state === 'unsupported' && denial.value.missingCapabilities.length) {
    return `缺少能力：${denial.value.missingCapabilities.join('、')}`;
  }
  if (denial.value.state === 'unlicensed') {
    return '设备上未安装或已过期对应的功能许可。';
  }
  return '请联系设备管理员确认角色与授权范围。';
});
</script>

<template>
  <section class="denied" data-testid="denied-page">
    <p class="denied__state" data-testid="denied-state">{{ denial?.state ?? 'not-built' }}</p>
    <h1 class="denied__title">{{ title }}</h1>
    <p class="denied__detail">{{ detail }}</p>
    <p v-if="page" class="denied__meta">页面：{{ page.title }}（{{ page.id }}）</p>
    <UiButton variant="secondary" @click="$router.replace('/')">返回概览</UiButton>
  </section>
</template>

<style scoped>
.denied {
  padding: var(--space-8);
  max-width: 640px;
}
.denied__state {
  margin: 0 0 var(--space-2);
  color: var(--text-secondary);
  font-family: var(--font-mono);
  font-size: var(--font-size-xs);
}
.denied__title {
  margin: 0 0 var(--space-3);
  font-size: var(--font-size-xl);
}
.denied__detail {
  margin: 0 0 var(--space-2);
  color: var(--text-secondary);
}
.denied__meta {
  margin: 0 0 var(--space-4);
  color: var(--text-secondary);
  font-size: var(--font-size-xs);
  font-family: var(--font-mono);
}
</style>

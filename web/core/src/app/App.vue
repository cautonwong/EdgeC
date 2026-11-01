<script setup lang="ts">
/** 根组件：只处理"连系统信息都拿不到"这种致命状态，其余交给路由。 */
import { useSessionStore } from './stores/session';
import UiBanner from '../ui/components/Banner.vue';

const session = useSessionStore();
</script>

<template>
  <div v-if="session.status === 'error'" class="fatal" data-testid="fatal">
    <UiBanner tone="danger" title="无法初始化界面">
      {{ session.error }}
    </UiBanner>
  </div>
  <router-view v-else />
</template>

<style scoped>
.fatal {
  padding: var(--space-6);
}
</style>

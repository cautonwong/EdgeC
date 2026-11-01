<script setup lang="ts">
/**
 * App Shell（§6.1 / §6.2 / §8.1）。
 *
 * 侧栏分组固定六组，条目由静态 Registry ∩ 权限 ∩ 能力 ∩ 许可生成；分组内一条都不
 * 可见时整组隐藏。顶栏的"待应用/断连"类状态用常驻横幅表达。
 */
import { computed, defineAsyncComponent, ref } from 'vue';
import { useRouter } from 'vue-router';
import { useRegistryStore } from '../stores/registry';
import { useSessionStore } from '../stores/session';
import { useCapabilityStore } from '../stores/capability';
import UiBanner from '../../ui/components/Banner.vue';
import UiButton from '../../ui/components/Button.vue';
// 全局抽屉（控制台 / AI 副驾）沿用既有实现：它们是 Shell 的能力，不属于任何产品包。
// 用异步组件挂载：抽屉只在用户打开时才需要，静态 import 会把终端渲染器和 Markdown
// 解析器全部塞进首屏 chunk（实测多出 ~45KB gzip），与 §6.8 的首屏预算直接冲突。
const WebConsoleDrawer = defineAsyncComponent(() => import('../../components/console/WebConsoleDrawer.vue'));
const AICopilotDrawer = defineAsyncComponent(() => import('../../components/ai/AICopilotDrawer.vue'));
import { useAICopilot } from '../../services/AIProviderService';

const router = useRouter();
const registryStore = useRegistryStore();
const session = useSessionStore();
const capability = useCapabilityStore();

const { toggleCopilot } = useAICopilot();
const isConsoleOpen = ref(false);

const productName = computed(() => registryStore.registry?.productName ?? 'EdgeOS');
const version = computed(() => registryStore.registry?.productVersion ?? '');
const releaseId = computed(() => registryStore.registry?.releaseScope.releaseId ?? '');

async function logout(): Promise<void> {
  await session.logout();
  capability.reset();
  await router.replace({ name: 'login' });
}
</script>

<template>
  <div class="shell">
    <aside class="shell__sidebar">
      <div class="shell__brand">
        <span class="shell__logo" aria-hidden="true">E</span>
        <div>
          <p class="shell__product">{{ productName }}</p>
          <p class="shell__version">v{{ version }} · release {{ releaseId }}</p>
        </div>
      </div>

      <nav class="shell__nav" aria-label="主导航">
        <div v-for="group in registryStore.menu" :key="group.id" class="nav-group" :data-testid="`nav-group-${group.id}`">
          <p class="nav-group__title">{{ group.title }}</p>
          <router-link
            v-for="item in group.items"
            :key="item.pageId"
            :to="item.path"
            class="nav-item"
            :data-testid="`nav-item-${item.pageId}`"
            active-class="nav-item--active"
          >
            {{ item.title }}
          </router-link>
        </div>
        <p v-if="registryStore.menu.length === 0" class="nav-empty">当前账号没有可见页面</p>
      </nav>
    </aside>

    <div class="shell__main">
      <header class="shell__topbar">
        <div class="shell__crumb">
          <slot name="topbar" />
        </div>
        <div class="shell__user">
          <UiButton variant="ghost" size="sm" data-testid="toggle-copilot" @click="toggleCopilot()">AI Copilot</UiButton>
          <UiButton
            variant="ghost"
            size="sm"
            data-testid="toggle-console"
            title="Toggle Web Console (Ctrl+`)"
            @click="isConsoleOpen = !isConsoleOpen"
          >
            控制台
          </UiButton>
          <span class="shell__username" data-testid="topbar-user">{{ session.displayName }}</span>
          <span class="shell__roles">{{ session.roles.join(' / ') }}</span>
          <UiButton variant="ghost" size="sm" data-testid="logout" @click="logout">退出</UiButton>
        </div>
      </header>

      <UiBanner v-if="capability.unknown" tone="warning" title="能力快照不可用" data-testid="capability-banner">
        无法确认设备能力，需要能力的页面与操作一律按"不支持"处理（fail-closed）。
      </UiBanner>

      <main class="shell__content">
        <router-view />
      </main>
    </div>

    <WebConsoleDrawer v-model:isOpen="isConsoleOpen" />
    <AICopilotDrawer />
  </div>
</template>

<style scoped>
.shell {
  display: flex;
  min-height: 100vh;
  background: var(--bg-page);
}
.shell__sidebar {
  width: var(--sidebar-width);
  flex-shrink: 0;
  display: flex;
  flex-direction: column;
  background: var(--bg-surface);
  border-right: 1px solid var(--border-default);
}
.shell__brand {
  display: flex;
  align-items: center;
  gap: var(--space-3);
  padding: var(--space-4);
  border-bottom: 1px solid var(--border-default);
}
.shell__logo {
  width: 32px;
  height: 32px;
  display: grid;
  place-items: center;
  border-radius: var(--radius-md);
  background: var(--color-primary);
  color: var(--color-on-primary);
  font-weight: 700;
}
.shell__product {
  margin: 0;
  font-weight: 600;
  font-size: var(--font-size-sm);
}
.shell__version {
  margin: 0;
  color: var(--text-secondary);
  font-size: 11px;
  font-variant-numeric: tabular-nums;
}
.shell__nav {
  padding: var(--space-3) var(--space-2);
  overflow-y: auto;
}
.nav-group {
  margin-bottom: var(--space-4);
}
.nav-group__title {
  margin: 0 0 var(--space-1);
  padding: 0 var(--space-3);
  color: var(--text-secondary);
  font-size: 11px;
  letter-spacing: 0.08em;
  text-transform: uppercase;
}
.nav-item {
  display: block;
  padding: var(--space-2) var(--space-3);
  border-radius: var(--radius-sm);
  color: var(--text-primary);
  text-decoration: none;
  font-size: var(--font-size-sm);
}
.nav-item:hover {
  background: var(--bg-page);
}
.nav-item--active {
  background: color-mix(in srgb, var(--color-primary) 14%, transparent);
  color: var(--color-primary);
  font-weight: 600;
}
.nav-empty {
  padding: var(--space-3);
  color: var(--text-secondary);
  font-size: var(--font-size-xs);
}
.shell__main {
  flex: 1;
  min-width: 0;
  display: flex;
  flex-direction: column;
}
.shell__topbar {
  height: var(--topbar-height);
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: var(--space-4);
  padding: 0 var(--space-6);
  background: var(--bg-surface);
  border-bottom: 1px solid var(--border-default);
}
.shell__user {
  display: flex;
  align-items: center;
  gap: var(--space-3);
}
.shell__username {
  font-size: var(--font-size-sm);
}
.shell__roles {
  color: var(--text-secondary);
  font-size: var(--font-size-xs);
}
.shell__content {
  flex: 1;
  padding: var(--space-6);
  overflow: auto;
}

@media (max-width: 1279px) {
  .shell__sidebar {
    width: 72px;
  }
  .shell__product,
  .shell__version,
  .nav-group__title {
    display: none;
  }
}
</style>

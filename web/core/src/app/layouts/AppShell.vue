<script setup lang="ts">
/**
 * App Shell（§6.1 / §6.2 / §8.1）。
 *
 * 侧栏分组固定六组，条目由静态 Registry ∩ 权限 ∩ 能力 ∩ 许可生成；分组内一条都不
 * 可见时整组隐藏。顶栏的"待应用/断连"类状态用常驻横幅表达。
 */
import { computed, defineAsyncComponent, onBeforeUnmount, onMounted, ref, watch } from 'vue';
import { useRoute, useRouter } from 'vue-router';
import { useRegistryStore } from '../stores/registry';
import { useSessionStore } from '../stores/session';
import { useCapabilityStore } from '../stores/capability';
import { useLiveStore } from '../stores/live';
import { cycleTheme, themePreference } from '../theme';
import UiBanner from '../../ui/components/Banner.vue';
import UiButton from '../../ui/components/Button.vue';
// 全局抽屉（控制台 / AI 副驾）沿用既有实现：它们是 Shell 的能力，不属于任何产品包。
// 用异步组件挂载：抽屉只在用户打开时才需要，静态 import 会把终端渲染器和 Markdown
// 解析器全部塞进首屏 chunk（实测多出 ~45KB gzip），与 §6.8 的首屏预算直接冲突。
const WebConsoleDrawer = defineAsyncComponent(() => import('../../components/console/WebConsoleDrawer.vue'));
const AICopilotDrawer = defineAsyncComponent(() => import('../../components/ai/AICopilotDrawer.vue'));
import { useAICopilot } from '../../services/AIProviderService';

const route = useRoute();
const router = useRouter();
const registryStore = useRegistryStore();
const session = useSessionStore();
const capability = useCapabilityStore();
const live = useLiveStore();

const { toggleCopilot } = useAICopilot();
const isConsoleOpen = ref(false);
const mobileNavOpen = ref(false);

const productName = computed(() => registryStore.registry?.productName ?? 'EdgeOS');
const version = computed(() => registryStore.registry?.productVersion ?? '');
const releaseId = computed(() => registryStore.registry?.releaseScope.releaseId ?? '');

const themeLabel = computed(() => {
  if (themePreference.value === 'light') return '主题：浅色';
  if (themePreference.value === 'dark') return '主题：深色';
  return '主题：跟随系统';
});

// 实时通道只在已登录时建立；断开后自动重连，状态直接反映在顶栏徽标上。
onMounted(() => {
  live.start();
  window.addEventListener('keydown', onShellKeydown);
});
onBeforeUnmount(() => {
  live.stop();
  window.removeEventListener('keydown', onShellKeydown);
});

watch(
  () => route.fullPath,
  () => {
    mobileNavOpen.value = false;
  },
);

function onShellKeydown(event: KeyboardEvent): void {
  if (event.key === 'Escape' && mobileNavOpen.value) mobileNavOpen.value = false;
}

const liveLabel = computed(() => {
  if (live.state === 'open') return '实时';
  if (live.state === 'connecting' || live.state === 'reconnecting') return '重连中';
  return '离线';
});

async function logout(): Promise<void> {
  await session.logout();
  capability.reset();
  await router.replace({ name: 'login' });
}
</script>

<template>
  <div class="shell">
    <aside id="primary-nav" class="shell__sidebar" :class="{ 'shell__sidebar--open': mobileNavOpen }">
      <div class="shell__brand">
        <span class="shell__logo" aria-hidden="true">E</span>
        <div class="shell__brand-text">
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
            :title="item.title"
            :aria-label="item.title"
            :data-testid="`nav-item-${item.pageId}`"
            active-class="nav-item--active"
          >
            <span class="nav-item__glyph" aria-hidden="true">{{ item.title.slice(0, 1) }}</span>
            <span class="nav-item__label">{{ item.title }}</span>
          </router-link>
        </div>
        <p v-if="registryStore.menu.length === 0" class="nav-empty">当前账号没有可见页面</p>
      </nav>
    </aside>

    <button
      v-if="mobileNavOpen"
      type="button"
      class="shell__scrim"
      aria-label="关闭导航"
      @click="mobileNavOpen = false"
    />

    <div class="shell__main">
      <header class="shell__topbar">
        <button
          type="button"
          class="shell__menu"
          :aria-expanded="mobileNavOpen"
          aria-controls="primary-nav"
          aria-label="打开导航"
          @click="mobileNavOpen = !mobileNavOpen"
        >
          ☰
        </button>
        <div class="shell__crumb">
          <slot name="topbar" />
        </div>
        <div class="shell__user">
          <button
            type="button"
            class="shell__theme"
            :title="themeLabel"
            :aria-label="themeLabel"
            data-testid="theme-toggle"
            @click="cycleTheme()"
          >
            {{ themeLabel }}
          </button>
          <span class="shell__live" :class="`shell__live--${live.state}`" :data-testid="'live-badge'">{{ liveLabel }}</span>
          <UiButton variant="ghost" size="sm" data-testid="toggle-copilot" @click="toggleCopilot()">AI Copilot</UiButton>
          <UiButton
            variant="ghost"
            size="sm"
            class="shell__hide-sm"
            data-testid="toggle-console"
            title="Toggle Web Console (Ctrl+`)"
            @click="isConsoleOpen = !isConsoleOpen"
          >
            控制台
          </UiButton>
          <span class="shell__username shell__hide-sm" data-testid="topbar-user">{{ session.displayName }}</span>
          <span class="shell__roles shell__hide-sm">{{ session.roles.join(' / ') }}</span>
          <UiButton variant="ghost" size="sm" data-testid="logout" @click="logout">退出</UiButton>
        </div>
      </header>

      <UiBanner v-if="capability.unknown" tone="warning" title="能力快照不可用" data-testid="capability-banner">
        无法确认设备能力，需要能力的页面与操作一律按"不支持"处理（fail-closed）。
      </UiBanner>

      <main class="shell__content">
        <div class="shell__page">
          <router-view />
        </div>
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
  flex-shrink: 0;
  display: grid;
  place-items: center;
  border-radius: var(--radius-md);
  background: var(--color-primary);
  color: var(--color-on-primary);
  font-weight: 600;
}
.shell__brand-text {
  min-width: 0;
}
.shell__product {
  margin: 0;
  font-weight: 600;
  font-size: var(--font-size-sm);
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}
.shell__version {
  margin: 0;
  color: var(--text-secondary);
  font-size: var(--font-size-xs);
  font-variant-numeric: tabular-nums;
  white-space: nowrap;
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
  font-size: var(--font-size-xs);
}
.nav-item {
  display: flex;
  align-items: center;
  gap: var(--space-2);
  padding: var(--space-2) var(--space-3);
  border-radius: var(--radius-sm);
  color: var(--text-primary);
  text-decoration: none;
  font-size: var(--font-size-sm);
  white-space: nowrap;
}
.nav-item:hover {
  background: var(--bg-page);
}
.nav-item--active {
  background: color-mix(in srgb, var(--color-primary) 14%, transparent);
  color: var(--color-primary);
  font-weight: 600;
}
.nav-item__glyph {
  display: none;
  width: 24px;
  height: 24px;
  place-items: center;
  border-radius: var(--radius-sm);
  background: color-mix(in srgb, var(--color-primary) 12%, transparent);
  font-size: var(--font-size-xs);
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
  min-height: var(--topbar-height);
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: var(--space-4);
  padding: 0 var(--space-6);
  background: var(--bg-surface);
  border-bottom: 1px solid var(--border-default);
}
.shell__menu {
  display: none;
  width: var(--touch-target);
  height: var(--touch-target);
  border: 1px solid var(--border-default);
  border-radius: var(--radius-md);
  background: var(--bg-surface);
  color: var(--text-primary);
  font-size: var(--font-size-lg);
  cursor: pointer;
}
.shell__crumb {
  min-width: 0;
}
.shell__user {
  display: flex;
  align-items: center;
  gap: var(--space-3);
  min-width: 0;
}
.shell__theme {
  padding: 4px var(--space-2);
  border: 1px solid var(--border-default);
  border-radius: 999px;
  background: transparent;
  color: var(--text-secondary);
  font: inherit;
  font-size: var(--font-size-xs);
  cursor: pointer;
  white-space: nowrap;
}
.shell__theme:hover {
  color: var(--text-primary);
  border-color: var(--border-strong);
}
.shell__username {
  font-size: var(--font-size-sm);
  white-space: nowrap;
}
.shell__live {
  display: inline-flex;
  align-items: center;
  gap: 4px;
  padding: 1px 8px;
  border-radius: 999px;
  font-size: var(--font-size-xs);
  border: 1px solid var(--border-default);
  color: var(--text-secondary);
  white-space: nowrap;
}
.shell__live--open {
  color: var(--color-success);
  border-color: color-mix(in srgb, var(--color-success) 40%, transparent);
}
.shell__live--reconnecting,
.shell__live--connecting {
  color: var(--color-warning);
  border-color: color-mix(in srgb, var(--color-warning) 40%, transparent);
}
.shell__roles {
  color: var(--text-secondary);
  font-size: var(--font-size-xs);
  white-space: nowrap;
}
.shell__content {
  flex: 1;
  padding: var(--space-6);
  overflow: auto;
}
.shell__page {
  width: 100%;
  max-width: 1600px;
  margin: 0 auto;
}
.shell__scrim {
  display: none;
}

@media (max-width: 1279px) {
  .shell__sidebar {
    width: 64px;
  }
  .shell__product,
  .shell__version,
  .nav-group__title,
  .nav-item__label {
    display: none;
  }
  .shell__brand {
    justify-content: center;
    padding: var(--space-3) var(--space-2);
  }
  .shell__nav {
    padding: var(--space-2) var(--space-1);
  }
  .nav-item {
    justify-content: center;
    padding: var(--space-2);
  }
  .nav-item__glyph {
    display: grid;
  }
  .shell__hide-sm {
    display: none;
  }
}

@media (max-width: 767px) {
  .shell__sidebar {
    position: fixed;
    inset: 0 auto 0 0;
    z-index: 100;
    width: var(--sidebar-width);
    transform: translateX(-100%);
    transition: transform var(--duration-base) ease;
  }
  .shell__sidebar--open {
    transform: translateX(0);
  }
  .shell__product,
  .shell__version,
  .nav-group__title,
  .nav-item__label {
    display: block;
  }
  .shell__brand {
    justify-content: flex-start;
    padding: var(--space-4);
  }
  .shell__nav {
    padding: var(--space-3) var(--space-2);
  }
  .nav-item {
    justify-content: flex-start;
    padding: var(--space-2) var(--space-3);
  }
  .nav-item__glyph {
    display: none;
  }
  .shell__scrim {
    display: block;
    position: fixed;
    inset: 0;
    z-index: 90;
    border: 0;
    background: color-mix(in srgb, var(--bg-page) 60%, transparent);
  }
  .shell__menu {
    display: grid;
    place-items: center;
    flex-shrink: 0;
  }
  .shell__topbar {
    padding: 0 var(--space-3);
    gap: var(--space-2);
    flex-wrap: nowrap;
  }
  .shell__user {
    gap: var(--space-2);
  }
  .shell__content {
    padding: var(--space-3);
  }
}
</style>

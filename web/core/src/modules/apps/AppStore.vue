<script setup lang="ts">
/**
 * 应用商店：把 WASM 能力模块当作现场可安装的模块陈列。
 *
 * shortcut: install/start/stop/uninstall only change local state until the
 * package manager HTTP API lands; replace with /api/v1/apps calls when it does.
 */
import { computed, onBeforeUnmount, ref, watch } from 'vue';
import UiButton from '../../ui/components/Button.vue';
import { useAuthz } from '../../app/composables/useAuthz';
import { useDialogA11y } from '../../app/composables/useDialogA11y';
import { APPS, CATEGORY_LABELS, type AppCategory, type StoreApp } from './catalog';

type InstalledState = { state: 'installed' | 'running' | 'stopped' };
type Notice = { tone: 'success' | 'danger'; text: string };

const STORAGE_KEY = 'edgeos.appstore.installed';
const SEED: Record<string, InstalledState> = {
  modbus_bridge: { state: 'running' },
  audit_shield: { state: 'running' },
};

const CATEGORY_TONES: Record<AppCategory, string> = {
  power: 'var(--color-warning)',
  protect: 'var(--color-danger)',
  energy: 'var(--color-success)',
  protocol: 'var(--color-primary)',
  diagnostics: 'var(--color-info)',
  security: 'var(--color-offline)',
};

function loadInstalled(): Record<string, InstalledState> {
  try {
    const raw = localStorage.getItem(STORAGE_KEY);
    if (raw) {
      const parsed = JSON.parse(raw) as Record<string, InstalledState>;
      if (parsed && typeof parsed === 'object') return parsed;
    }
  } catch {
    // private mode or storage disabled: the page still works for this session
  }
  return { ...SEED };
}

const authz = useAuthz();
const installed = ref(loadInstalled());
const query = ref('');
const category = ref<AppCategory | 'all'>('all');
const view = ref<'all' | 'installed'>('all');
const selectedId = ref<string | null>(null);
const installingId = ref<string | null>(null);
const confirmUninstall = ref(false);
const notice = ref<Notice | null>(null);
const drawerRef = ref<HTMLElement | null>(null);

let noticeTimer: ReturnType<typeof setTimeout> | undefined;
let installTimer: ReturnType<typeof setTimeout> | undefined;

const canInstall = computed(() => authz.can('core.app:execute'));
const installedCount = computed(() => Object.keys(installed.value).length);
const featured = APPS.find((app) => app.featured) ?? APPS[0];
const selectedApp = computed(() => APPS.find((app) => app.id === selectedId.value) ?? null);
const drawerOpen = computed(() => selectedApp.value !== null);

const categoryEntries = computed(() => [
  { id: 'all' as const, label: '全部', count: APPS.length },
  ...(Object.entries(CATEGORY_LABELS) as [AppCategory, string][]).map(([id, label]) => ({
    id,
    label,
    count: APPS.filter((app) => app.category === id).length,
  })),
]);

const visibleApps = computed(() => {
  const needle = query.value.trim().toLowerCase();
  return APPS.filter((app) => {
    if (view.value === 'installed' && !installed.value[app.id]) return false;
    if (category.value !== 'all' && app.category !== category.value) return false;
    if (!needle) return true;
    return [app.name, app.subtitle, app.id, app.triggerTag, ...app.protocols].some((value) =>
      value.toLowerCase().includes(needle),
    );
  });
});

useDialogA11y(drawerOpen, drawerRef, closeDrawer);

watch(installed, (value) => {
  try {
    localStorage.setItem(STORAGE_KEY, JSON.stringify(value));
  } catch {
    // persistence is best-effort
  }
}, { deep: true });

watch(drawerOpen, (open) => {
  document.body.style.overflow = open ? 'hidden' : '';
});

onBeforeUnmount(() => {
  if (noticeTimer) clearTimeout(noticeTimer);
  if (installTimer) clearTimeout(installTimer);
  document.body.style.overflow = '';
});

function stateOf(app: StoreApp): InstalledState['state'] | undefined {
  return installed.value[app.id]?.state;
}

function stateLabel(app: StoreApp): string {
  const state = stateOf(app);
  if (state === 'running') return '运行中';
  if (state === 'stopped') return '已停止';
  if (state === 'installed') return '已安装';
  return '可安装';
}

function stateClass(app: StoreApp): string {
  const state = stateOf(app);
  return state ? `is-${state}` : 'is-available';
}

function categoryTone(value: AppCategory): string {
  return CATEGORY_TONES[value];
}

function formatSize(sizeKb: number): string {
  return sizeKb >= 1024 ? `${(sizeKb / 1024).toFixed(1)} MB` : `${sizeKb} KB`;
}

function formatDeployments(count: number): string {
  return count.toLocaleString('zh-CN');
}

function notify(tone: Notice['tone'], text: string): void {
  if (noticeTimer) clearTimeout(noticeTimer);
  notice.value = { tone, text };
  noticeTimer = setTimeout(() => {
    notice.value = null;
  }, 4000);
}

function openApp(app: StoreApp): void {
  selectedId.value = app.id;
  confirmUninstall.value = false;
}

function closeDrawer(): void {
  selectedId.value = null;
  confirmUninstall.value = false;
}

function install(app: StoreApp): void {
  if (!canInstall.value || installingId.value) return;
  installingId.value = app.id;
  if (installTimer) clearTimeout(installTimer);
  installTimer = setTimeout(() => {
    installed.value = { ...installed.value, [app.id]: { state: 'installed' } };
    installingId.value = null;
    notify('success', `${app.name} 已安装到本机`);
  }, 900);
}

function start(app: StoreApp): void {
  if (!canInstall.value) return;
  installed.value = { ...installed.value, [app.id]: { state: 'running' } };
  notify('success', `${app.name} 已启动`);
}

function stop(app: StoreApp): void {
  if (!canInstall.value) return;
  installed.value = { ...installed.value, [app.id]: { state: 'stopped' } };
  notify('success', `${app.name} 已停止`);
}

function primaryAction(app: StoreApp): void {
  const state = stateOf(app);
  if (!state) {
    install(app);
    return;
  }
  if (state === 'running') {
    stop(app);
    return;
  }
  start(app);
}

function uninstall(app: StoreApp): void {
  if (!canInstall.value) return;
  if (!confirmUninstall.value) {
    confirmUninstall.value = true;
    return;
  }
  const next = { ...installed.value };
  delete next[app.id];
  installed.value = next;
  closeDrawer();
  notify('success', `${app.name} 已卸载`);
}

function heroAction(): void {
  if (stateOf(featured)) {
    openApp(featured);
    return;
  }
  install(featured);
}

function resetFilters(): void {
  query.value = '';
  category.value = 'all';
  view.value = 'all';
}
</script>

<template>
  <section class="store" data-testid="app-store">
    <header class="store__header">
      <div class="store__intro">
        <h1>应用商店</h1>
        <p>为这台边缘网关安装可运行的能力模块。应用在 WASM 沙箱中执行，按清单申请权限。</p>
      </div>
      <div class="store__tools">
        <label class="store__search">
          <span class="ui-visually-hidden">搜索应用</span>
          <input v-model="query" type="search" data-testid="app-search" placeholder="搜索应用、协议或点位" />
        </label>
        <div class="store__view" role="group" aria-label="应用范围">
          <button type="button" :aria-pressed="view === 'all'" @click="view = 'all'">全部</button>
          <button type="button" :aria-pressed="view === 'installed'" @click="view = 'installed'">
            已安装 <span>{{ installedCount }}</span>
          </button>
        </div>
      </div>
    </header>

    <p v-if="!canInstall" class="store__role-note">
      当前角色只能浏览。安装、启停和卸载需要 core.app:execute 权限。
    </p>

    <div v-if="notice" class="store__notice" :class="`store__notice--${notice.tone}`" role="status">
      {{ notice.text }}
    </div>

    <section class="hero" :style="{ '--tile-accent': categoryTone(featured.category) }">
      <div class="hero__identity">
        <div class="app-tile app-tile--hero" aria-hidden="true"><span>{{ featured.glyph }}</span></div>
        <div class="hero__copy">
          <span class="hero__tag">本机推荐</span>
          <h2>{{ featured.name }}</h2>
          <p class="hero__tagline">{{ featured.tagline }}</p>
          <p class="hero__description">{{ featured.description }}</p>
          <div class="hero__meta">
            <span>{{ featured.version }}</span>
            <span>{{ formatSize(featured.sizeKb) }}</span>
            <span>评分 {{ featured.rating.toFixed(1) }}</span>
            <span>{{ formatDeployments(featured.deployments) }} 台部署</span>
          </div>
        </div>
      </div>
      <div class="hero__side">
        <dl>
          <div><dt>触发点位</dt><dd>{{ featured.triggerTag }}</dd></div>
          <div><dt>堆上限</dt><dd>{{ featured.heapKb }} KB</dd></div>
          <div><dt>单次超时</dt><dd>{{ featured.timeoutMs }} ms</dd></div>
          <div><dt>故障策略</dt><dd>{{ featured.faultPolicy === 'auto-restart' ? '自动重启' : '隔离告警' }}</dd></div>
        </dl>
        <UiButton
          variant="primary"
          :loading="installingId === featured.id"
          :disabled="!canInstall || !!installingId"
          :title="!canInstall ? '需要 core.app:execute 权限' : undefined"
          data-testid="hero-install"
          @click="heroAction"
        >
          {{ stateOf(featured) ? '查看状态' : '安装到本机' }}
        </UiButton>
        <button type="button" class="hero__details" @click="openApp(featured)">查看应用清单</button>
      </div>
    </section>

    <nav class="store__categories" aria-label="应用分类">
      <button
        v-for="entry in categoryEntries"
        :key="entry.id"
        type="button"
        class="store__chip"
        :class="{ 'store__chip--active': category === entry.id }"
        :aria-pressed="category === entry.id"
        @click="category = entry.id"
      >
        {{ entry.label }} <span>{{ entry.count }}</span>
      </button>
    </nav>

    <div v-if="visibleApps.length" class="store__grid">
      <article
        v-for="(app, index) in visibleApps"
        :key="app.id"
        class="app-card"
        data-testid="app-card"
        :data-app-id="app.id"
        :style="{ '--tile-accent': categoryTone(app.category), '--i': index }"
      >
        <div class="app-card__top">
          <div class="app-tile" aria-hidden="true"><span>{{ app.glyph }}</span></div>
          <div class="app-card__identity">
            <h3>{{ app.name }}</h3>
            <p>{{ app.subtitle }}</p>
          </div>
          <span class="app-state" :class="stateClass(app)" :data-testid="`state-${app.id}`">{{ stateLabel(app) }}</span>
        </div>
        <p class="app-card__description">{{ app.description }}</p>
        <div class="app-card__meta">
          <span>{{ app.version }}</span>
          <span>{{ formatSize(app.sizeKb) }}</span>
          <span>评分 {{ app.rating.toFixed(1) }}</span>
          <span>{{ formatDeployments(app.deployments) }} 台部署</span>
        </div>
        <div class="app-card__actions">
          <button type="button" class="app-card__details" @click="openApp(app)">详情</button>
          <UiButton
            size="sm"
            :variant="stateOf(app) ? 'secondary' : 'primary'"
            :loading="installingId === app.id"
            :disabled="!canInstall || (!!installingId && installingId !== app.id)"
            :title="!canInstall ? '需要 core.app:execute 权限' : undefined"
            :data-testid="`install-${app.id}`"
            @click="primaryAction(app)"
          >
            {{ installingId === app.id ? '安装中…' : stateOf(app) === 'running' ? '停止' : stateOf(app) ? '启动' : '安装' }}
          </UiButton>
        </div>
      </article>
    </div>

    <div v-else class="store__empty">
      <p>没有匹配的应用。</p>
      <p>试试清空搜索，或切回全部分类。</p>
      <UiButton variant="secondary" size="sm" @click="resetFilters">清空筛选</UiButton>
    </div>

    <Teleport to="body">
      <div v-if="selectedApp" class="drawer__scrim" @click="closeDrawer" />
      <aside
        v-if="selectedApp"
        ref="drawerRef"
        class="drawer"
        role="dialog"
        aria-modal="true"
        :aria-label="`${selectedApp.name} 详情`"
        tabindex="-1"
        data-testid="app-drawer"
      >
        <header class="drawer__head">
          <div class="app-tile app-tile--drawer" :style="{ '--tile-accent': categoryTone(selectedApp.category) }" aria-hidden="true">
            <span>{{ selectedApp.glyph }}</span>
          </div>
          <div class="drawer__identity">
            <p class="drawer__category">{{ CATEGORY_LABELS[selectedApp.category] }}</p>
            <h2>{{ selectedApp.name }}</h2>
            <p>{{ selectedApp.subtitle }}</p>
          </div>
          <button type="button" class="drawer__close" aria-label="关闭应用详情" title="关闭" @click="closeDrawer">✕</button>
        </header>

        <div class="drawer__body">
          <p class="drawer__lede">{{ selectedApp.description }}</p>

          <div class="drawer__actions">
            <UiButton
              v-if="!stateOf(selectedApp)"
              variant="primary"
              :loading="installingId === selectedApp.id"
              :disabled="!canInstall || (!!installingId && installingId !== selectedApp.id)"
              :title="!canInstall ? '需要 core.app:execute 权限' : undefined"
              data-testid="drawer-install"
              @click="install(selectedApp)"
            >
              安装到本机
            </UiButton>
            <UiButton
              v-else
              variant="primary"
              :disabled="!canInstall"
              :title="!canInstall ? '需要 core.app:execute 权限' : undefined"
              data-testid="drawer-start-stop"
              @click="stateOf(selectedApp) === 'running' ? stop(selectedApp) : start(selectedApp)"
            >
              {{ stateOf(selectedApp) === 'running' ? '停止应用' : '启动应用' }}
            </UiButton>
            <UiButton
              v-if="stateOf(selectedApp)"
              variant="danger"
              :disabled="!canInstall"
              :title="!canInstall ? '需要 core.app:execute 权限' : undefined"
              data-testid="drawer-uninstall"
              @click="uninstall(selectedApp)"
            >
              {{ confirmUninstall ? '确认卸载' : '卸载' }}
            </UiButton>
          </div>

          <p v-if="confirmUninstall" class="drawer__confirm">
            卸载会删除本机上的应用包和运行状态，正在运行的规则会立即停止。
            <button type="button" @click="confirmUninstall = false">取消</button>
          </p>

          <section class="drawer__section">
            <h3>它能做什么</h3>
            <ul>
              <li v-for="line in selectedApp.highlights" :key="line">{{ line }}</li>
            </ul>
          </section>

          <section class="drawer__section">
            <h3>运行清单</h3>
            <dl class="drawer__specs">
              <div><dt>版本</dt><dd>{{ selectedApp.version }}</dd></div>
              <div><dt>作者</dt><dd>{{ selectedApp.author }}</dd></div>
              <div><dt>大小</dt><dd>{{ formatSize(selectedApp.sizeKb) }}</dd></div>
              <div><dt>触发点位</dt><dd>{{ selectedApp.triggerTag }}</dd></div>
              <div><dt>堆上限</dt><dd>{{ selectedApp.heapKb }} KB</dd></div>
              <div><dt>单次超时</dt><dd>{{ selectedApp.timeoutMs }} ms</dd></div>
              <div><dt>故障策略</dt><dd>{{ selectedApp.faultPolicy === 'auto-restart' ? '自动重启' : '隔离告警' }}</dd></div>
              <div><dt>协议</dt><dd>{{ selectedApp.protocols.join(' / ') }}</dd></div>
            </dl>
          </section>

          <section class="drawer__section">
            <h3>安装后授予</h3>
            <ul class="drawer__grants">
              <li v-for="grant in selectedApp.grants" :key="grant">{{ grant }}</li>
            </ul>
            <p class="drawer__hint">权限只用于本应用的 WASM 沙箱，不会扩大其他应用的权限。</p>
          </section>

          <section class="drawer__section">
            <h3>更新内容</h3>
            <ul>
              <li v-for="line in selectedApp.changelog" :key="line">{{ line }}</li>
            </ul>
          </section>
        </div>
      </aside>
    </Teleport>
  </section>
</template>

<style scoped>
.store {
  display: flex;
  flex-direction: column;
  gap: var(--space-4);
}
.store__header {
  display: flex;
  flex-wrap: wrap;
  align-items: flex-end;
  justify-content: space-between;
  gap: var(--space-4);
}
.store__intro h1 {
  margin: 0;
  font-size: var(--font-size-2xl);
  font-weight: 600;
  letter-spacing: -0.01em;
}
.store__intro p {
  max-width: 62ch;
  margin: var(--space-1) 0 0;
  color: var(--text-secondary);
  font-size: var(--font-size-sm);
  line-height: 1.55;
}
.store__tools {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: var(--space-2);
}
.store__search input {
  width: 260px;
  height: var(--input-height);
  padding: 0 var(--space-3);
  border: 1px solid var(--border-default);
  border-radius: var(--radius-md);
  background: var(--bg-surface);
  color: var(--text-primary);
  font: inherit;
  font-size: var(--font-size-sm);
}
.store__search input::placeholder {
  color: var(--text-disabled);
}
.store__view {
  display: inline-flex;
  padding: 2px;
  border: 1px solid var(--border-default);
  border-radius: var(--radius-md);
  background: var(--bg-surface);
}
.store__view button {
  min-height: 30px;
  padding: 0 var(--space-3);
  border: 0;
  border-radius: var(--radius-sm);
  background: transparent;
  color: var(--text-secondary);
  font: inherit;
  font-size: var(--font-size-sm);
  cursor: pointer;
}
.store__view button[aria-pressed='true'] {
  background: color-mix(in srgb, var(--color-primary) 12%, transparent);
  color: var(--color-primary);
  font-weight: 600;
}
.store__view span {
  margin-left: 4px;
  font-family: var(--font-mono);
  font-size: var(--font-size-xs);
}
.store__role-note {
  margin: 0;
  padding: var(--space-2) var(--space-3);
  border-left: 2px solid var(--color-warning);
  background: color-mix(in srgb, var(--color-warning) 8%, transparent);
  color: var(--text-primary);
  font-size: var(--font-size-sm);
}
.store__notice {
  padding: var(--space-2) var(--space-3);
  border: 1px solid var(--border-default);
  border-radius: var(--radius-md);
  font-size: var(--font-size-sm);
}
.store__notice--success {
  border-color: color-mix(in srgb, var(--color-success) 35%, transparent);
  background: color-mix(in srgb, var(--color-success) 9%, transparent);
  color: var(--color-success);
}
.store__notice--danger {
  border-color: color-mix(in srgb, var(--color-danger) 35%, transparent);
  background: color-mix(in srgb, var(--color-danger) 9%, transparent);
  color: var(--color-danger);
}
.hero {
  position: relative;
  display: grid;
  grid-template-columns: minmax(0, 1.5fr) minmax(260px, 0.8fr);
  gap: var(--space-6);
  padding: var(--space-6);
  border: 1px solid var(--border-default);
  border-radius: var(--radius-lg);
  background: var(--bg-surface);
  overflow: hidden;
  animation: store-rise 380ms ease both;
}
.hero::before {
  content: '';
  position: absolute;
  inset: 0 auto 0 0;
  width: 4px;
  background: var(--tile-accent);
}
.hero__identity {
  display: flex;
  align-items: flex-start;
  gap: var(--space-4);
  min-width: 0;
}
.hero__copy {
  min-width: 0;
}
.hero__tag {
  display: inline-flex;
  align-items: center;
  min-height: 22px;
  padding: 0 8px;
  border: 1px solid color-mix(in srgb, var(--tile-accent) 35%, transparent);
  border-radius: 999px;
  background: color-mix(in srgb, var(--tile-accent) 9%, transparent);
  color: var(--tile-accent);
  font-size: var(--font-size-xs);
  font-weight: 600;
}
.hero h2 {
  margin: var(--space-2) 0 0;
  font-size: 1.75rem;
  line-height: 1.2;
  font-weight: 600;
  letter-spacing: -0.02em;
}
.hero__tagline {
  margin: var(--space-1) 0 0;
  color: var(--text-primary);
  font-size: var(--font-size-base);
  line-height: 1.5;
}
.hero__description {
  max-width: 58ch;
  margin: var(--space-2) 0 0;
  color: var(--text-secondary);
  font-size: var(--font-size-sm);
  line-height: 1.6;
}
.hero__meta {
  display: flex;
  flex-wrap: wrap;
  gap: var(--space-3);
  margin-top: var(--space-3);
  color: var(--text-secondary);
  font-family: var(--font-mono);
  font-size: var(--font-size-xs);
}
.hero__side {
  display: flex;
  flex-direction: column;
  gap: var(--space-3);
  padding: var(--space-4);
  border: 1px solid var(--border-default);
  border-radius: var(--radius-md);
  background: var(--bg-page);
}
.hero__side dl {
  display: flex;
  flex-direction: column;
  gap: var(--space-2);
  margin: 0;
}
.hero__side dl > div {
  display: flex;
  justify-content: space-between;
  gap: var(--space-3);
}
.hero__side dt {
  color: var(--text-secondary);
  font-size: var(--font-size-xs);
}
.hero__side dd {
  margin: 0;
  color: var(--text-primary);
  font-family: var(--font-mono);
  font-size: var(--font-size-xs);
}
.hero__details {
  align-self: flex-start;
  padding: 0;
  border: 0;
  background: none;
  color: var(--text-secondary);
  font: inherit;
  font-size: var(--font-size-sm);
  text-decoration: underline;
  text-underline-offset: 3px;
  cursor: pointer;
}
.hero__details:hover {
  color: var(--text-primary);
}
.store__categories {
  display: flex;
  flex-wrap: wrap;
  gap: var(--space-2);
}
.store__chip {
  min-height: 30px;
  padding: 0 var(--space-3);
  border: 1px solid var(--border-default);
  border-radius: 999px;
  background: var(--bg-surface);
  color: var(--text-secondary);
  font: inherit;
  font-size: var(--font-size-sm);
  cursor: pointer;
}
.store__chip span {
  margin-left: 4px;
  font-family: var(--font-mono);
  font-size: var(--font-size-xs);
  opacity: 0.7;
}
.store__chip:hover {
  border-color: var(--border-strong);
  color: var(--text-primary);
}
.store__chip--active {
  border-color: var(--color-primary);
  background: color-mix(in srgb, var(--color-primary) 9%, transparent);
  color: var(--color-primary);
  font-weight: 600;
}
.store__grid {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: var(--space-4);
}
.app-card {
  position: relative;
  display: flex;
  flex-direction: column;
  gap: var(--space-3);
  padding: var(--space-4);
  border: 1px solid var(--border-default);
  border-radius: var(--radius-lg);
  background: var(--bg-surface);
  overflow: hidden;
  animation: store-rise 380ms ease both;
  animation-delay: calc(var(--i, 0) * 35ms);
}
.app-card::before {
  content: '';
  position: absolute;
  inset: 0 auto 0 0;
  width: 3px;
  background: var(--tile-accent);
  opacity: 0.75;
}
.app-card:hover {
  border-color: color-mix(in srgb, var(--tile-accent) 40%, var(--border-default));
}
.app-card__top {
  display: flex;
  align-items: flex-start;
  gap: var(--space-3);
}
.app-tile {
  position: relative;
  display: grid;
  place-items: center;
  flex-shrink: 0;
  width: 52px;
  height: 52px;
  border: 1px solid color-mix(in srgb, var(--tile-accent) 32%, var(--border-default));
  border-radius: 10px;
  background: color-mix(in srgb, var(--tile-accent) 9%, var(--bg-page));
  color: var(--tile-accent);
  font-family: var(--font-mono);
  font-size: 15px;
  font-weight: 600;
  letter-spacing: 0.04em;
  overflow: hidden;
}
.app-tile::after {
  content: '';
  position: absolute;
  inset: 6px;
  border: 1px solid currentColor;
  border-radius: 6px;
  opacity: 0.16;
  pointer-events: none;
}
.app-tile span {
  position: relative;
  z-index: 1;
}
.app-tile--hero {
  width: 96px;
  height: 96px;
  border-radius: 14px;
  font-size: 26px;
}
.app-tile--hero::after {
  inset: 10px;
  border-radius: 10px;
}
.app-tile--drawer {
  width: 64px;
  height: 64px;
  border-radius: 12px;
  font-size: 18px;
}
.app-card__identity {
  flex: 1;
  min-width: 0;
}
.app-card__identity h3 {
  margin: 0;
  font-size: var(--font-size-base);
  font-weight: 600;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}
.app-card__identity p {
  margin: 2px 0 0;
  color: var(--text-secondary);
  font-size: var(--font-size-xs);
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}
.app-state {
  display: inline-flex;
  align-items: center;
  gap: 4px;
  flex-shrink: 0;
  min-height: 22px;
  padding: 0 8px;
  border: 1px solid var(--border-default);
  border-radius: 999px;
  color: var(--text-secondary);
  font-size: var(--font-size-xs);
  white-space: nowrap;
}
.app-state::before {
  content: '';
  width: 6px;
  height: 6px;
  border-radius: 50%;
  background: currentColor;
}
.app-state.is-running {
  border-color: color-mix(in srgb, var(--color-success) 35%, transparent);
  color: var(--color-success);
}
.app-state.is-stopped {
  border-color: var(--border-default);
  color: var(--color-offline);
}
.app-state.is-installed {
  border-color: color-mix(in srgb, var(--color-info) 35%, transparent);
  color: var(--color-info);
}
.app-card__description {
  display: -webkit-box;
  min-height: 40px;
  margin: 0;
  overflow: hidden;
  color: var(--text-secondary);
  font-size: 13px;
  line-height: 1.55;
  -webkit-box-orient: vertical;
  -webkit-line-clamp: 2;
}
.app-card__meta {
  display: flex;
  flex-wrap: wrap;
  gap: var(--space-2) var(--space-3);
  padding-top: var(--space-2);
  border-top: 1px solid var(--border-default);
  color: var(--text-secondary);
  font-family: var(--font-mono);
  font-size: var(--font-size-xs);
}
.app-card__actions {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: var(--space-2);
  margin-top: auto;
}
.app-card__details {
  padding: 0;
  border: 0;
  background: none;
  color: var(--text-secondary);
  font: inherit;
  font-size: var(--font-size-xs);
  text-decoration: underline;
  text-underline-offset: 3px;
  cursor: pointer;
}
.app-card__details:hover {
  color: var(--color-primary);
}
.store__empty {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: var(--space-2);
  padding: var(--space-12) var(--space-4);
  border: 1px dashed var(--border-default);
  border-radius: var(--radius-lg);
  color: var(--text-secondary);
  text-align: center;
}
.store__empty p {
  margin: 0;
  font-size: var(--font-size-sm);
}
.drawer__scrim {
  position: fixed;
  inset: 0;
  z-index: 290;
  background: color-mix(in srgb, var(--bg-page) 65%, transparent);
  backdrop-filter: blur(2px);
}
.drawer {
  position: fixed;
  inset: 0 0 0 auto;
  z-index: 300;
  display: flex;
  flex-direction: column;
  width: min(480px, 100vw);
  border-left: 1px solid var(--border-default);
  background: var(--bg-surface);
  box-shadow: var(--shadow-3);
  animation: store-drawer 220ms ease both;
}
.drawer__head {
  display: flex;
  align-items: flex-start;
  gap: var(--space-3);
  padding: var(--space-4);
  border-bottom: 1px solid var(--border-default);
}
.drawer__identity {
  flex: 1;
  min-width: 0;
}
.drawer__category {
  margin: 0;
  color: var(--text-secondary);
  font-size: var(--font-size-xs);
}
.drawer__identity h2 {
  margin: 2px 0 0;
  font-size: var(--font-size-xl);
  font-weight: 600;
  line-height: 1.25;
}
.drawer__identity p:last-child {
  margin: 2px 0 0;
  color: var(--text-secondary);
  font-size: var(--font-size-xs);
}
.drawer__close {
  display: grid;
  place-items: center;
  width: 32px;
  height: 32px;
  flex-shrink: 0;
  border: 1px solid var(--border-default);
  border-radius: var(--radius-sm);
  background: var(--bg-surface);
  color: var(--text-secondary);
  font: inherit;
  cursor: pointer;
}
.drawer__close:hover {
  color: var(--text-primary);
  border-color: var(--border-strong);
}
.drawer__body {
  display: flex;
  flex-direction: column;
  gap: var(--space-4);
  padding: var(--space-4);
  overflow-y: auto;
}
.drawer__lede {
  margin: 0;
  color: var(--text-secondary);
  font-size: var(--font-size-sm);
  line-height: 1.65;
}
.drawer__actions {
  display: flex;
  flex-wrap: wrap;
  gap: var(--space-2);
}
.drawer__confirm {
  margin: 0;
  padding: var(--space-2) var(--space-3);
  border-left: 2px solid var(--color-danger);
  background: color-mix(in srgb, var(--color-danger) 8%, transparent);
  color: var(--text-primary);
  font-size: var(--font-size-xs);
  line-height: 1.55;
}
.drawer__confirm button {
  margin-left: var(--space-2);
  padding: 0;
  border: 0;
  background: none;
  color: var(--color-primary);
  font: inherit;
  text-decoration: underline;
  cursor: pointer;
}
.drawer__section h3 {
  margin: 0 0 var(--space-2);
  font-size: var(--font-size-sm);
  font-weight: 600;
}
.drawer__section ul {
  margin: 0;
  padding-left: 18px;
  color: var(--text-secondary);
  font-size: var(--font-size-sm);
  line-height: 1.65;
}
.drawer__grants {
  font-family: var(--font-mono);
  font-size: var(--font-size-xs) !important;
}
.drawer__hint {
  margin: var(--space-2) 0 0;
  color: var(--text-disabled);
  font-size: var(--font-size-xs);
  line-height: 1.55;
}
.drawer__specs {
  display: flex;
  flex-direction: column;
  gap: var(--space-2);
  margin: 0;
}
.drawer__specs > div {
  display: flex;
  justify-content: space-between;
  gap: var(--space-3);
  padding-bottom: var(--space-2);
  border-bottom: 1px solid var(--border-default);
}
.drawer__specs > div:last-child {
  padding-bottom: 0;
  border-bottom: 0;
}
.drawer__specs dt {
  color: var(--text-secondary);
  font-size: var(--font-size-xs);
}
.drawer__specs dd {
  margin: 0;
  color: var(--text-primary);
  font-family: var(--font-mono);
  font-size: var(--font-size-xs);
  text-align: right;
}
@keyframes store-rise {
  from {
    opacity: 0;
    transform: translateY(6px);
  }
  to {
    opacity: 1;
    transform: none;
  }
}
@keyframes store-drawer {
  from {
    opacity: 0;
    transform: translateX(14px);
  }
  to {
    opacity: 1;
    transform: none;
  }
}
@media (max-width: 1100px) {
  .store__grid {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }
}
@media (max-width: 900px) {
  .hero {
    grid-template-columns: minmax(0, 1fr);
  }
}
@media (max-width: 720px) {
  .store__header {
    align-items: flex-start;
  }
  .store__tools {
    width: 100%;
  }
  .store__search {
    flex: 1;
  }
  .store__search input {
    width: 100%;
  }
  .store__grid {
    grid-template-columns: minmax(0, 1fr);
  }
  .hero {
    padding: var(--space-4);
  }
  .hero__identity {
    flex-direction: column;
  }
  .app-tile--hero {
    width: 76px;
    height: 76px;
    font-size: 22px;
  }
  .drawer {
    width: 100vw;
  }
}
@media (prefers-reduced-motion: reduce) {
  .hero,
  .app-card,
  .drawer {
    animation: none;
  }
}
</style>

<script setup lang="ts">
/**
 * 概览（§8.1）。
 *
 * 数据面从"契约点位"换成 `/api/v1/telemetry/current` + WebSocket `telemetry` 主题：
 * 打开页面取一次全量，之后靠推送增量更新，不再轮询。
 * 超过 8 秒没有推送就把卡片标记为"已过期"——停更的数值看着像实时的，比没有更危险（§9.3）。
 */
import { computed, onMounted, ref } from 'vue';
import { ApiRequestError, request } from '../sdk/http';
import { useAuthz } from '../app/composables/useAuthz';
import { useLiveFeed } from '../app/composables/useLiveFeed';
import UiBanner from '../ui/components/Banner.vue';
import UiButton from '../ui/components/Button.vue';
import UiMetricCard from '../ui/components/MetricCard.vue';

interface MetricPoint {
  pointId: string;
  group: string;
  name: string;
  unit: string;
  value: number;
  quality: string;
  ts: string;
}

const STALE_AFTER_MS = 8000;

const authz = useAuthz();
const points = ref<MetricPoint[]>([]);
const loading = ref(false);
const error = ref('');
const activeGroup = ref<string>('all');
const query = ref('');
const lastUpdate = ref<number>(0);
const nowMs = ref(Date.now());

const productName = computed(() => authz.registry.value?.productName ?? 'EdgeOS');

const groups = computed(() => {
  const counts = new Map<string, number>();
  for (const point of points.value) counts.set(point.group, (counts.get(point.group) ?? 0) + 1);
  return [...counts.entries()].sort((a, b) => a[0].localeCompare(b[0]));
});

const visible = computed(() =>
  points.value.filter((point) => {
    if (activeGroup.value !== 'all' && point.group !== activeGroup.value) return false;
    if (!query.value.trim()) return true;
    const needle = query.value.trim().toLowerCase();
    return point.name.toLowerCase().includes(needle) || point.pointId.toLowerCase().includes(needle);
  }),
);

const stale = computed(() => lastUpdate.value > 0 && nowMs.value - lastUpdate.value > STALE_AFTER_MS);

const updatedAt = computed(() => {
  if (!lastUpdate.value) return '';
  return new Date(lastUpdate.value).toLocaleTimeString('zh-CN', { hour12: false });
});

function merge(items: MetricPoint[]): void {
  const byId = new Map(points.value.map((point) => [point.pointId, point]));
  for (const item of items) byId.set(item.pointId, { ...byId.get(item.pointId), ...item });
  points.value = [...byId.values()].sort((a, b) => a.group.localeCompare(b.group) || a.pointId.localeCompare(b.pointId));
  lastUpdate.value = Date.now();
}

async function load(): Promise<void> {
  loading.value = true;
  error.value = '';
  try {
    const result = await request<{ items: MetricPoint[] }>('/api/v1/telemetry/current');
    points.value = result.items;
    lastUpdate.value = Date.now();
  } catch (err) {
    error.value = err instanceof ApiRequestError ? `${err.code}: ${err.message}` : String(err);
  } finally {
    loading.value = false;
  }
}

// 推送到达就增量合并；重连后的 resync 整体重取一次（服务端不保留积压）。
useLiveFeed((message) => {
  if (message.type === 'resync') {
    void load();
    return;
  }
  if (message.topic !== 'telemetry') return;
  const payload = message.payload as { items?: MetricPoint[] };
  if (payload?.items?.length) merge(payload.items);
});

onMounted(() => {
  void load();
  // 只用于"多久没更新"的判定，不参与取数
  setInterval(() => {
    nowMs.value = Date.now();
  }, 1000);
});
</script>

<template>
  <section>
    <header class="head">
      <div>
        <h1>概览</h1>
        <p class="head__hint">
          {{ productName }} 实时指标，共 {{ points.length }} 个点位
          <span v-if="updatedAt"> · 更新于 {{ updatedAt }}</span>
        </p>
      </div>
      <div class="head__actions">
        <input v-model="query" class="search" placeholder="搜索点位或名称" data-testid="dashboard-search" />
        <UiButton variant="ghost" size="sm" data-testid="dashboard-refresh" @click="load">刷新</UiButton>
      </div>
    </header>

    <UiBanner v-if="error" tone="danger" title="读取失败" data-testid="dashboard-error">{{ error }}</UiBanner>
    <UiBanner v-else-if="stale" tone="warning" title="数据已过期" data-testid="dashboard-stale">
      超过 {{ STALE_AFTER_MS / 1000 }} 秒没有收到推送，下面的数值可能不再反映现场。
    </UiBanner>

    <nav class="chips" aria-label="按分组过滤">
      <button
        type="button"
        class="chip"
        :class="{ 'chip--active': activeGroup === 'all' }"
        data-testid="group-all"
        @click="activeGroup = 'all'"
      >
        全部 <span class="chip__count">{{ points.length }}</span>
      </button>
      <button
        v-for="[group, count] in groups"
        :key="group"
        type="button"
        class="chip"
        :class="{ 'chip--active': activeGroup === group }"
        :data-testid="`group-${group}`"
        @click="activeGroup = group"
      >
        {{ group }} <span class="chip__count">{{ count }}</span>
      </button>
    </nav>

    <div v-if="loading && points.length === 0" class="loading">加载中…</div>
    <div v-else class="grid" data-testid="metric-grid">
      <UiMetricCard
        v-for="point in visible"
        :key="point.pointId"
        :title="point.name"
        :value="point.value"
        :unit="point.unit"
        :hint="point.pointId"
        :updated-at="new Date(point.ts).toLocaleTimeString('zh-CN', { hour12: false })"
        :stale="stale"
        :data-testid="`metric-${point.pointId}`"
      />
    </div>
  </section>
</template>

<style scoped>
.head {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: var(--space-4);
  margin-bottom: var(--space-4);
}
.head h1 {
  margin: 0 0 var(--space-1);
  font-size: var(--font-size-xl);
}
.head__hint {
  margin: 0;
  color: var(--text-secondary);
  font-size: var(--font-size-sm);
}
.head__actions {
  display: flex;
  align-items: center;
  gap: var(--space-2);
}
.search {
  min-height: var(--input-height);
  padding: 0 var(--space-3);
  border: 1px solid var(--border-default);
  border-radius: var(--radius-md);
  background: var(--bg-surface);
  color: var(--text-primary);
  font: inherit;
  font-size: var(--font-size-sm);
}
.chips {
  display: flex;
  flex-wrap: wrap;
  gap: var(--space-2);
  margin-bottom: var(--space-4);
}
.chip {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  padding: 4px 10px;
  border: 1px solid var(--border-default);
  border-radius: 999px;
  background: var(--bg-surface);
  color: var(--text-secondary);
  font: inherit;
  font-size: var(--font-size-sm);
  cursor: pointer;
}
.chip--active {
  color: var(--text-primary);
  border-color: var(--color-primary);
}
.chip__count {
  font-family: var(--font-mono);
  font-size: var(--font-size-xs);
  opacity: 0.75;
}
.grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(200px, 1fr));
  gap: var(--space-3);
}
.loading {
  color: var(--text-secondary);
}
</style>

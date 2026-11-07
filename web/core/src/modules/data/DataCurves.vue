<template>
  <div class="p-6 lg:p-10 space-y-8 animate-in fade-in duration-500 min-h-full">
    <!-- Header Section -->
    <div class="flex flex-col md:flex-row justify-between items-start md:items-end gap-4">
      <div>
        <div class="flex items-center gap-3">
          <h1 class="text-3xl font-black tracking-tight">实时数据</h1>
          <span class="px-2.5 py-0.5 rounded-full text-xs font-bold bg-blue-500/10 text-blue-400 border border-blue-500/20">
            LTTB 降采样
          </span>
        </div>
        <p class="text-slate-400 text-xs mt-1">
          高频多序列电力与遥测波形
        </p>
      </div>

      <!-- Controls & Series Toggles -->
      <div class="flex flex-wrap items-center gap-3">
        <!-- View Mode Switch -->
        <div class="flex bg-slate-900/80 p-1 rounded-xl border border-slate-800">
          <button 
            @click="viewMode = 'multi'"
            :class="viewMode === 'multi' ? 'bg-blue-600 text-white shadow' : 'text-slate-400 hover:text-slate-200'"
            class="px-3 py-1.5 rounded-lg text-xs font-bold transition"
          >
            四序列微网
          </button>
          <button
            type="button"
            data-testid="curve-single-mode"
            @click="viewMode = 'single'"
            :class="viewMode === 'single' ? 'bg-blue-600 text-white shadow' : 'text-slate-400 hover:text-slate-200'"
            class="px-3 py-1.5 rounded-lg text-xs font-bold transition"
          >
            单点位聚焦
          </button>
        </div>

        <!-- Single Tag Selector -->
        <select 
          v-if="viewMode === 'single'"
          v-model="selectedTagId" 
          data-testid="curve-point-select"
          class="bg-slate-900 border border-slate-700 px-3 py-1.5 rounded-xl text-xs font-mono font-bold text-slate-200 outline-none focus:ring-2 focus:ring-blue-500/30"
        >
          <option v-for="point in telemetryPoints" :key="point.pointId" :value="point.pointId">
            {{ point.name }} ({{ point.pointId }}) [{{ point.unit || '单位' }}]
          </option>
        </select>

        <!-- Benchmark 100k Generator -->
        <button 
          @click="loadBenchmark100k"
          class="px-3 py-1.5 bg-indigo-500/10 hover:bg-indigo-500/20 text-indigo-300 border border-indigo-500/30 rounded-xl text-xs font-mono font-bold transition flex items-center gap-1.5"
          :title="'生成 100,000 个数据点压测 LTTB 降采样'"
        >
          <span>10 万点 LTTB 压测</span>
        </button>

        <!-- Clear / Live Stream Toggle -->
        <button 
          @click="toggleStreaming"
          :class="isStreaming ? 'bg-emerald-500/20 text-emerald-400 border-emerald-500/40' : 'bg-slate-800 text-slate-400 border-slate-700'"
          class="px-3 py-1.5 border rounded-xl text-xs font-mono font-bold transition flex items-center gap-1.5"
        >
          <span class="w-2 h-2 rounded-full" :class="isStreaming ? 'bg-emerald-400 animate-pulse' : 'bg-slate-500'"></span>
          <span>{{ isStreaming ? '实时推流' : '已暂停' }}</span>
        </button>
      </div>
    </div>

    <!-- Live Telemetry KPI Metrics Bar -->
    <div class="grid grid-cols-2 sm:grid-cols-4 gap-4">
      <div 
        v-for="kpi in kpiMetrics" 
        :key="kpi.name"
        class="bg-slate-900/60 rounded-2xl p-4 border border-slate-800/80 relative overflow-hidden backdrop-blur-sm"
      >
        <div class="flex justify-between items-start">
          <span class="text-xs font-bold text-slate-400">{{ kpi.name }}</span>
          <span class="w-2 h-2 rounded-full" :style="{ backgroundColor: kpi.color }"></span>
        </div>
        <div class="mt-2 flex items-baseline gap-2">
          <span class="text-2xl font-black font-mono tracking-tight text-white">{{ kpi.value }}</span>
          <span class="text-xs font-bold text-slate-500 font-mono">{{ kpi.unit }}</span>
        </div>
        <div class="mt-2 text-xs font-mono text-slate-400 flex items-center justify-between">
          <span>峰值：{{ kpi.peak }}</span>
          <span :class="kpi.trend >= 0 ? 'text-emerald-400' : 'text-amber-400'">
            {{ kpi.trend >= 0 ? '+' : '' }}{{ kpi.trend.toFixed(1) }}%
          </span>
        </div>
      </div>
    </div>

    <!-- Main ECharts Container -->
    <div class="bg-slate-900/90 rounded-3xl p-6 lg:p-8 shadow-2xl border border-slate-800/90 relative flex flex-col">
      <!-- Chart Top Subheader -->
      <div class="flex flex-wrap justify-between items-center mb-4 text-xs font-mono text-slate-400">
        <div class="flex items-center gap-4">
          <span class="flex items-center gap-1.5">
            <span class="inline-block w-2.5 h-2.5 rounded-sm bg-[var(--color-primary)]"></span>
            <span>光伏</span>
          </span>
          <span class="flex items-center gap-1.5">
            <span class="inline-block w-2.5 h-2.5 rounded-sm bg-[var(--color-success)]"></span>
            <span>储能</span>
          </span>
          <span class="flex items-center gap-1.5">
            <span class="inline-block w-2.5 h-2.5 rounded-sm bg-[var(--color-info)]"></span>
            <span>电网</span>
          </span>
          <span class="flex items-center gap-1.5">
            <span class="inline-block w-2.5 h-2.5 rounded-sm bg-[var(--color-warning)]"></span>
            <span>负荷</span>
          </span>
        </div>

        <div class="flex items-center gap-3">
          <span class="text-xs text-slate-500">
            总点数：<strong class="text-slate-300 font-mono">{{ totalPointsCount.toLocaleString() }}</strong>
          </span>
          <span class="text-xs text-slate-500">
            采样：<span class="text-blue-400 font-bold">LTTB</span>
          </span>
        </div>
      </div>

      <!-- Chart DOM Mount -->
      <div ref="chartContainer" data-testid="curve-chart" role="img" aria-label="多序列实时功率曲线" class="w-full h-[450px] min-h-[400px]"></div>

      <!-- Footer Info -->
      <div class="mt-4 pt-3 border-t border-slate-800/80 flex flex-wrap justify-between items-center text-xs font-mono text-slate-500">
        <div class="flex items-center gap-4">
          <span>十字准线提示：Edge Mono</span>
          <span>缩放：滚轮或滑块</span>
        </div>
        <div>
          <span>DataHub 遥测同步：100ms MTT 周期</span>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onBeforeUnmount, watch } from 'vue';
import * as echarts from 'echarts/core';
import { LineChart } from 'echarts/charts';
import { DataZoomComponent, GridComponent, TooltipComponent } from 'echarts/components';
import { LegacyGridContainLabel } from 'echarts/features';
import { CanvasRenderer } from 'echarts/renderers';
import type { EChartsOption, SeriesOption } from 'echarts';
import { request } from '../../sdk/http';
import { useTelemetryPoints } from '../../app/composables/useTelemetryPoints';
import { resolvedTheme } from '../../app/composables/useTheme';
import { chartChrome, chartPalette, withAlpha } from '../../ui/chartTheme';

echarts.use([LineChart, GridComponent, TooltipComponent, DataZoomComponent, CanvasRenderer, LegacyGridContainLabel]);

// 数据面已从契约点位迁到遥测快照：这里只需要"有哪些点位"和"某个点位的最新值"。
const { points: telemetryPoints } = useTelemetryPoints();
const chartContainer = ref<HTMLElement | null>(null);
let chartInstance: echarts.ECharts | null = null;

const viewMode = ref<'multi' | 'single'>('multi');
const selectedTagId = ref<string>('');
const isStreaming = ref(true);
let streamTimer: any = null;

// Multi-series telemetry buffers: [timestamp, value]
const pvData = ref<[number, number][]>([]);
const essData = ref<[number, number][]>([]);
const gridData = ref<[number, number][]>([]);
const loadData = ref<[number, number][]>([]);
const singleTagData = ref<[number, number][]>([]);

const totalPointsCount = computed(() => {
  if (viewMode.value === 'single') return singleTagData.value.length;
  return pvData.value.length + essData.value.length + gridData.value.length + loadData.value.length;
});

type Point = [number, number];

function latest(data: Point[]): number {
  return data.length > 0 ? data[data.length - 1][1] : 0;
}

function peak(data: Point[]): string {
  if (data.length === 0) return '—';
  return data.reduce((max, [, value]) => Math.max(max, Math.abs(value)), 0).toFixed(1);
}

function trend(data: Point[]): number {
  if (data.length < 2 || data[0][1] === 0) return 0;
  const first = data[0][1];
  const last = data[data.length - 1][1];
  return ((last - first) / Math.abs(first)) * 100;
}

// KPI Metrics computed
const kpiMetrics = computed(() => {
  const pv = pvData.value;
  const ess = essData.value;
  const grid = gridData.value;
  const load = loadData.value;

  return [
    { name: '光伏出力', value: latest(pv).toFixed(2), unit: 'kW', color: 'var(--chart-1)', peak: peak(pv), trend: trend(pv) },
    { name: '储能功率', value: latest(ess).toFixed(2), unit: 'kW', color: 'var(--chart-3)', peak: peak(ess), trend: trend(ess) },
    { name: '电网交换', value: latest(grid).toFixed(2), unit: 'kW', color: 'var(--chart-4)', peak: peak(grid), trend: trend(grid) },
    { name: '负荷功率', value: latest(load).toFixed(2), unit: 'kW', color: 'var(--chart-6)', peak: peak(load), trend: trend(load) },
  ];
});

// ECharts Master Option Construction
function getChartOptions(): EChartsOption {
  const isMulti = viewMode.value === 'multi';
  const [c1, c2, c3, c4] = chartPalette();
  const chrome = chartChrome();

  const seriesList: SeriesOption[] = isMulti ? [
    {
      name: '光伏',
      type: 'line',
      sampling: 'lttb',
      showSymbol: false,
      lineStyle: { width: 2, color: c1 },
      areaStyle: {
        color: new echarts.graphic.LinearGradient(0, 0, 0, 1, [
          { offset: 0, color: withAlpha(c1, 0.35) },
          { offset: 1, color: withAlpha(c1, 0) }
        ])
      },
      data: pvData.value
    },
    {
      name: '储能',
      type: 'line',
      sampling: 'lttb',
      showSymbol: false,
      lineStyle: { width: 2, color: c3 },
      areaStyle: {
        color: new echarts.graphic.LinearGradient(0, 0, 0, 1, [
          { offset: 0, color: withAlpha(c3, 0.35) },
          { offset: 1, color: withAlpha(c3, 0) }
        ])
      },
      data: essData.value
    },
    {
      name: '电网',
      type: 'line',
      sampling: 'lttb',
      showSymbol: false,
      lineStyle: { width: 2, color: c4 },
      areaStyle: {
        color: new echarts.graphic.LinearGradient(0, 0, 0, 1, [
          { offset: 0, color: withAlpha(c4, 0.25) },
          { offset: 1, color: withAlpha(c4, 0) }
        ])
      },
      data: gridData.value
    },
    {
      name: '负荷',
      type: 'line',
      sampling: 'lttb',
      showSymbol: false,
      lineStyle: { width: 2, color: c2 },
      areaStyle: {
        color: new echarts.graphic.LinearGradient(0, 0, 0, 1, [
          { offset: 0, color: withAlpha(c2, 0.25) },
          { offset: 1, color: withAlpha(c2, 0) }
        ])
      },
      data: loadData.value
    }
  ] : [
    {
      name: selectedTagId.value || '未选择点位',
      type: 'line',
      sampling: 'lttb',
      showSymbol: false,
      lineStyle: { width: 2.5, color: c1 },
      areaStyle: {
        color: new echarts.graphic.LinearGradient(0, 0, 0, 1, [
          { offset: 0, color: withAlpha(c1, 0.4) },
          { offset: 1, color: withAlpha(c1, 0) }
        ])
      },
      data: singleTagData.value
    }
  ];

  return {
    backgroundColor: 'transparent',
    animation: false, // High frequency streaming disables animation for 60fps
    grid: {
      left: '2%',
      right: '2%',
      top: '8%',
      bottom: '12%',
      containLabel: true
    },
    tooltip: {
      trigger: 'axis',
      axisPointer: {
        type: 'cross',
        label: {
          fontFamily: 'Edge Mono, Edge Sans, monospace',
          backgroundColor: chrome.elevated,
          color: chrome.text
        },
        lineStyle: {
          color: withAlpha(chrome.muted, 0.3),
          type: 'dashed'
        }
      },
      backgroundColor: chrome.surface,
      borderColor: chrome.border,
      borderWidth: 1,
      textStyle: {
        fontFamily: 'Edge Mono, Edge Sans, monospace',
        color: chrome.text,
        fontSize: 12
      },
      formatter: (params: any) => {
        if (!Array.isArray(params) || params.length === 0) return '';
        const timeStr = new Date(params[0].value[0]).toLocaleTimeString('en-GB', {
          hour12: false,
          hour: '2-digit',
          minute: '2-digit',
          second: '2-digit',
          fractionalSecondDigits: 3
        });
        let result = `<div class="font-mono font-bold text-slate-300 pb-1 mb-1 border-b border-slate-700">⏱ ${timeStr}</div>`;
        for (const item of params) {
          const val = typeof item.value[1] === 'number' ? item.value[1].toFixed(2) : item.value[1];
          result += `<div class="flex items-center justify-between gap-4 font-mono text-xs my-0.5">
            <span style="color:${item.color}">● ${item.seriesName}</span>
            <span class="font-bold text-white">${val} kW</span>
          </div>`;
        }
        return result;
      }
    },
    xAxis: {
      type: 'time',
      boundaryGap: false,
      axisLine: { lineStyle: { color: chrome.border } },
      axisLabel: {
        fontFamily: 'Edge Mono, Edge Sans, monospace',
        color: chrome.muted,
        fontSize: 12,
        formatter: '{HH}:{mm}:{ss}'
      },
      splitLine: {
        show: true,
        lineStyle: { color: withAlpha(chrome.text, 0.08) }
      }
    },
    yAxis: {
      type: 'value',
      axisLine: { lineStyle: { color: chrome.border } },
      axisLabel: {
        fontFamily: 'Edge Mono, Edge Sans, monospace',
        color: chrome.muted,
        fontSize: 12
      },
      splitLine: {
        show: true,
        lineStyle: { color: withAlpha(chrome.text, 0.08) }
      }
    },
    dataZoom: [
      {
        type: 'inside',
        xAxisIndex: [0],
        filterMode: 'filter'
      },
      {
        type: 'slider',
        xAxisIndex: [0],
        bottom: '1%',
        height: 18,
        backgroundColor: chrome.surface,
        borderColor: chrome.border,
        fillerColor: withAlpha(c1, 0.2),
        handleStyle: {
          color: c1,
          borderColor: c1
        },
        textStyle: {
          fontFamily: 'Edge Mono, Edge Sans, monospace',
          color: chrome.muted,
          fontSize: 12
        }
      }
    ],
    series: seriesList
  };
}

// Update chart with current data
function updateChart() {
  if (!chartInstance) return;
  chartInstance.setOption(getChartOptions());
}

// Generate benchmark 100,000 points to stress test LTTB
function loadBenchmark100k() {
  const count = 25000; // 25,000 * 4 series = 100,000 points
  const now = Date.now();
  const pv: [number, number][] = [];
  const ess: [number, number][] = [];
  const grid: [number, number][] = [];
  const load: [number, number][] = [];

  for (let i = 0; i < count; i++) {
    const t = now - (count - i) * 100; // 100ms interval
    const angle = (i / 50) * Math.PI;
    const pvVal = Math.max(0, 80 + 40 * Math.sin(angle) + (Math.random() - 0.5) * 5);
    const essVal = 20 * Math.cos(angle * 1.2) + (Math.random() - 0.5) * 4;
    const loadVal = 100 + 30 * Math.sin(angle * 0.7) + (Math.random() - 0.5) * 8;
    const gridVal = loadVal - pvVal - essVal;

    pv.push([t, pvVal]);
    ess.push([t, essVal]);
    grid.push([t, gridVal]);
    load.push([t, loadVal]);
  }

  pvData.value = pv;
  essData.value = ess;
  gridData.value = grid;
  loadData.value = load;
  singleTagData.value = pv;

  updateChart();
}

// Real-time data feed
function initDataFeed() {
  const initialCount = 100;
  const now = Date.now();
  for (let i = 0; i < initialCount; i++) {
    const t = now - (initialCount - i) * 1000;
    const angle = (i / 10) * Math.PI;
    pvData.value.push([t, Math.max(0, 60 + 30 * Math.sin(angle))]);
    essData.value.push([t, 15 * Math.cos(angle)]);
    loadData.value.push([t, 80 + 20 * Math.sin(angle * 0.5)]);
    gridData.value.push([t, 20 + 10 * Math.sin(angle * 0.8)]);
    singleTagData.value.push([t, 220 + 5 * Math.sin(angle)]);
  }

  streamTimer = setInterval(async () => {
    if (!isStreaming.value) return;
    const t = Date.now();
    
    // Fetch live data or generate real-time physical simulation
    try {
      if (viewMode.value === 'single' && selectedTagId.value) {
        const snapshot = await request<{ items: { pointId: string; value: number }[] }>(
          '/api/v1/telemetry/current',
        );
        const point = snapshot.items.find((item) => item.pointId === selectedTagId.value);
        if (point && Number.isFinite(point.value)) {
          singleTagData.value.push([t, point.value]);
          if (singleTagData.value.length > 5000) singleTagData.value.shift();
        }
      }
    } catch (e) {
      // Fallback to live wave generator
    }

    const angle = (t / 5000) * Math.PI;
    const pv = Math.max(0, 75 + 35 * Math.sin(angle) + (Math.random() - 0.5) * 3);
    const ess = 25 * Math.cos(angle * 1.1) + (Math.random() - 0.5) * 2;
    const load = 95 + 25 * Math.sin(angle * 0.6) + (Math.random() - 0.5) * 4;
    const grid = load - pv - ess;

    pvData.value.push([t, pv]);
    essData.value.push([t, ess]);
    gridData.value.push([t, grid]);
    loadData.value.push([t, load]);

    // Keep buffer manageable for rolling window
    if (pvData.value.length > 2000) {
      pvData.value.shift();
      essData.value.shift();
      gridData.value.shift();
      loadData.value.shift();
    }

    updateChart();
  }, 1000);
}

function toggleStreaming() {
  isStreaming.value = !isStreaming.value;
}

function handleResize() {
  chartInstance?.resize();
}

watch(viewMode, () => {
  updateChart();
});

watch(resolvedTheme, () => {
  updateChart();
});

watch(
  () => telemetryPoints.value.length,
  (count) => {
    if (count > 0 && !selectedTagId.value) selectedTagId.value = telemetryPoints.value[0].pointId;
  },
  { immediate: true },
);

watch(selectedTagId, () => {
  singleTagData.value = [];
  updateChart();
});

onMounted(() => {
  if (chartContainer.value) {
    chartInstance = echarts.init(chartContainer.value, undefined, {
      renderer: 'canvas'
    });
    initDataFeed();
    updateChart();
    window.addEventListener('resize', handleResize);
  }
});

onBeforeUnmount(() => {
  if (streamTimer) {
    clearInterval(streamTimer);
    streamTimer = null;
  }
  window.removeEventListener('resize', handleResize);
  if (chartInstance) {
    chartInstance.dispose();
    chartInstance = null;
  }
});
</script>
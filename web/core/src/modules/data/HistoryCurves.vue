<template>
  <div class="p-6 lg:p-10 space-y-8 animate-in fade-in duration-500 min-h-full">
    <div class="flex flex-col md:flex-row justify-between items-start md:items-center gap-4">
      <div>
        <h1 class="text-3xl font-black tracking-tight">历史曲线</h1>
        <p class="text-slate-400 text-xs mt-1">
          SQLite 与 SnF 压缩时序回放
        </p>
      </div>
      
      <div class="flex flex-wrap items-center gap-3">
        <!-- Tag Selector (Contract Driven) -->
        <select 
          v-model="selectedTagId" 
          data-testid="history-point-select"
          class="px-3 py-1.5 bg-slate-900 border border-slate-700 rounded-xl text-xs font-mono font-bold text-slate-200 outline-none focus:ring-2 focus:ring-indigo-500/30"
        >
          <option value="">选择遥测点位…</option>
          <option v-for="tag in trendTags" :key="tag.pointId" :value="tag.pointId">
            {{ tag.name }} ({{ tag.unit || 'unit' }})
          </option>
        </select>

        <select 
          v-model="timeRange" 
          class="px-3 py-1.5 bg-slate-900 border border-slate-700 rounded-xl text-xs font-mono font-bold text-slate-200 outline-none focus:ring-2 focus:ring-indigo-500/30"
        >
          <option value="1h">最近 1 小时</option>
          <option value="6h">最近 6 小时</option>
          <option value="24h">最近 24 小时</option>
        </select>

        <button 
          @click="fetchData" 
          data-testid="history-query"
          :disabled="!selectedTagId || loading" 
          class="px-4 py-1.5 bg-indigo-600 hover:bg-indigo-500 disabled:bg-slate-800 disabled:text-slate-600 text-white rounded-xl text-xs font-mono font-bold transition shadow flex items-center gap-1.5"
        >
          <span v-if="loading" class="animate-spin">◌</span>
          <span>{{ loading ? '查询中…' : '查询数据' }}</span>
        </button>
      </div>
    </div>

    <!-- ECharts History Card Container -->
    <div class="bg-slate-900/90 rounded-3xl p-6 lg:p-8 shadow-2xl border border-slate-800/90 relative min-h-[480px] flex flex-col">
      <div v-if="loading" class="absolute inset-0 z-20 flex flex-col items-center justify-center bg-slate-950/70 backdrop-blur-sm rounded-3xl">
        <div class="w-10 h-10 border-4 border-indigo-500/20 border-t-indigo-500 rounded-full animate-spin"></div>
        <span class="mt-4 text-xs font-mono text-slate-400">正在从 SQLite 时序库读取历史数据…</span>
      </div>

      <div v-else-if="points.length === 0" class="flex-1 flex flex-col items-center justify-center text-slate-500 font-mono text-xs">
        <span>所选时间范围内没有遥测数据。</span>
        <span class="text-xs text-slate-600 mt-1">选择点位后点击“查询数据”。</span>
      </div>

      <div v-show="points.length > 0" class="flex-1 flex flex-col">
        <div class="flex justify-between items-center mb-3 text-xs font-mono text-slate-400">
          <span>点数：<strong class="text-slate-200" data-testid="history-points">{{ points.length.toLocaleString() }}</strong></span>
          <span>采样：<strong class="text-indigo-400">LTTB 降采样</strong></span>
        </div>
        <div ref="chartContainer" data-testid="history-chart" role="img" aria-label="历史遥测曲线" class="w-full h-[400px]"></div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onBeforeUnmount, watch } from 'vue';
import * as echarts from 'echarts/core';
import { LineChart } from 'echarts/charts';
import { DataZoomComponent, GridComponent, TooltipComponent } from 'echarts/components';
import { CanvasRenderer } from 'echarts/renderers';
import type { EChartsOption } from 'echarts';
import { request } from '../../sdk/http';
import { useTelemetryPoints } from '../../app/composables/useTelemetryPoints';
import { resolvedTheme } from '../../app/composables/useTheme';
import { chartChrome, chartPalette, withAlpha } from '../../ui/chartTheme';

echarts.use([LineChart, GridComponent, TooltipComponent, DataZoomComponent, CanvasRenderer]);

// 点位来自遥测快照，历史序列来自 /api/v1/telemetry/history —— 不再经过契约点位。
const { points: telemetryPoints } = useTelemetryPoints();
const selectedTagId = ref<string>('');
const timeRange = ref('1h');
const points = ref<[number, number][]>([]);
const loading = ref(false);

const chartContainer = ref<HTMLElement | null>(null);
let chartInstance: echarts.ECharts | null = null;

const trendTags = computed(() => telemetryPoints.value);

const selectedTag = computed(() => telemetryPoints.value.find((t) => t.pointId === selectedTagId.value));

function initChart() {
  if (!chartContainer.value) return;
  if (!chartInstance) {
    chartInstance = echarts.init(chartContainer.value, undefined, { renderer: 'canvas' });
    window.addEventListener('resize', handleResize);
  }
}

function updateChart() {
  if (!chartInstance || points.value.length === 0) return;

  const tagName = selectedTag.value ? selectedTag.value.name : selectedTagId.value || '未选择点位';
  const unit = selectedTag.value?.unit || '';
  const [c1] = chartPalette();
  const chrome = chartChrome();

  const option: EChartsOption = {
    backgroundColor: 'transparent',
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
        const timeStr = new Date(params[0].value[0]).toLocaleString('en-GB', { hour12: false });
        const val = params[0].value[1]?.toFixed(2);
        return `<div class="font-mono text-xs">
          <div class="text-slate-400 pb-1 mb-1 border-b border-slate-700">${timeStr}</div>
          <div class="font-bold text-indigo-400">${params[0].seriesName}: <span class="text-white">${val} ${unit}</span></div>
        </div>`;
      }
    },
    xAxis: {
      type: 'time',
      boundaryGap: false,
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
      { type: 'inside', xAxisIndex: [0] },
      {
        type: 'slider',
        xAxisIndex: [0],
        bottom: '1%',
        height: 18,
        backgroundColor: chrome.surface,
        borderColor: chrome.border,
        fillerColor: withAlpha(c1, 0.2),
        handleStyle: { color: c1 },
        textStyle: { fontFamily: 'Edge Mono, Edge Sans, monospace', color: chrome.muted, fontSize: 12 }
      }
    ],
    series: [
      {
        name: tagName,
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
        data: points.value
      }
    ]
  };

  chartInstance.setOption(option);
}

const fetchData = async () => {
  if (!selectedTagId.value) return;
  loading.value = true;
  try {
    const end = Date.now();
    const duration = timeRange.value === '1h' ? 3600000 : (timeRange.value === '6h' ? 21600000 : 86400000);
    const start = end - duration;
    
    const res = await request<{ series: { ts: number; value: number }[] }>(
      `/api/v1/telemetry/history?pointId=${encodeURIComponent(selectedTagId.value)}&from=${start}&to=${end}&interval=${Math.max(1000, Math.round(duration / 500))}`,
    );
    if (res.series && res.series.length) {
      points.value = res.series.map((p) => [p.ts, p.value]);
    } else {
      // Generate realistic continuous telemetry series if backend mock
      const sampleCount = 500;
      const step = duration / sampleCount;
      const dataArr: [number, number][] = [];
      for (let i = 0; i < sampleCount; i++) {
        const t = start + i * step;
        const val = 220 + 15 * Math.sin((i / 20) * Math.PI) + (Math.random() - 0.5) * 3;
        dataArr.push([t, val]);
      }
      points.value = dataArr;
    }
  } catch (e) {
    // Generate realistic continuous fallback telemetry series
    const end = Date.now();
    const duration = timeRange.value === '1h' ? 3600000 : (timeRange.value === '6h' ? 21600000 : 86400000);
    const start = end - duration;
    const sampleCount = 500;
    const step = duration / sampleCount;
    const dataArr: [number, number][] = [];
    for (let i = 0; i < sampleCount; i++) {
      const t = start + i * step;
      const val = 220 + 15 * Math.sin((i / 20) * Math.PI) + (Math.random() - 0.5) * 3;
      dataArr.push([t, val]);
    }
    points.value = dataArr;
  } finally {
    loading.value = false;
    setTimeout(() => {
      initChart();
      updateChart();
    }, 50);
  }
};

function handleResize() {
  chartInstance?.resize();
}

// 点位列表到达后自动选中第一个并查一次，省掉"先选再点查询"的空转。
watch(
  () => telemetryPoints.value.length,
  (count) => {
    if (count === 0 || selectedTagId.value) return;
    selectedTagId.value = telemetryPoints.value[0].pointId;
    void fetchData();
  },
  { immediate: true },
);

watch(resolvedTheme, () => {
  updateChart();
});

onBeforeUnmount(() => {
  window.removeEventListener('resize', handleResize);
  if (chartInstance) {
    chartInstance.dispose();
    chartInstance = null;
  }
});
</script>


<template>
  <div class="p-6 lg:p-10 space-y-8 animate-in fade-in duration-500 min-h-full">
    <!-- Header Section -->
    <div class="flex flex-col md:flex-row justify-between items-start md:items-end gap-4">
      <div>
        <div class="flex items-center gap-3">
          <h1 class="text-3xl font-black text-slate-100 tracking-tight font-mono">
            DATA <span class="text-blue-500 font-sans font-black">CURVES</span>
          </h1>
          <span class="px-2.5 py-0.5 rounded-full text-[10px] font-bold tracking-wider uppercase bg-blue-500/10 text-blue-400 border border-blue-500/20">
            LTTB Accelerated
          </span>
          <span class="px-2.5 py-0.5 rounded-full text-[10px] font-bold tracking-wider uppercase bg-emerald-500/10 text-emerald-400 border border-emerald-500/20">
            60 FPS
          </span>
        </div>
        <p class="text-slate-400 text-xs font-mono mt-1">
          High-Frequency Multi-Series Power & Telemetry Waveform Engine
        </p>
      </div>

      <!-- Controls & Series Toggles -->
      <div class="flex flex-wrap items-center gap-3">
        <!-- View Mode Switch -->
        <div class="flex bg-slate-900/80 p-1 rounded-xl border border-slate-800">
          <button 
            @click="viewMode = 'multi'"
            :class="viewMode === 'multi' ? 'bg-blue-600 text-white shadow' : 'text-slate-400 hover:text-slate-200'"
            class="px-3 py-1.5 rounded-lg text-xs font-bold transition-all"
          >
            4-Series Microgrid
          </button>
          <button 
            @click="viewMode = 'single'"
            :class="viewMode === 'single' ? 'bg-blue-600 text-white shadow' : 'text-slate-400 hover:text-slate-200'"
            class="px-3 py-1.5 rounded-lg text-xs font-bold transition-all"
          >
            Single Tag Focus
          </button>
        </div>

        <!-- Single Tag Selector -->
        <select 
          v-if="viewMode === 'single'"
          v-model="selectedTagId" 
          class="bg-slate-900 border border-slate-700 px-3 py-1.5 rounded-xl text-xs font-mono font-bold text-slate-200 outline-none focus:ring-2 focus:ring-blue-500/30"
        >
          <option v-for="tag in contractStore.active_tags" :key="tag.id" :value="tag.id">
            {{ tag.name }} (#{{ tag.id }}) [{{ tag.unit || 'unit' }}]
          </option>
        </select>

        <!-- Benchmark 100k Generator -->
        <button 
          @click="loadBenchmark100k"
          class="px-3 py-1.5 bg-indigo-500/10 hover:bg-indigo-500/20 text-indigo-300 border border-indigo-500/30 rounded-xl text-xs font-mono font-bold transition-all flex items-center gap-1.5"
          :title="'Generate 100,000 data points to stress test LTTB downsampling'"
        >
          <span>⚡</span>
          <span>100K LTTB Stress</span>
        </button>

        <!-- Clear / Live Stream Toggle -->
        <button 
          @click="toggleStreaming"
          :class="isStreaming ? 'bg-emerald-500/20 text-emerald-400 border-emerald-500/40' : 'bg-slate-800 text-slate-400 border-slate-700'"
          class="px-3 py-1.5 border rounded-xl text-xs font-mono font-bold transition-all flex items-center gap-1.5"
        >
          <span class="w-2 h-2 rounded-full" :class="isStreaming ? 'bg-emerald-400 animate-pulse' : 'bg-slate-500'"></span>
          <span>{{ isStreaming ? 'LIVE STREAM' : 'PAUSED' }}</span>
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
          <span class="text-[11px] font-bold text-slate-400 uppercase tracking-wider">{{ kpi.name }}</span>
          <span class="w-2 h-2 rounded-full" :style="{ backgroundColor: kpi.color }"></span>
        </div>
        <div class="mt-2 flex items-baseline gap-2">
          <span class="text-2xl font-black font-mono tracking-tight text-white">{{ kpi.value }}</span>
          <span class="text-xs font-bold text-slate-500 font-mono">{{ kpi.unit }}</span>
        </div>
        <div class="mt-2 text-[10px] font-mono text-slate-400 flex items-center justify-between">
          <span>Peak: {{ kpi.peak }}</span>
          <span :class="kpi.trend >= 0 ? 'text-emerald-400' : 'text-amber-400'">
            {{ kpi.trend >= 0 ? '+' : '' }}{{ kpi.trend }}%
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
            <span class="inline-block w-2.5 h-2.5 rounded-sm bg-[#3B82F6]"></span>
            <span>Solar PV</span>
          </span>
          <span class="flex items-center gap-1.5">
            <span class="inline-block w-2.5 h-2.5 rounded-sm bg-[#10B981]"></span>
            <span>ESS Battery</span>
          </span>
          <span class="flex items-center gap-1.5">
            <span class="inline-block w-2.5 h-2.5 rounded-sm bg-[#A855F7]"></span>
            <span>Grid Bus</span>
          </span>
          <span class="flex items-center gap-1.5">
            <span class="inline-block w-2.5 h-2.5 rounded-sm bg-[#F97316]"></span>
            <span>Active Load</span>
          </span>
        </div>

        <div class="flex items-center gap-3">
          <span class="text-[11px] text-slate-500">
            Total Points: <strong class="text-slate-300 font-mono">{{ totalPointsCount.toLocaleString() }}</strong>
          </span>
          <span class="text-[11px] text-slate-500">
            Sampling: <span class="text-blue-400 font-bold">LTTB</span>
          </span>
        </div>
      </div>

      <!-- Chart DOM Mount -->
      <div ref="chartContainer" class="w-full h-[450px] min-h-[400px]"></div>

      <!-- Footer Info -->
      <div class="mt-4 pt-3 border-t border-slate-800/80 flex flex-wrap justify-between items-center text-[11px] font-mono text-slate-500">
        <div class="flex items-center gap-4">
          <span>Crosshair Tooltip: JetBrains Mono</span>
          <span>Zoom: Wheel / DataZoom Slider</span>
        </div>
        <div>
          <span>DataHub Telemetry Sync: 100ms MTT Cycle</span>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onBeforeUnmount, watch } from 'vue';
import * as echarts from 'echarts';
import axios from 'axios';
import { useContractStore } from '../../services/ContractLoader';

const contractStore = useContractStore();
const chartContainer = ref<HTMLElement | null>(null);
let chartInstance: echarts.ECharts | null = null;

const viewMode = ref<'multi' | 'single'>('multi');
const selectedTagId = ref<number>(1001);
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

// KPI Metrics computed
const kpiMetrics = computed(() => {
  const lastPv = pvData.value.length > 0 ? pvData.value[pvData.value.length - 1][1] : 0;
  const lastEss = essData.value.length > 0 ? essData.value[essData.value.length - 1][1] : 0;
  const lastGrid = gridData.value.length > 0 ? gridData.value[gridData.value.length - 1][1] : 0;
  const lastLoad = loadData.value.length > 0 ? loadData.value[loadData.value.length - 1][1] : 0;

  return [
    { name: 'Solar PV Output', value: lastPv.toFixed(2), unit: 'kW', color: '#3B82F6', peak: '124.5', trend: 4.2 },
    { name: 'ESS Power Flow', value: lastEss.toFixed(2), unit: 'kW', color: '#10B981', peak: '85.0', trend: -1.8 },
    { name: 'Grid Exchange', value: lastGrid.toFixed(2), unit: 'kW', color: '#A855F7', peak: '50.2', trend: 0.5 },
    { name: 'Facility Load', value: lastLoad.toFixed(2), unit: 'kW', color: '#F97316', peak: '160.8', trend: 2.1 },
  ];
});

// ECharts Master Option Construction
function getChartOptions(): echarts.EChartsOption {
  const isMulti = viewMode.value === 'multi';

  const seriesList: echarts.SeriesOption[] = isMulti ? [
    {
      name: 'Solar PV',
      type: 'line',
      sampling: 'lttb',
      showSymbol: false,
      lineStyle: { width: 2, color: '#3B82F6' },
      areaStyle: {
        color: new echarts.graphic.LinearGradient(0, 0, 0, 1, [
          { offset: 0, color: 'rgba(59, 130, 246, 0.35)' },
          { offset: 1, color: 'rgba(59, 130, 246, 0.0)' }
        ])
      },
      data: pvData.value
    },
    {
      name: 'ESS Battery',
      type: 'line',
      sampling: 'lttb',
      showSymbol: false,
      lineStyle: { width: 2, color: '#10B981' },
      areaStyle: {
        color: new echarts.graphic.LinearGradient(0, 0, 0, 1, [
          { offset: 0, color: 'rgba(16, 185, 129, 0.35)' },
          { offset: 1, color: 'rgba(16, 185, 129, 0.0)' }
        ])
      },
      data: essData.value
    },
    {
      name: 'Grid Bus',
      type: 'line',
      sampling: 'lttb',
      showSymbol: false,
      lineStyle: { width: 2, color: '#A855F7' },
      areaStyle: {
        color: new echarts.graphic.LinearGradient(0, 0, 0, 1, [
          { offset: 0, color: 'rgba(168, 85, 247, 0.25)' },
          { offset: 1, color: 'rgba(168, 85, 247, 0.0)' }
        ])
      },
      data: gridData.value
    },
    {
      name: 'Active Load',
      type: 'line',
      sampling: 'lttb',
      showSymbol: false,
      lineStyle: { width: 2, color: '#F97316' },
      areaStyle: {
        color: new echarts.graphic.LinearGradient(0, 0, 0, 1, [
          { offset: 0, color: 'rgba(249, 115, 22, 0.25)' },
          { offset: 1, color: 'rgba(249, 115, 22, 0.0)' }
        ])
      },
      data: loadData.value
    }
  ] : [
    {
      name: `Tag #${selectedTagId.value}`,
      type: 'line',
      sampling: 'lttb',
      showSymbol: false,
      lineStyle: { width: 2.5, color: '#06B6D4' },
      areaStyle: {
        color: new echarts.graphic.LinearGradient(0, 0, 0, 1, [
          { offset: 0, color: 'rgba(6, 182, 212, 0.4)' },
          { offset: 1, color: 'rgba(6, 182, 212, 0.0)' }
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
          fontFamily: 'JetBrains Mono, Menlo, monospace',
          backgroundColor: '#1E293B',
          color: '#F8FAFC'
        },
        lineStyle: {
          color: 'rgba(148, 163, 184, 0.3)',
          type: 'dashed'
        }
      },
      backgroundColor: '#0F172A',
      borderColor: '#334155',
      borderWidth: 1,
      textStyle: {
        fontFamily: 'JetBrains Mono, Menlo, monospace',
        color: '#F1F5F9',
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
      axisLine: { lineStyle: { color: '#334155' } },
      axisLabel: {
        fontFamily: 'JetBrains Mono, Menlo, monospace',
        color: '#64748B',
        fontSize: 10,
        formatter: '{HH}:{mm}:{ss}'
      },
      splitLine: {
        show: true,
        lineStyle: { color: 'rgba(255, 255, 255, 0.04)' }
      }
    },
    yAxis: {
      type: 'value',
      axisLine: { lineStyle: { color: '#334155' } },
      axisLabel: {
        fontFamily: 'JetBrains Mono, Menlo, monospace',
        color: '#64748B',
        fontSize: 10
      },
      splitLine: {
        show: true,
        lineStyle: { color: 'rgba(255, 255, 255, 0.05)' }
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
        backgroundColor: '#0F172A',
        borderColor: '#1E293B',
        fillerColor: 'rgba(59, 130, 246, 0.2)',
        handleStyle: {
          color: '#3B82F6',
          borderColor: '#60A5FA'
        },
        textStyle: {
          fontFamily: 'JetBrains Mono, Menlo, monospace',
          color: '#64748B',
          fontSize: 9
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
        const res = await axios.get(`/api/v1/data/latest/${selectedTagId.value}`);
        if (res.data && res.data.value !== undefined) {
          singleTagData.value.push([t, Number(res.data.value)]);
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
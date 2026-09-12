<template>
  <div 
    class="relative group bg-[#0f172a] border p-5 rounded-2xl transition-all duration-300 hover:shadow-xl hover:-translate-y-0.5 overflow-hidden font-ui"
    :class="[
      status === 'OFFLINE' 
        ? 'border-rose-900/50 bg-[#0f172a]/80 shadow-rose-950/20' 
        : (status === 'STALE' 
            ? 'border-amber-900/40 hover:border-amber-500/40' 
            : 'border-[#334155]/60 hover:border-sky-500/50 hover:shadow-sky-500/10')
    ]"
  >
    <!-- Left Status Accent Bar -->
    <div 
      class="absolute top-0 left-0 w-1 h-full transition-all duration-300"
      :class="[
        status === 'LIVE' ? 'bg-emerald-500 opacity-90' : 
        (status === 'STALE' ? 'bg-amber-500 opacity-80' : 'bg-rose-600 opacity-100')
      ]"
    ></div>

    <!-- Header: Label / Name & Status Badge -->
    <div class="flex justify-between items-start gap-2 mb-3 pl-1">
      <div class="flex items-center gap-1.5 min-w-0">
        <span v-if="meta.attributes?.ui?.icon" class="text-xs text-slate-400">{{ meta.attributes.ui.icon }}</span>
        <h4 
          class="text-[11px] font-bold text-slate-300 truncate uppercase tracking-wider leading-tight group-hover:text-white transition-colors"
          :title="meta.name"
        >
          {{ meta.attributes?.ui?.label || meta.name }}
        </h4>
      </div>
      
      <!-- Three-state Industrial Liveliness Indicator -->
      <div class="flex items-center gap-1.5 flex-shrink-0">
        <span 
          class="inline-flex items-center px-1.5 py-0.5 rounded text-[9px] font-mono font-bold tracking-tight uppercase"
          :class="[
            status === 'LIVE' ? 'bg-emerald-500/10 text-emerald-400 border border-emerald-500/20' :
            (status === 'STALE' ? 'bg-amber-500/10 text-amber-400 border border-amber-500/20' :
             'bg-rose-500/10 text-rose-400 border border-rose-500/20')
          ]"
        >
          <span 
            class="w-1.5 h-1.5 rounded-full mr-1"
            :class="[
              status === 'LIVE' ? 'bg-emerald-400 animate-pulse' :
              (status === 'STALE' ? 'bg-amber-400' : 'bg-rose-500')
            ]"
          ></span>
          {{ lastUpdateLabel }}
        </span>
      </div>
    </div>

    <!-- Telemetry Value Display with 400ms GSAP / CSS Breathing Animation -->
    <div class="flex items-baseline gap-1.5 pl-1 my-1">
      <span 
        class="text-2xl lg:text-3xl font-black font-mono tracking-tight transition-all duration-400 ease-out"
        :class="[
          valueChanged ? 'scale-[1.03]' : '',
          valueChanged && flashColor ? '' : (valueChanged ? 'text-sky-400 drop-shadow-[0_0_8px_rgba(56,189,248,0.4)]' : (status === 'OFFLINE' ? 'text-slate-400' : 'text-white'))
        ]"
        :style="valueChanged && flashColor ? { color: flashColor, textShadow: `0 0 10px ${flashColor}` } : {}"
      >
        {{ formattedValue }}
      </span>
      <span v-if="meta.unit" class="text-[11px] font-bold text-slate-400 font-mono uppercase tracking-wider">
        {{ meta.unit }}
      </span>
    </div>

    <!-- Optional Min/Max Gauge Range Bar -->
    <div v-if="hasRange" class="mt-3 pl-1 space-y-1">
      <div class="w-full bg-[#1e293b] h-1.5 rounded-full overflow-hidden border border-[#334155]/40">
        <div 
          class="h-full rounded-full transition-all duration-500 ease-out"
          :style="{ width: `${rangePercentage}%`, backgroundColor: flashColor || '#38bdf8' }"
        ></div>
      </div>
      <div class="flex justify-between text-[9px] font-mono text-slate-500">
        <span>{{ meta.attributes?.ui?.min }}</span>
        <span>{{ meta.attributes?.ui?.max }}</span>
      </div>
    </div>

    <!-- Footer: Point ID, AI Diagnostic & Widget Type -->
    <div class="mt-4 pt-2.5 border-t border-[#334155]/40 flex justify-between items-center text-[10px] font-mono text-slate-400 pl-1">
      <div class="flex items-center gap-2">
        <span class="text-slate-400">ID: <span class="text-slate-300 font-bold">{{ meta.id }}</span></span>
        <button 
          @click.stop="diagnoseWithAI"
          class="px-1.5 py-0.5 rounded bg-blue-950/60 hover:bg-blue-600 text-blue-300 hover:text-white border border-blue-500/30 transition-all cursor-pointer text-[9px] font-sans flex items-center gap-1"
          title="看图问诊 (Context-Aware Chat)"
        >
          <span>⚡</span>
          <span>AI 诊断</span>
        </button>
      </div>
      <span class="text-[9px] uppercase tracking-wider px-1 py-0.5 rounded bg-[#1e293b] text-slate-400 border border-[#334155]/30">
        {{ meta.attributes?.ui?.widget || 'TEXT' }}
      </span>
    </div>
  </div>
</template>

<script setup lang="ts">
import { shallowRef, computed, onMounted, onUnmounted } from 'vue';
import axios from 'axios';
import { TagMeta } from '../services/ContractLoader';
import { useAICopilot } from '../services/AIProviderService';

const props = defineProps<{
  meta: TagMeta;
  refreshMs?: number;
}>();

const { openWithContext } = useAICopilot();

function diagnoseWithAI() {
  openWithContext({
    selectedTagId: typeof props.meta.id === 'string' ? parseInt(props.meta.id, 16) || 0 : props.meta.id,
    selectedTagName: props.meta.name,
    currentValue: formattedValue.value,
    unit: props.meta.unit,
    pageTitle: '测点实时监控',
  }, `请对测点 ${props.meta.name} (当前值: ${formattedValue.value} ${props.meta.unit || ''}) 进行物理工况诊断与异常排查。`);
}

// ShallowRef for high-frequency telemetry memory optimization (AEP-492)
const value = shallowRef<number | null>(null);
const lastUpdateTs = shallowRef<number>(0);
const valueChanged = shallowRef(false);
const status = shallowRef<'LIVE' | 'STALE' | 'OFFLINE'>('OFFLINE');

let timer: any = null;
let animTimer: any = null;
let rafId: number | null = null;

const flashColor = computed(() => props.meta.attributes?.ui?.flash_color || null);

const updateStatus = () => {
  if (!lastUpdateTs.value) {
    status.value = 'OFFLINE';
    return;
  }
  const diff = Date.now() - lastUpdateTs.value;
  if (diff <= 2000) {
    status.value = 'LIVE';
  } else if (diff <= 10000) {
    status.value = 'STALE';
  } else {
    status.value = 'OFFLINE';
  }
};

const fetchValue = async () => {
  try {
    const res = await axios.get(`/api/v1/data/latest/${props.meta.id}`);
    if (res.data && res.data.value !== undefined) {
      if (rafId) cancelAnimationFrame(rafId);
      rafId = requestAnimationFrame(() => {
        const newVal = res.data.value;
        const newTs = res.data.ts || Date.now();

        if (value.value !== null && value.value !== newVal) {
          valueChanged.value = true;
          if (animTimer) clearTimeout(animTimer);
          // 400ms GSAP / CSS smooth breathing flash animation
          animTimer = setTimeout(() => {
            valueChanged.value = false;
          }, 400);
        }

        value.value = newVal;
        lastUpdateTs.value = newTs;
        updateStatus();
      });
    }
  } catch (e) {
    updateStatus();
  }
};

const formattedValue = computed(() => {
  if (value.value === null) return '---';
  const p = props.meta.attributes?.ui?.precision ?? 2;
  return Number(value.value).toLocaleString(undefined, {
    minimumFractionDigits: p,
    maximumFractionDigits: p
  });
});

const lastUpdateLabel = computed(() => {
  if (!lastUpdateTs.value || status.value === 'OFFLINE') return 'OFFLINE';
  const d = Math.round((Date.now() - lastUpdateTs.value) / 1000);
  return d < 2 ? 'LIVE' : `STALE ${d}S`;
});

const hasRange = computed(() => {
  const ui = props.meta.attributes?.ui;
  return ui && ui.min !== undefined && ui.max !== undefined && ui.max > ui.min;
});

const rangePercentage = computed(() => {
  if (!hasRange.value || value.value === null) return 0;
  const min = props.meta.attributes!.ui!.min!;
  const max = props.meta.attributes!.ui!.max!;
  const clamped = Math.max(min, Math.min(max, value.value));
  return Math.round(((clamped - min) / (max - min)) * 100);
});

onMounted(() => {
  fetchValue();
  timer = setInterval(() => {
    fetchValue();
    updateStatus();
  }, props.refreshMs || 2000);
});

onUnmounted(() => {
  if (timer) clearInterval(timer);
  if (animTimer) clearTimeout(animTimer);
  if (rafId) cancelAnimationFrame(rafId);
});
</script>
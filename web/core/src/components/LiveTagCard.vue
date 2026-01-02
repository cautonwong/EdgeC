<template>
  <div 
    class="relative group bg-white border border-slate-200 p-6 rounded-[2rem] transition-all duration-300 hover:shadow-2xl hover:shadow-blue-500/10 hover:-translate-y-1 overflow-hidden"
    :class="{ 'opacity-60 bg-slate-50': isStale }"
  >
    <!-- Border Accent -->
    <div class="absolute top-0 left-0 w-1.5 h-full bg-blue-600 transition-opacity duration-300 opacity-0 group-hover:opacity-100" :class="{ '!opacity-100': !isStale }"></div>

    <div class="flex justify-between items-start mb-4">
      <h4 class="text-[10px] font-black text-slate-400 uppercase tracking-[0.15em] leading-tight">{{ meta.name }}</h4>
      <div 
        class="w-1.5 h-1.5 rounded-full shadow-[0_0_8px_currentColor]" 
        :class="isStale ? 'text-slate-300 bg-current' : 'text-emerald-500 bg-current animate-pulse'"
      ></div>
    </div>

    <div class="flex items-baseline gap-1.5">
      <span 
        class="text-3xl font-black text-slate-900 font-mono tracking-tighter transition-all duration-300"
        :class="{ 'text-blue-600 scale-105': valueChanged }"
      >
        {{ formattedValue }}
      </span>
      <span class="text-xs font-bold text-slate-400 uppercase">{{ meta.unit }}</span>
    </div>

    <div class="mt-6 flex justify-between items-center text-[10px] font-bold tracking-widest">
      <span class="text-slate-300 font-mono">ID: {{ meta.id }}</span>
      <span :class="isStale ? 'text-rose-400' : 'text-emerald-500'">
        {{ lastUpdateLabel }}
      </span>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted } from 'vue';
import axios from 'axios';
import { TagMeta } from '../services/ContractLoader';

const props = defineProps<{
  meta: TagMeta;
  refreshMs?: number;
}>();

const value = ref<number | null>(null);
const lastUpdateTs = ref<number>(0);
const valueChanged = ref(false);
let timer: any;

const fetchValue = async () => {
  try {
    const res = await axios.get(`/api/v1/data/latest/${props.meta.id}`);
    if (res.data.value !== undefined) {
      if (value.value !== null && value.value !== res.data.value) {
        valueChanged.value = true;
        setTimeout(() => valueChanged.value = false, 400);
      }
      value.value = res.data.value;
      lastUpdateTs.value = res.data.ts;
    }
  } catch (e) {}
};

const formattedValue = computed(() => {
  if (value.value === null) return '---';
  const p = props.meta.attributes?.ui?.precision ?? 2;
  return value.value.toLocaleString(undefined, { minimumFractionDigits: p, maximumFractionDigits: p });
});

const isStale = computed(() => !lastUpdateTs.value || (Date.now() - lastUpdateTs.value > 10000));

const lastUpdateLabel = computed(() => {
  if (!lastUpdateTs.value) return 'OFFLINE';
  const d = Math.round((Date.now() - lastUpdateTs.value) / 1000);
  return d < 2 ? 'NOW' : `${d}S`;
});

onMounted(() => {
  fetchValue();
  timer = setInterval(fetchValue, props.refreshMs || 2000);
});
onUnmounted(() => timer && clearInterval(timer));
</script>
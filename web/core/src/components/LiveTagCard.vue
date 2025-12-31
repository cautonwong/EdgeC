<template>
  <div class="bg-gray-50 border border-gray-200 p-3 rounded-lg flex flex-col justify-center min-h-[100px]">
    <div class="text-xs text-gray-400 uppercase tracking-wider mb-1">{{ meta.name }}</div>
    <div class="flex items-baseline gap-1">
      <span class="text-2xl font-mono font-bold text-gray-800">
        {{ formattedValue }}
      </span>
      <span class="text-sm text-gray-500">{{ meta.unit }}</span>
    </div>
    <div class="text-[10px] text-gray-400 mt-2 flex justify-between items-center">
      <span>#{{ meta.id }}</span>
      <span :class="statusClass">{{ lastUpdate }}</span>
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
let timer: number;

const fetchValue = async () => {
  try {
    const res = await axios.get(`/api/v1/data/latest/${props.meta.id}`);
    if (res.data.value !== undefined) {
      value.value = res.data.value;
      lastUpdateTs.value = res.data.ts;
    }
  } catch (e) {
    // 影子点查失败不打断界面
  }
};

const formattedValue = computed(() => {
  if (value.value === null) return '--';
  const precision = props.meta.attributes?.ui?.precision ?? 2;
  return value.value.toFixed(precision);
});

const lastUpdate = computed(() => {
  if (!lastUpdateTs.value) return 'OFFLINE';
  const diff = (Date.now() - lastUpdateTs.value) / 1000;
  return diff < 5 ? 'LIVE' : Math.round(diff) + 's ago';
});

const statusClass = computed(() => {
  const diff = (Date.now() - lastUpdateTs.value) / 1000;
  return diff < 5 ? 'text-green-500 font-bold' : 'text-orange-400';
});

onMounted(() => {
  fetchValue();
  timer = setInterval(fetchValue, props.refreshMs || 2000);
});

onUnmounted(() => clearInterval(timer));
</script>

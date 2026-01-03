<template>
  <div class="p-8 lg:p-12 space-y-10 animate-in fade-in duration-500">
    <div class="flex justify-between items-end">
      <div>
        <h1 class="text-3xl font-black text-slate-800 tracking-tight italic">DATA <span class="text-blue-600 not-italic">ANALYSIS</span></h1>
        <p class="text-slate-400 text-sm font-medium mt-1 uppercase tracking-widest">High-Frequency Waveform Stream</p>
      </div>
      <select v-model="selectedTagId" class="bg-white border border-slate-200 px-4 py-2 rounded-xl text-xs font-bold text-slate-600 outline-none focus:ring-2 focus:ring-blue-500/20">
        <option v-for="tag in contractStore.active_tags" :key="tag.id" :value="tag.id">
          {{ tag.name }} (#{{ tag.id }})
        </option>
      </select>
    </div>

    <!-- Real-time Chart Area -->
    <div class="bg-slate-900 rounded-[3rem] p-10 shadow-2xl relative overflow-hidden border border-white/5">
      <div class="absolute inset-0 bg-gradient-to-br from-blue-500/5 to-transparent pointer-events-none"></div>
      
      <div class="relative z-10 space-y-8">
        <div class="flex justify-between items-center">
          <div v-if="activeTag" class="space-y-1">
            <h3 class="text-white font-black text-2xl tracking-tighter uppercase">{{ activeTag.name }}</h3>
            <p class="text-slate-500 text-[10px] font-bold tracking-[0.2em] uppercase">{{ activeTag.desc }}</p>
          </div>
          <div class="text-right">
            <div class="text-blue-400 text-4xl font-black font-mono tracking-tighter">{{ currentVal }}<span class="text-sm ml-2 opacity-50">{{ activeTag?.unit }}</span></div>
          </div>
        </div>

        <!-- Mock Chart Visualization -->
        <div class="h-64 w-full flex items-end gap-1 px-2 border-b border-white/10 pb-4">
          <div 
            v-for="(point, idx) in history" 
            :key="idx"
            class="flex-1 bg-gradient-to-t from-blue-600 to-indigo-400 rounded-t-sm transition-all duration-500"
            :style="{ height: `${point.percent}%` }"
          ></div>
        </div>
        
        <div class="flex justify-between text-[10px] font-black text-slate-600 uppercase tracking-widest">
          <span>T-60s</span>
          <span>Buffer: Syncing</span>
          <span>Real-time</span>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted, watch } from 'vue';
import axios from 'axios';
import { useContractStore } from '../../services/ContractLoader';

const contractStore = useContractStore();
const selectedTagId = ref<number>(1001);
const currentVal = ref('0.00');
const history = ref<any[]>([]);
let timer: any;

const activeTag = computed(() => {
  return Array.from(contractStore.tagMap.values()).find(t => t.id === selectedTagId.value);
});

const fetchData = async () => {
  try {
    const res = await axios.get(`/api/v1/data/latest/${selectedTagId.value}`);
    if (res.data.value !== undefined) {
      currentVal.value = res.data.value.toFixed(2);
      // 模拟曲线数据
      const percent = Math.min(100, Math.max(10, (res.data.value % 100)));
      history.value.push({ percent });
      if (history.value.length > 60) history.value.shift();
    }
  } catch (e) {}
};

watch(selectedTagId, () => {
  history.value = [];
  fetchData();
});

onMounted(() => {
  timer = setInterval(fetchData, 1000);
});
onUnmounted(() => clearInterval(timer));
</script>
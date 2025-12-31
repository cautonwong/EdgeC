<template>
  <div class="p-6 max-w-7xl mx-auto">
    <div class="flex flex-col md:flex-row justify-between items-start md:items-center gap-4 mb-8">
      <h2 class="text-2xl font-bold text-gray-800">Historical Curves</h2>
      
      <div class="flex flex-wrap gap-3">
        <!-- Tag Selector (Contract Driven) -->
        <select v-model="selectedTagId" class="px-3 py-2 bg-white border border-gray-300 rounded-md shadow-sm focus:ring-indigo-500 focus:border-indigo-500 text-sm">
          <option :value="0">Select Telemetry Tag...</option>
          <option v-for="tag in trendTags" :key="tag.id" :value="tag.id">
            {{ tag.name }} ({{ tag.unit }})
          </option>
        </select>

        <select v-model="timeRange" class="px-3 py-2 bg-white border border-gray-300 rounded-md shadow-sm text-sm">
          <option value="1h">Last 1 Hour</option>
          <option value="6h">Last 6 Hours</option>
          <option value="24h">Last 24 Hours</option>
        </select>

        <button @click="fetchData" :disabled="!selectedTagId" 
                class="px-4 py-2 bg-indigo-600 text-white rounded-md shadow hover:bg-indigo-700 disabled:bg-gray-300 text-sm font-medium transition-colors">
          Query Data
        </button>
      </div>
    </div>

    <!-- SVG Chart Implementation -->
    <div class="bg-white p-6 rounded-xl shadow-sm border border-gray-100 min-h-[400px] flex flex-col">
      <div v-if="loading" class="flex-1 flex items-center justify-center text-gray-400 animate-pulse">
        Fetching history from SQLite...
      </div>
      <div v-else-if="points.length === 0" class="flex-1 flex items-center justify-center text-gray-400 italic">
        No data found for the selected period.
      </div>
      <div v-else class="flex-1 relative">
        <!-- Simple SVG Line Chart -->
        <svg viewBox="0 0 1000 300" class="w-full h-full preserve-3d" preserveAspectRatio="none">
          <polyline
            fill="none"
            stroke="#6366f1"
            stroke-width="2"
            :points="svgPoints"
          />
        </svg>
        <!-- Time Axis Labels (Simplified) -->
        <div class="flex justify-between mt-4 text-[10px] text-gray-400 font-mono">
          <span>{{ formatTs(points[0].ts) }}</span>
          <span>{{ formatTs(points[points.length-1].ts) }}</span>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue';
import axios from 'axios';
import { useContractStore } from '../../services/ContractLoader';

const contractStore = useContractStore();
const selectedTagId = ref(0);
const timeRange = ref('1h');
const points = ref<{ts: number, val: number}[]>([]);
const loading = ref(false);

const trendTags = computed(() => {
  return contractStore.contract?.tags.filter(t => t.attributes?.northbound?.storage === 'trend') || [];
});

const fetchData = async () => {
  if (!selectedTagId.value) return;
  loading.value = true;
  try {
    const end = Date.now();
    const duration = timeRange.value === '1h' ? 3600000 : (timeRange.value === '6h' ? 21600000 : 86400000);
    const start = end - duration;
    
    const res = await axios.get(`/api/v1/data/query?tag=${selectedTagId.value}&start=${start}&end=${end}`);
    points.value = res.data.data || [];
  } catch (e) {
    console.error('Query failed:', e);
  } finally {
    loading.value = false;
  }
};

const svgPoints = computed(() => {
  if (points.value.length < 2) return "";
  const minV = Math.min(...points.value.map(p => p.val));
  const maxV = Math.max(...points.value.map(p => p.val));
  const range = (maxV - minV) || 1;
  const startTime = points.value[0].ts;
  const timeRange = points.value[points.value.length - 1].ts - startTime || 1;

  return points.value.map(p => {
    const x = ((p.ts - startTime) / timeRange) * 1000;
    const y = 300 - (((p.val - minV) / range) * 250 + 25); // Padding 25px
    return `${x},${y}`;
  }).join(' ');
});

function formatTs(ms: number) {
  return new Date(ms).toLocaleTimeString();
}

onMounted(() => {
  if (contractStore.groups.size === 0) {
    contractStore.loadContract();
  }
});
</script>

<style scoped>
svg { overflow: visible; }
</style>

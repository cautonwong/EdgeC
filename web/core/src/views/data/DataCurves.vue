<template>
  <div class="p-4">
    <div class="flex justify-between items-center mb-4">
      <h2 class="text-xl font-bold">Data Analysis</h2>
      <div class="space-x-2">
        <select v-model="selectedTag" class="border p-1 rounded">
          <option v-for="tag in trendTags" :key="tag.id" :value="tag.id">
            {{ tag.name }} ({{ tag.unit }})
          </option>
        </select>
        <button @click="fetchData" class="bg-blue-500 text-white px-3 py-1 rounded">Query</button>
      </div>
    </div>

    <div class="bg-white p-4 rounded shadow h-64 flex items-center justify-center border-2 border-dashed border-gray-200">
      <div v-if="loading">Loading...</div>
      <div v-else-if="chartData.length === 0" class="text-gray-400">No Data Available</div>
      <div v-else class="w-full h-full relative">
        <!-- 简易 SVG 折线图 -->
        <svg viewBox="0 0 100 100" preserveAspectRatio="none" class="w-full h-full">
          <polyline
            fill="none"
            stroke="#3b82f6"
            stroke-width="2"
            :points="svgPoints"
          />
        </svg>
      </div>
    </div>
    
    <div class="mt-4">
      <h3 class="font-bold">Raw Data</h3>
      <table class="w-full mt-2 text-sm">
        <thead><tr class="text-left"><th>Time</th><th>Value</th></tr></thead>
        <tbody>
          <tr v-for="(row, i) in chartData.slice(0, 10)" :key="i" class="border-b">
            <td>{{ new Date(row.ts).toLocaleString() }}</td>
            <td>{{ row.val }}</td>
          </tr>
        </tbody>
      </table>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue';
import { useContractStore } from '../../services/ContractLoader';
import axios from 'axios';

const store = useContractStore();
const selectedTag = ref<number | null>(null);
const chartData = ref<{ts: number, val: number}[]>([]);
const loading = ref(false);

// 筛选出支持存储的 Tag
const trendTags = computed(() => {
  const list = [];
  if (!store.tagMap) return [];
  for (const tag of store.tagMap.values()) {
    if (tag.attributes?.northbound?.storage === 'trend') {
      list.push(tag);
    }
  }
  return list;
});

const svgPoints = computed(() => {
  if (chartData.value.length < 2) return '';
  const maxVal = Math.max(...chartData.value.map(d => d.val));
  const minVal = Math.min(...chartData.value.map(d => d.val));
  const range = maxVal - minVal || 1;
  
  return chartData.value.map((d, i) => {
    const x = (i / (chartData.value.length - 1)) * 100;
    const y = 100 - ((d.val - minVal) / range) * 100;
    return `${x},${y}`;
  }).join(' ');
});

const fetchData = async () => {
  if (!selectedTag.value) return;
  loading.value = true;
  try {
    // 模拟时间范围：过去 1 小时
    const end = Date.now();
    const start = end - 3600 * 1000;
    const res = await axios.get(`/api/v1/data/query`, {
      params: { tag_id: selectedTag.value, start, end }
    });
    chartData.value = res.data.data || [];
  } finally {
    loading.value = false;
  }
};

onMounted(async () => {
  if (!store.contract) await store.loadContract();
  if (trendTags.value.length > 0) {
    selectedTag.value = trendTags.value[0].id;
  }
});
</script>

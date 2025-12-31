<template>
  <div class="p-6 max-w-7xl mx-auto">
    <div class="flex justify-between items-center mb-6">
      <h2 class="text-2xl font-bold text-gray-800">System Dashboard</h2>
      <div v-if="contractStore.loading" class="text-sm text-blue-500 animate-pulse">Loading Contract...</div>
    </div>
    
    <!-- 1. System High-Level Stats (Summary) -->
    <div class="grid grid-cols-1 md:grid-cols-3 gap-6 mb-8">
      <div class="bg-white p-5 rounded-xl shadow-sm border border-gray-100">
        <div class="text-sm text-gray-400 font-medium uppercase mb-1">Controller Status</div>
        <div class="text-3xl font-bold" :class="health.status === 'ok' ? 'text-emerald-600' : 'text-rose-600'">
          {{ health.status?.toUpperCase() || 'OFFLINE' }}
        </div>
        <div class="text-xs text-gray-400 mt-2 italic">Uptime: {{ formatUptime(health.uptime) }}</div>
      </div>

      <div class="bg-white p-5 rounded-xl shadow-sm border border-gray-100">
        <div class="text-sm text-gray-400 font-medium uppercase mb-1">Persistence Health</div>
        <div class="text-3xl font-bold text-blue-600">
          {{ health.storage?.queue_depth || 0 }}
        </div>
        <div class="text-xs text-gray-400 mt-2">Active SQLite WAL Queue</div>
      </div>

      <div class="bg-white p-5 rounded-xl shadow-sm border border-gray-100">
        <div class="text-sm text-gray-400 font-medium uppercase mb-1">Engine Pipelines</div>
        <div class="text-3xl font-bold text-indigo-600">
          {{ health.pipelines?.running || 0 }}
        </div>
        <div class="text-xs text-gray-400 mt-2 text-rose-400" v-if="health.pipelines?.error">
          Alert: {{ health.pipelines.error }} failed!
        </div>
        <div class="text-xs text-gray-400 mt-2" v-else>All logic chains healthy</div>
      </div>
    </div>

    <!-- 2. Dynamic Telemetry Groups (Driven by Contract YAML) -->
    <div v-for="[groupName, tags] in contractStore.groups" :key="groupName" class="mb-8">
      <h3 class="text-lg font-semibold text-gray-700 mb-4 border-l-4 border-indigo-500 pl-3">
        {{ formatGroupName(groupName) }}
      </h3>
      <div class="grid grid-cols-2 sm:grid-cols-3 lg:grid-cols-4 xl:grid-cols-5 gap-4">
        <LiveTagCard 
          v-for="tag in tags" 
          :key="tag.id" 
          :meta="tag" 
          :refreshMs="1000"
        />
      </div>
    </div>

    <div v-if="contractStore.groups.size === 0 && !contractStore.loading" class="text-center py-20 bg-gray-50 rounded-xl border-2 border-dashed border-gray-200">
      <div class="text-gray-400">No active telemetry groups found in contract.</div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted } from 'vue';
import axios from 'axios';
import { useContractStore } from '../services/ContractLoader';
import LiveTagCard from '../components/LiveTagCard.vue';

const contractStore = useContractStore();
const health = ref<any>({});
let timer: number;

const fetchHealth = async () => {
  try {
    const res = await axios.get('/api/v1/system/health');
    health.value = res.data;
  } catch (e) {
    health.value = { status: 'error' };
  }
};

function formatUptime(sec: number) {
  if (!sec) return '-';
  const h = Math.floor(sec / 3600);
  const m = Math.floor((sec % 3600) / 60);
  return `${h}h ${m}m`;
}

function formatGroupName(name: string) {
  return name.replace(/_/g, ' ').toUpperCase();
}

onMounted(async () => {
  // 启动状态轮询
  fetchHealth();
  timer = setInterval(fetchHealth, 3000);
});

onUnmounted(() => clearInterval(timer));
</script>

<style scoped>
/* 可以在此添加针对 Dashboard 的特定样式 */
</style>
<template>
  <div class="p-10 space-y-12 animate-in fade-in slide-in-from-bottom-4 duration-700">
    <!-- Hero Visualization Area -->
    <section class="relative bg-slate-900 rounded-[2.5rem] overflow-hidden shadow-2xl group">
      <!-- Ambient Background -->
      <div class="absolute inset-0 bg-gradient-to-br from-blue-600/20 to-transparent pointer-events-none"></div>
      <div class="absolute -top-24 -right-24 w-96 h-96 bg-blue-500/10 rounded-full blur-3xl group-hover:bg-blue-500/20 transition-all duration-1000"></div>
      
      <div class="relative p-12 md:p-16 flex flex-col md:flex-row items-end justify-between gap-8">
        <div class="space-y-4">
          <h1 class="text-4xl md:text-5xl font-black text-white tracking-tight italic">
            EDGE<span class="text-blue-500 not-italic">OS</span>
          </h1>
          <p class="text-slate-400 text-lg font-medium max-w-md leading-relaxed">
            Industrial intelligence platform for <span class="text-slate-200">{{ contractStore.contract?.product }}</span> real-time management.
          </p>
        </div>

        <!-- Quick Metrics Grid -->
        <div class="grid grid-cols-2 gap-4 w-full md:w-auto">
          <div class="bg-white/5 backdrop-blur-xl border border-white/10 p-6 rounded-3xl min-w-[160px]">
            <div class="text-[10px] font-black text-blue-400 uppercase tracking-widest mb-2">Storage WAL</div>
            <div class="text-2xl font-black text-white font-mono leading-none">{{ health.storage?.queue_depth || 0 }}</div>
          </div>
          <div class="bg-white/5 backdrop-blur-xl border border-white/10 p-6 rounded-3xl min-w-[160px]">
            <div class="text-[10px] font-black text-emerald-400 uppercase tracking-widest mb-2">IO Pipelines</div>
            <div class="text-2xl font-black text-white font-mono leading-none">{{ health.pipelines?.running || 0 }}</div>
          </div>
        </div>
      </div>
    </section>

    <!-- Content Groups -->
    <div v-for="[groupName, tags] in contractStore.groups" :key="groupName" class="space-y-8">
      <div class="flex items-center gap-6">
        <h2 class="text-2xl font-black text-slate-800 tracking-tight uppercase italic">{{ groupName.replace(/_/g, ' ') }}</h2>
        <div class="h-px flex-1 bg-slate-200"></div>
        <div class="flex items-center gap-2 bg-white px-4 py-1.5 rounded-full shadow-sm border border-slate-100">
          <span class="w-1.5 h-1.5 rounded-full bg-blue-500 animate-pulse"></span>
          <span class="text-[10px] font-black text-slate-500 uppercase tracking-widest">{{ tags.length }} Nodes</span>
        </div>
      </div>

      <div class="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4 gap-8">
        <LiveTagCard 
          v-for="tag in tags" 
          :key="tag.id" 
          :meta="tag" 
          :refreshMs="1000"
        />
      </div>
    </div>

    <!-- Empty State -->
    <div v-if="contractStore.groups.size === 0 && !contractStore.loading" class="flex flex-col items-center justify-center py-32 bg-white rounded-[3rem] border-2 border-dashed border-slate-200">
      <div class="text-6xl mb-6 grayscale opacity-20">📡</div>
      <h3 class="text-xl font-bold text-slate-800">Telemetry Constitution Missing</h3>
      <p class="text-slate-400 mt-2">No active tags with UI attributes were found in the current contract.</p>
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
let timer: any;

const fetchHealth = async () => {
  try {
    const res = await axios.get('/api/v1/system/health');
    health.value = res.data;
  } catch (e) {
    health.value = { status: 'error' };
  }
};

onMounted(() => {
  fetchHealth();
  timer = setInterval(fetchHealth, 3000);
});
onUnmounted(() => timer && clearInterval(timer));
</script>

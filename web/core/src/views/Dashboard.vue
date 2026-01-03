<template>
  <div class="min-h-full bg-slate-50/50 p-8 lg:p-12 space-y-16 animate-in fade-in duration-700">
    <!-- Hero Header: The Command Center Look -->
    <section class="relative bg-slate-900 rounded-[3rem] overflow-hidden shadow-2xl border border-white/5">
      <!-- Ambient light effect -->
      <div class="absolute top-0 right-0 w-[500px] h-[500px] bg-blue-500/10 rounded-full blur-[120px] -mr-40 -mt-40"></div>
      <div class="absolute bottom-0 left-0 w-[300px] h-[300px] bg-indigo-500/10 rounded-full blur-[80px] -ml-20 -mb-20"></div>
      
      <div class="relative z-10 p-12 lg:p-16 flex flex-col md:flex-row items-center justify-between gap-12">
        <div class="space-y-6 text-center md:text-left">
          <div class="inline-flex items-center px-4 py-1.5 rounded-full bg-blue-500/10 border border-blue-500/20 text-blue-400 text-[10px] font-black uppercase tracking-[0.2em]">
            System Operational
          </div>
          <h1 class="text-4xl lg:text-6xl font-black text-white tracking-tighter leading-tight">
            Operational<br/><span class="text-blue-500">Intelligence</span>
          </h1>
          <p class="text-slate-400 text-base lg:text-lg font-medium max-w-md">
            Advanced management interface for <span class="text-slate-200 underline decoration-blue-500/50 underline-offset-4">{{ contractStore.contract?.product_name }}</span>.
          </p>
        </div>

        <!-- System-wide Stats Wall -->
        <div class="grid grid-cols-2 gap-6 w-full md:w-auto">
          <div class="bg-white/5 backdrop-blur-md border border-white/10 p-8 rounded-[2rem] hover:bg-white/10 transition-colors">
            <div class="text-[10px] font-black text-blue-400 uppercase tracking-widest mb-3">Health Index</div>
            <div class="text-3xl font-black text-white font-mono uppercase tracking-tighter">99.2<span class="text-xs ml-1 opacity-40">%</span></div>
          </div>
          <div class="bg-white/5 backdrop-blur-md border border-white/10 p-8 rounded-[2rem] hover:bg-white/10 transition-colors">
            <div class="text-[10px] font-black text-indigo-400 uppercase tracking-widest mb-3">Sync Latency</div>
            <div class="text-3xl font-black text-white font-mono uppercase tracking-tighter">14<span class="text-xs ml-1 opacity-40">ms</span></div>
          </div>
        </div>
      </div>
    </section>

    <!-- Content: Grouped Telemetry Cards -->
    <div v-for="[groupName, tags] in contractStore.groups" :key="groupName" class="space-y-10">
      <div class="flex items-center gap-8">
        <h2 class="text-2xl font-black text-slate-800 tracking-tight uppercase italic">{{ groupName }}</h2>
        <div class="h-px flex-1 bg-gradient-to-r from-slate-200 to-transparent"></div>
        <div class="text-[10px] font-black text-slate-400 uppercase tracking-widest">{{ tags.length }} metrics</div>
      </div>

      <div class="grid grid-cols-1 sm:grid-cols-2 xl:grid-cols-4 gap-8">
        <LiveTagCard 
          v-for="tag in tags" 
          :key="tag.id" 
          :meta="tag" 
          :refreshMs="1000"
        />
      </div>
    </div>

    <!-- Loading & Empty States -->
    <div v-if="contractStore.loading" class="flex flex-col items-center justify-center py-32">
      <div class="w-16 h-16 border-4 border-slate-200 border-t-blue-600 rounded-full animate-spin"></div>
      <p class="mt-6 text-slate-400 font-bold uppercase tracking-widest text-xs">Accessing Data Constitution...</p>
    </div>

    <div v-if="contractStore.groups.size === 0 && !contractStore.loading" class="text-center py-40 bg-white rounded-[4rem] shadow-sm border border-slate-100">
      <div class="text-7xl mb-8 opacity-20">📡</div>
      <h3 class="text-2xl font-black text-slate-800 tracking-tight">No Active Streams Found</h3>
      <p class="text-slate-400 mt-2 max-w-sm mx-auto">Verify your <b>enabled_tags</b> in product configuration.</p>
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
  } catch (e) { health.value = { status: 'error' }; }
};

onMounted(() => {
  fetchHealth();
  timer = setInterval(fetchHealth, 3000);
});
onUnmounted(() => clearInterval(timer));
</script>
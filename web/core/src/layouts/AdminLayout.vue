<template>
  <div class="flex h-screen w-screen overflow-hidden bg-slate-100 font-sans selection:bg-blue-100">
    <!-- Sidebar -->
    <aside class="w-72 bg-slate-950 text-slate-400 flex flex-col shadow-2xl relative z-50">
      <div class="p-8 flex items-center gap-4 bg-slate-900/50 backdrop-blur-md">
        <div class="w-10 h-10 bg-blue-600 rounded-xl flex items-center justify-center shadow-lg shadow-blue-500/20 ring-1 ring-white/20">
          <span class="text-white font-black text-xl italic">E</span>
        </div>
        <div>
          <div class="text-white font-black tracking-tight leading-tight uppercase">
            {{ contractStore.contract?.ui_config?.branding?.title || 'EdgeOS' }}
          </div>
          <div class="text-[10px] font-bold text-slate-500 uppercase tracking-widest mt-0.5">Control Plane</div>
        </div>
      </div>

      <nav class="flex-1 px-4 py-6 space-y-1 overflow-y-auto">
        <router-link 
          v-for="item in visibleMenuItems" 
          :key="item.path" 
          :to="item.path" 
          class="flex items-center gap-3 px-4 py-3 rounded-xl transition-all duration-200 group hover:bg-slate-900 hover:text-white"
          active-class="!bg-blue-600 !text-white shadow-xl shadow-blue-600/20"
        >
          <span class="text-lg opacity-70 group-hover:scale-110 transition-transform">{{ item.meta?.icon }}</span>
          <span class="text-sm font-semibold tracking-wide">{{ item.meta?.title }}</span>
        </router-link>
      </nav>

      <div class="p-6 border-t border-slate-900">
        <div class="p-4 bg-slate-900/50 rounded-2xl border border-slate-800">
          <div class="text-[10px] font-black text-slate-600 uppercase tracking-[0.2em] mb-1">Node Status</div>
          <div class="text-xs text-slate-400 font-mono flex items-center gap-2">
            <span class="w-1.5 h-1.5 rounded-full bg-emerald-500 animate-pulse"></span>
            Operational
          </div>
        </div>
      </div>
    </aside>

    <!-- Main -->
    <div class="flex-1 flex flex-col min-w-0">
      <header class="h-20 bg-white/80 backdrop-blur-md border-b border-slate-200 flex items-center justify-between px-10 sticky top-0 z-40">
        <div class="flex items-center gap-2 text-sm">
          <span class="text-slate-400 font-medium">Nodes</span>
          <span class="text-slate-300">/</span>
          <span class="font-bold text-slate-800 uppercase tracking-tighter">{{ contractStore.contract?.product }}</span>
        </div>

        <div class="flex items-center gap-8">
          <div class="hidden md:flex gap-8 border-r border-slate-100 pr-8">
            <div class="text-right">
              <div class="text-[9px] font-black text-slate-400 uppercase tracking-widest">System Uptime</div>
              <div class="text-sm font-bold text-slate-700 font-mono">{{ formatTime(uptime) }}</div>
            </div>
            <div class="text-right">
              <div class="text-[9px] font-black text-slate-400 uppercase tracking-widest">Real-time Link</div>
              <div class="text-sm font-bold flex items-center gap-1.5 justify-end" :class="isOnline ? 'text-emerald-600' : 'text-rose-500'">
                <span class="w-1 h-1 rounded-full bg-current"></span>
                {{ isOnline ? 'CONNECTED' : 'DISCONNECTED' }}
              </div>
            </div>
          </div>
          <div class="w-10 h-10 rounded-full bg-slate-100 border border-slate-200 flex items-center justify-center text-slate-500 font-bold text-xs cursor-pointer hover:bg-slate-200 transition-colors">
            AD
          </div>
        </div>
      </header>
      
      <main class="flex-1 overflow-y-auto">
        <router-view v-slot="{ Component }">
          <transition name="fade" mode="out-in">
            <component :is="Component" />
          </transition>
        </router-view>
      </main>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted, computed } from 'vue';
import { getVisibleMenuItems } from '../router';
import { useContractStore } from '../services/ContractLoader';
import axios from 'axios';

const contractStore = useContractStore();
const visibleMenuItems = computed(() => getVisibleMenuItems());
const uptime = ref(0);
const isOnline = ref(false);
let timer: any = null;

const fetchStatus = async () => {
  try {
    const res = await axios.get('/api/v1/system/health');
    uptime.value = Math.round(res.data.uptime || 0);
    isOnline.value = true;
  } catch (e) {
    isOnline.value = false;
  }
};

function formatTime(sec: number) {
  const h = Math.floor(sec / 3600);
  const m = Math.floor((sec % 3600) / 60);
  const s = sec % 60;
  return `${h.toString().padStart(2, '0')}:${m.toString().padStart(2, '0')}:${s.toString().padStart(2, '0')}`;
}

onMounted(async () => {
  if (!contractStore.contract) await contractStore.loadContract();
  fetchStatus();
  timer = setInterval(fetchStatus, 5000);
});
onUnmounted(() => timer && clearInterval(timer));
</script>

<template>
  <div class="flex h-screen w-screen bg-slate-100 font-sans selection:bg-blue-100 overflow-hidden">
    <!-- Sidebar Container -->
    <aside 
      class="bg-slate-950 text-slate-400 flex flex-col transition-all duration-300 ease-in-out z-50 overflow-hidden relative"
      :class="[isCollapsed ? 'w-0 shadow-none' : 'w-72 shadow-2xl']"
    >
      <!-- Fixed width inner content -->
      <div class="w-72 h-full flex flex-col transition-opacity duration-200" :class="{ 'opacity-0 pointer-events-none': isCollapsed }">
        <!-- Brand Section -->
        <div class="p-8 flex items-center gap-4 bg-slate-900/50 backdrop-blur-md">
          <div class="w-10 h-10 bg-blue-600 rounded-xl flex items-center justify-center shadow-lg shadow-blue-500/20 ring-1 ring-white/20">
            <span class="text-white font-black text-xl italic">E</span>
          </div>
          <div>
            <div class="text-white font-black tracking-tight leading-tight uppercase truncate max-w-[160px]">
              {{ contractStore.contract?.product_name || 'EdgeOS IEMS' }}
            </div>
            <div class="text-[10px] font-bold text-slate-500 uppercase tracking-widest mt-0.5">Control Plane</div>
          </div>
        </div>

        <!-- Navigation -->
        <nav class="flex-1 px-4 py-6 space-y-1 overflow-y-auto">
          <router-link 
            v-for="item in visibleMenuItems" 
            :key="item.path" 
            :to="item.path" 
            class="flex items-center gap-3 px-4 py-3 rounded-xl transition-all duration-200 group hover:bg-slate-900 hover:text-white"
            active-class="!bg-blue-600 !text-white shadow-xl shadow-blue-600/20"
          >
            <span class="text-lg">{{ item.meta?.icon }}</span>
            <span class="text-sm font-semibold tracking-wide">{{ item.meta?.title }}</span>
          </router-link>
        </nav>

        <!-- Sidebar Footer: Status & Profile -->
        <div class="p-4 border-t border-slate-900 space-y-4">
          <!-- Node Status Card -->
          <div class="p-4 bg-slate-900/50 rounded-2xl border border-slate-800 whitespace-nowrap">
            <div class="text-[10px] font-black text-slate-600 uppercase tracking-[0.2em] mb-1">System Status</div>
            <div class="text-xs text-slate-400 font-mono flex items-center gap-2">
              <span class="w-1.5 h-1.5 rounded-full bg-emerald-500 animate-pulse"></span>
              Operational
            </div>
          </div>

          <!-- User Profile Section (Moved from Header) -->
          <div class="flex items-center gap-3 p-2 rounded-2xl hover:bg-slate-900 transition-colors cursor-pointer group border border-transparent hover:border-slate-800">
            <div class="min-w-[40px] w-10 h-10 bg-indigo-100 text-indigo-600 rounded-xl flex items-center justify-center font-black shadow-lg shadow-indigo-500/10 transition-transform group-hover:scale-105">
              AD
            </div>
            <div class="overflow-hidden">
              <div class="text-sm font-bold text-slate-200 leading-none mb-1">Administrator</div>
              <div class="text-[9px] font-black text-slate-500 uppercase tracking-widest truncate">Root Access</div>
            </div>
          </div>
        </div>
      </div>
    </aside>

    <!-- Main Container -->
    <div class="flex-1 flex flex-col min-w-0 bg-slate-50 overflow-hidden relative">
      <header class="h-20 bg-white/80 backdrop-blur-md border-b border-slate-200 flex items-center justify-between px-6 lg:px-10 sticky top-0 z-40 shadow-sm">
        <div class="flex items-center gap-4">
          <button 
            @click="isCollapsed = !isCollapsed" 
            class="p-2.5 rounded-xl bg-slate-50 hover:bg-slate-100 text-slate-500 transition-all active:scale-95 border border-slate-200 group"
          >
            <svg v-if="isCollapsed" xmlns="http://www.w3.org/2000/svg" class="h-5 w-5 group-hover:text-blue-600" fill="none" viewBox="0 0 24 24" stroke="currentColor">
              <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M4 6h16M4 12h16M4 18h16" />
            </svg>
            <svg v-else xmlns="http://www.w3.org/2000/svg" class="h-5 w-5 group-hover:text-blue-600" fill="none" viewBox="0 0 24 24" stroke="currentColor">
              <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M15 19l-7-7 7-7" />
            </svg>
          </button>

          <div class="flex items-center gap-2 text-sm">
            <span class="text-slate-400 font-medium">Infrastructure</span>
            <span class="text-slate-300">/</span>
            <span class="font-bold text-slate-800 uppercase tracking-tighter truncate">{{ contractStore.contract?.product_name || 'Core System' }}</span>
          </div>
        </div>

        <div class="flex items-center gap-8">
          <!-- Header Right: System Metrics -->
          <div class="flex gap-8">
            <div class="text-right">
              <div class="text-[9px] font-black text-slate-400 uppercase tracking-widest">Uptime</div>
              <div class="text-sm font-bold text-slate-700 font-mono">{{ formatTime(uptime) }}</div>
            </div>
            <div class="text-right pr-2">
              <div class="text-[9px] font-black text-slate-400 uppercase tracking-widest">Link</div>
              <div class="text-sm font-bold flex items-center gap-1.5 justify-end" :class="isOnline ? 'text-emerald-600' : 'text-rose-500'">
                <span class="w-1 h-1 rounded-full bg-current shadow-[0_0_4px_currentColor]"></span>
                {{ isOnline ? 'LIVE' : 'DOWN' }}
              </div>
            </div>
          </div>
        </div>
      </header>
      
      <main class="flex-1 overflow-y-auto overflow-x-hidden p-0 relative">
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
const isCollapsed = ref(false);
let timer: any = null;

const fetchStatus = async () => {
  try {
    const res = await axios.get('/api/v1/system/health');
    uptime.value = Math.round(res.data.uptime || 0);
    isOnline.value = true;
  } catch (e) { isOnline.value = false; }
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
<template>
  <div class="p-8 lg:p-12 space-y-10 animate-in fade-in duration-500">
    <div class="flex justify-between items-end">
      <div>
        <h1 class="text-3xl font-black text-slate-800 tracking-tight italic">PHYSICAL <span class="text-blue-600 not-italic">DEVICES</span></h1>
        <p class="text-slate-400 text-sm font-medium mt-1 uppercase tracking-widest">Execution Plane Inventory</p>
      </div>
      <button @click="fetchDevices" class="px-6 py-2 bg-white border border-slate-200 rounded-xl text-xs font-black text-slate-600 hover:bg-slate-50 transition-all shadow-sm active:scale-95">
        SCAN BUS
      </button>
    </div>

    <!-- Device Grid -->
    <div class="grid grid-cols-1 gap-6">
      <div class="bg-white rounded-[2rem] shadow-xl shadow-slate-200/50 border border-slate-100 overflow-hidden">
        <table class="w-full text-left border-collapse">
          <thead>
            <tr class="bg-slate-50/50 border-b border-slate-100">
              <th class="px-8 py-5 text-[10px] font-black text-slate-400 uppercase tracking-widest">Device Identity</th>
              <th class="px-8 py-5 text-[10px] font-black text-slate-400 uppercase tracking-widest">Protocol Stack</th>
              <th class="px-8 py-5 text-[10px] font-black text-slate-400 uppercase tracking-widest">Comm Status</th>
              <th class="px-8 py-5 text-[10px] font-black text-slate-400 uppercase tracking-widest text-right">Metrics</th>
            </tr>
          </thead>
          <tbody class="divide-y divide-slate-50">
            <tr v-for="dev in devices" :key="dev.id" class="hover:bg-blue-50/30 transition-colors group">
              <td class="px-8 py-6">
                <div class="flex items-center gap-4">
                  <div class="w-10 h-10 bg-slate-100 rounded-xl flex items-center justify-center text-lg shadow-inner">🔌</div>
                  <div>
                    <div class="text-sm font-bold text-slate-700">{{ dev.id }}</div>
                    <div class="text-[10px] text-slate-400 font-mono">ADDR: {{ dev.address || 'Auto' }}</div>
                  </div>
                </div>
              </td>
              <td class="px-8 py-6">
                <span class="px-3 py-1 rounded-lg bg-blue-50 text-blue-600 text-[10px] font-black tracking-tighter border border-blue-100">
                  {{ dev.protocol }}
                </span>
              </td>
              <td class="px-8 py-6">
                <div class="flex items-center gap-2">
                  <span class="w-2 h-2 rounded-full" :class="dev.status === 'online' ? 'bg-emerald-500 shadow-[0_0_8px_#10b981]' : 'bg-rose-500'"></span>
                  <span class="text-xs font-bold uppercase tracking-tight" :class="dev.status === 'online' ? 'text-emerald-600' : 'text-rose-500'">
                    {{ dev.status }}
                  </span>
                </div>
              </td>
              <td class="px-8 py-6 text-right">
                <div class="text-xs font-bold text-slate-600 font-mono">{{ dev.errors }} ERR / {{ dev.success || 0 }} OK</div>
                <div class="text-[9px] text-slate-300 uppercase mt-1">Latency: {{ dev.latency || 12 }}ms</div>
              </td>
            </tr>
          </tbody>
        </table>
        
        <div v-if="devices.length === 0" class="py-20 text-center">
          <div class="text-4xl mb-4 opacity-20">📡</div>
          <p class="text-slate-400 font-medium text-sm italic">Searching for physical nodes on the bus...</p>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue';
import axios from 'axios';

const devices = ref<any[]>([]);

const fetchDevices = async () => {
  try {
    const res = await axios.get('/api/v1/devices');
    devices.value = res.data;
  } catch (e) {
    console.error('Failed to fetch devices', e);
  }
};

onMounted(fetchDevices);
</script>
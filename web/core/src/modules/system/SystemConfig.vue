<template>
  <div class="p-8 lg:p-12 space-y-10 animate-in fade-in duration-500">
    <div>
      <h1 class="text-3xl font-black text-slate-800 tracking-tight italic">SYSTEM <span class="text-rose-600 not-italic">CONFIG</span></h1>
      <p class="text-slate-400 text-sm font-medium mt-1 uppercase tracking-widest">Global Parameter Management</p>
    </div>

    <div class="grid grid-cols-1 gap-8">
      <div class="bg-white rounded-[2.5rem] shadow-xl shadow-slate-200/50 border border-slate-100 p-10">
        <div class="space-y-8">
          <div v-for="param in parameters" :key="param.key" class="flex items-center justify-between group">
            <div class="space-y-1">
              <div class="text-xs font-black text-slate-400 uppercase tracking-widest">{{ param.key }}</div>
              <div class="text-[10px] text-slate-300 font-mono italic">TYPE: {{ param.type }}</div>
            </div>
            
            <div class="flex items-center gap-4">
              <input 
                v-model="param.pendingValue"
                type="text" 
                class="w-32 bg-slate-50 border border-slate-100 rounded-xl px-4 py-2 text-right font-mono font-bold text-slate-700 outline-none focus:ring-2 focus:ring-rose-500/20"
              />
              <button 
                @click="updateParam(param)"
                class="px-4 py-2 bg-slate-900 text-white rounded-xl text-[10px] font-black uppercase tracking-widest hover:bg-rose-600 transition-all shadow-lg shadow-slate-900/10 active:scale-95"
              >
                Apply
              </button>
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue';
import axios from 'axios';

const parameters = ref<any[]>([]);

const fetchParams = async () => {
  try {
    const res = await axios.get('/api/v1/system/config');
    parameters.value = (res.data.params || []).map((p: any) => ({
      ...p,
      pendingValue: p.value
    }));
  } catch (e) {
    parameters.value = [
      { key: 'vpp_max_charge_w', value: 5000, type: 'float', pendingValue: 5000 },
      { key: 'grid_freq_threshold_hz', value: 0.5, type: 'float', pendingValue: 0.5 }
    ];
  }
};

const updateParam = async (param: any) => {
  try {
    await axios.post('/api/v1/system/config', {
      key: param.key,
      value: parseFloat(param.pendingValue)
    });
    alert(`Parameter ${param.key} queued for update.`);
    fetchParams();
  } catch (e) {
    alert('Failed to update parameter');
  }
};

onMounted(fetchParams);
</script>
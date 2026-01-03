<template>
  <div class="p-8 lg:p-12 space-y-10 animate-in fade-in duration-500">
    <div class="flex justify-between items-end">
      <div>
        <h1 class="text-3xl font-black text-slate-800 tracking-tight italic">COMPUTE <span class="text-indigo-600 not-italic">PIPELINES</span></h1>
        <p class="text-slate-400 text-sm font-medium mt-1 uppercase tracking-widest">Stream Processing Engines</p>
      </div>
    </div>

    <div class="grid grid-cols-1 lg:grid-cols-2 gap-8">
      <div v-for="p in pipelines" :key="p.id" class="bg-white rounded-[2.5rem] p-8 shadow-xl shadow-slate-200/50 border border-slate-100 flex flex-col justify-between">
        <div class="space-y-6">
          <div class="flex justify-between items-start">
            <div class="space-y-1">
              <h3 class="text-xl font-black text-slate-800 tracking-tight">{{ p.id }}</h3>
              <div class="text-[10px] font-bold text-slate-400 uppercase tracking-widest flex items-center gap-2">
                Trigger: <span class="text-indigo-500">Tag #{{ p.trigger_tag || 'Any' }}</span>
              </div>
            </div>
            <div class="px-3 py-1 bg-emerald-50 text-emerald-600 rounded-full text-[9px] font-black uppercase tracking-widest">Running</div>
          </div>

          <!-- Pipeline Visualization -->
          <div class="flex items-center gap-3">
            <div v-for="(stage, idx) in p.stages" :key="idx" class="flex items-center gap-3">
              <div class="px-4 py-2 bg-slate-50 border border-slate-100 rounded-xl text-[10px] font-bold text-slate-600 shadow-sm">
                {{ stage.name }}
              </div>
              <svg v-if="idx < p.stages.length - 1" xmlns="http://www.w3.org/2000/svg" class="h-4 w-4 text-slate-200" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                <path stroke-linecap="round" stroke-linejoin="round" stroke-width="3" d="M13 7l5 5-5 5M6 7l5 5-5 5" />
              </svg>
            </div>
          </div>
        </div>

        <div class="mt-8 pt-6 border-t border-slate-50 flex justify-between items-center text-[10px] font-black uppercase tracking-widest">
          <span class="text-slate-400">Throughput: {{ p.total_events || 0 }} msg</span>
          <span class="text-indigo-500">Latency: {{ p.latency || 0.5 }}ms</span>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue';
import axios from 'axios';

const pipelines = ref<any[]>([]);

const fetchPipelines = async () => {
  try {
    const res = await axios.get('/api/v1/pipelines');
    // 如果返回的是列表，直接赋值；如果是对象，提取 values
    pipelines.value = Array.isArray(res.data) ? res.data : Object.values(res.data);
  } catch (e) {
    // Mock for development
    pipelines.value = [
      { id: 'POWER_SMOOTHING', trigger_tag: 1003, stages: [{name: 'KalmanFilter'}, {name: 'Limiter'}], total_events: 1240 },
      { id: 'ALARM_ENGINE', trigger_tag: 0, stages: [{name: 'ThresholdCheck'}, {name: 'MQTT_Push'}], total_events: 8560 }
    ];
  }
};

onMounted(fetchPipelines);
</script>

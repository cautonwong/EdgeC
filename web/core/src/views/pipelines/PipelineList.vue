<template>
  <div class="p-6 max-w-7xl mx-auto">
    <h2 class="text-2xl font-bold text-gray-800 mb-6">Execution Pipelines</h2>

    <div class="space-y-6">
      <div v-for="pipe in pipelines" :key="pipe.id" 
           class="bg-white rounded-xl shadow-sm border border-gray-100 p-6 hover:shadow-md transition-shadow">
        
        <div class="flex justify-between items-start mb-6">
          <div>
            <div class="text-lg font-bold text-gray-900 flex items-center gap-2">
              {{ pipe.id }}
              <span :class="statusClass(pipe.status)" class="text-[10px] px-2 py-0.5 rounded-full uppercase border">
                {{ pipe.status }}
              </span>
            </div>
            <div class="text-xs text-gray-400 mt-1">Trigger Tag: #{{ pipe.trigger_tag }}</div>
          </div>
          
          <div class="flex gap-2">
            <button class="text-sm px-3 py-1 bg-gray-50 text-gray-600 rounded-md border hover:bg-gray-100">Stats</button>
            <button class="text-sm px-3 py-1 bg-gray-50 text-indigo-600 rounded-md border border-indigo-100 hover:bg-indigo-50">Edit</button>
          </div>
        </div>

        <!-- Pipeline Graph (Visual Chain) -->
        <div class="flex items-center gap-2 overflow-x-auto pb-2">
          <div class="flex-shrink-0 bg-amber-50 border border-amber-200 px-4 py-2 rounded-lg text-amber-700 text-sm font-medium">
            Trigger
          </div>
          
          <template v-for="(stage, index) in pipe.stages" :key="index">
            <svg xmlns="http://www.w3.org/2000/svg" class="h-4 w-4 text-gray-300" fill="none" viewBox="0 0 24 24" stroke="currentColor">
              <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M13 7l5 5m0 0l-5 5m5-5H6" />
            </svg>
            <div class="flex-shrink-0 bg-blue-50 border border-blue-200 px-4 py-2 rounded-lg text-blue-700 text-sm">
              <div class="font-bold uppercase text-[10px] text-blue-400 leading-none mb-1">Stage</div>
              {{ stage.name }}
            </div>
          </template>

          <svg xmlns="http://www.w3.org/2000/svg" class="h-4 w-4 text-gray-300" fill="none" viewBox="0 0 24 24" stroke="currentColor">
            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M13 7l5 5m0 0l-5 5m5-5H6" />
          </svg>
          <div class="flex-shrink-0 bg-gray-50 border border-gray-200 px-4 py-2 rounded-lg text-gray-400 text-sm italic">
            End
          </div>
        </div>
      </div>

      <div v-if="pipelines.length === 0" class="text-center py-20 bg-gray-50 rounded-xl border-2 border-dashed border-gray-200">
        <div class="text-gray-400 font-medium">No active pipelines found in Stream Engine.</div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted } from 'vue';
import axios from 'axios';

interface Stage {
  name: string;
}

interface Pipeline {
  id: string;
  trigger_tag: number;
  status: 'idle' | 'running' | 'suspended';
  stages: Stage[];
}

const pipelines = ref<Pipeline[]>([]);
let timer: number;

const fetchPipelines = async () => {
  try {
    const res = await axios.get('/api/v1/pipelines');
    pipelines.value = res.data;
  } catch (e) {
    console.error('Failed to fetch pipelines:', e);
  }
};

function statusClass(status: string) {
  switch (status) {
    case 'running': return 'bg-green-50 text-green-600 border-green-200';
    case 'suspended': return 'bg-amber-50 text-amber-600 border-amber-200';
    default: return 'bg-gray-50 text-gray-500 border-gray-200';
  }
}

onMounted(() => {
  fetchPipelines();
  timer = setInterval(fetchPipelines, 5000);
});

onUnmounted(() => clearInterval(timer));
</script>
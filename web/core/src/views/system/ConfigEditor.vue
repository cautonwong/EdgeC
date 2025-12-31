<template>
  <div class="p-6 max-w-7xl mx-auto">
    <div class="mb-8">
      <h2 class="text-2xl font-bold text-gray-800">System Configuration</h2>
      <p class="text-sm text-gray-500 mt-1">Manage runtime parameters and control setpoints.</p>
    </div>

    <div class="grid grid-cols-1 lg:grid-cols-3 gap-8">
      <!-- Parameters List -->
      <div class="lg:col-span-2 space-y-4">
        <div v-for="param in parameters" :key="param.key" 
             class="bg-white p-5 rounded-xl shadow-sm border border-gray-100 flex justify-between items-center hover:border-indigo-200 transition-all group">
          <div>
            <div class="text-sm font-bold text-gray-700 font-mono">{{ param.key }}</div>
            <div class="text-xs text-gray-400 mt-1">Type: {{ param.type }} | Registry: Persistent</div>
          </div>
          <div class="flex items-center gap-4">
            <div class="text-lg font-mono font-bold text-indigo-600">{{ param.value }}</div>
            <button @click="openEditor(param)" 
                    class="p-2 opacity-0 group-hover:opacity-100 bg-gray-50 text-gray-400 hover:text-indigo-600 hover:bg-indigo-50 rounded-lg transition-all">
              <svg xmlns="http://www.w3.org/2000/svg" class="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M15.232 5.232l3.536 3.536m-2.036-5.036a2.5 2.5 0 113.536 3.536L6.5 21.036H3v-3.572L16.732 3.732z" />
              </svg>
            </button>
          </div>
        </div>
      </div>

      <!-- Quick Actions / Info -->
      <div class="space-y-6">
        <div class="bg-indigo-900 text-white p-6 rounded-xl shadow-lg shadow-indigo-200">
          <h3 class="font-bold mb-3 flex items-center gap-2">
            <svg xmlns="http://www.w3.org/2000/svg" class="h-5 w-5 text-indigo-300" fill="none" viewBox="0 0 24 24" stroke="currentColor">
              <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M13 16h-1v-4h-1m1-4h.01M21 12a9 9 0 11-18 0 9 9 0 0118 0z" />
            </svg>
            Audit Notice
          </h3>
          <p class="text-sm text-indigo-100 leading-relaxed">
            All parameter changes are logged to the <strong>edge_audit</strong> table in SQLite. 
            Modifications take effect immediately in the logic engine.
          </p>
        </div>
      </div>
    </div>

    <!-- Edit Modal (Simplified) -->
    <div v-if="editingParam" class="fixed inset-0 bg-black/50 backdrop-blur-sm flex items-center justify-center p-4 z-50">
      <div class="bg-white rounded-2xl p-8 max-w-md w-full shadow-2xl">
        <h3 class="text-xl font-bold mb-2">Update Parameter</h3>
        <p class="text-sm text-gray-500 mb-6 font-mono">{{ editingParam.key }}</p>
        
        <input v-model.number="editValue" type="number" step="0.1"
               class="w-full px-4 py-3 bg-gray-50 border border-gray-200 rounded-xl mb-6 focus:ring-2 focus:ring-indigo-500 outline-none font-mono text-xl" />
        
        <div class="flex gap-3">
          <button @click="editingParam = null" class="flex-1 py-3 text-gray-500 hover:bg-gray-50 rounded-xl font-medium">Cancel</button>
          <button @click="saveParam" class="flex-1 py-3 bg-indigo-600 text-white rounded-xl font-bold shadow-lg shadow-indigo-200 hover:bg-indigo-700 transition-colors">Apply Change</button>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue';
import axios from 'axios';

interface Parameter {
  key: string;
  value: number;
  type: string;
}

const parameters = ref<Parameter[]>([]);
const editingParam = ref<Parameter | null>(null);
const editValue = ref(0);

const fetchParams = async () => {
  try {
    const res = await axios.get('/api/v1/system/config');
    parameters.value = res.data.params;
  } catch (e) {
    console.error('Failed to load system config:', e);
  }
};

const openEditor = (p: Parameter) => {
  editingParam.value = p;
  editValue.value = p.value;
};

const saveParam = async () => {
  if (!editingParam.value) return;
  try {
    await axios.post('/api/v1/system/config', {
      key: editingParam.value.key,
      value: editValue.value
    });
    editingParam.value = null;
    fetchParams(); // Refresh
  } catch (e) {
    alert('Update failed');
  }
};

onMounted(fetchParams);
</script>
